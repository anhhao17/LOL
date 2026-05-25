"""
Tests for GET /api/v1/stream (WebSocket streaming endpoint).

websocket_handler.cpp behaviour:
  - Requires a valid HTTP Upgrade handshake (RFC 6455)
  - Auth token from: Sec-WebSocket-Protocol header OR Cookie: session=<token>
  - View type from: ?view=<type> query param OR Sec-WebSocket-Protocol header
  - Valid view types: cl_view, ir_view, both_views, cl_sub_view, ir_sub_view, both_sub_views
  - Missing auth          → 401 UNAUTHORIZED
  - Invalid token         → 401 INVALID_TOKEN
  - Missing/invalid view  → 400 INVALID_STREAM_TYPE
  - Missing upgrade hdrs  → 400 INVALID_WEBSOCKET_UPGRADE
  - On connect: server sends binary frames [1-byte tag][JPEG bytes...]
      tag 0x00 = CL channel, 0x01 = IR channel
"""

import time
import pytest
import requests
import websocket  # websocket-client
from conftest import BASE_URL, ADMIN_USER, ADMIN_PASS, do_login, assert_error_code

STREAM_HTTP_URL = f"{BASE_URL}/api/v1/stream"
STREAM_WS_URL   = STREAM_HTTP_URL.replace("http://", "ws://")

VALID_VIEWS   = ["cl_view", "ir_view", "both_views", "cl_sub_view", "ir_sub_view", "both_sub_views"]
INVALID_VIEWS = ["", "unknown", "cl", "COLOR", "0", "null"]


def get_session_token() -> str:
    resp = do_login()
    assert resp.status_code == 200
    return resp.cookies.get("session", "")


# ── HTTP upgrade validation (no WS library) ───────────────────────────────────

class TestStreamHttpValidation:
    """Hit the endpoint as a plain HTTP GET to check pre-upgrade rejection paths."""

    def test_no_auth_returns_401(self, anon):
        resp = anon.get(STREAM_HTTP_URL, params={"view": "cl_view"})
        assert resp.status_code == 401
        assert_error_code(resp, "UNAUTHORIZED")

    def test_invalid_token_returns_401(self, anon):
        anon.cookies.set("session", "bogus_token")
        resp = anon.get(STREAM_HTTP_URL, params={"view": "cl_view"})
        assert resp.status_code == 401

    def test_missing_view_param_returns_400(self, auth_session):
        session, _ = auth_session
        # Auth is fine, but no ?view= — server must reject before upgrading
        resp = session.get(STREAM_HTTP_URL)
        assert resp.status_code == 400
        assert_error_code(resp, "INVALID_STREAM_TYPE")

    def test_invalid_view_param_returns_400(self, auth_session):
        session, _ = auth_session
        resp = session.get(STREAM_HTTP_URL, params={"view": "bad_view"})
        assert resp.status_code == 400
        assert_error_code(resp, "INVALID_STREAM_TYPE")

    @pytest.mark.parametrize("view", INVALID_VIEWS)
    def test_invalid_view_types(self, auth_session, view):
        session, _ = auth_session
        resp = session.get(STREAM_HTTP_URL, params={"view": view})
        assert resp.status_code == 400


# ── WebSocket connection (requires server running) ────────────────────────────

class TestStreamWebSocket:
    @pytest.fixture(autouse=True)
    def token(self):
        self._token = get_session_token()

    def _connect(self, view: str, token: str = None, timeout: int = 5):
        """Open a WebSocket connection using cookie auth."""
        tok = token or self._token
        url = f"{STREAM_WS_URL}?view={view}"
        ws = websocket.WebSocket()
        ws.connect(url, cookie=f"session={tok}", timeout=timeout)
        return ws

    @pytest.mark.parametrize("view", VALID_VIEWS)
    def test_valid_view_connects(self, view):
        """Server must accept the upgrade for every valid view type."""
        ws = self._connect(view)
        assert ws.connected
        ws.close()

    def test_no_token_rejected(self):
        """WebSocket without auth must be refused."""
        url = f"{STREAM_WS_URL}?view=cl_view"
        with pytest.raises(websocket.WebSocketBadStatusException) as exc_info:
            ws = websocket.WebSocket()
            ws.connect(url, timeout=5)
        assert exc_info.value.status_code == 401

    def test_invalid_token_rejected(self):
        url = f"{STREAM_WS_URL}?view=cl_view"
        with pytest.raises(websocket.WebSocketBadStatusException) as exc_info:
            ws = websocket.WebSocket()
            ws.connect(url, cookie="session=invalid_token", timeout=5)
        assert exc_info.value.status_code == 401

    def test_missing_view_rejected(self):
        with pytest.raises(websocket.WebSocketBadStatusException) as exc_info:
            ws = websocket.WebSocket()
            ws.connect(STREAM_WS_URL, cookie=f"session={self._token}", timeout=5)
        assert exc_info.value.status_code == 400

    def test_receives_binary_frames(self):
        """After connecting, server must send binary JPEG frames within 5 seconds."""
        ws = self._connect("cl_view")
        ws.settimeout(5)
        try:
            raw = ws.recv()
        except websocket.WebSocketTimeoutException:
            ws.close()
            pytest.fail("No frame received within 5 seconds (is --video-cl configured?)")

        assert isinstance(raw, bytes), "Frame must be binary"
        assert len(raw) > 1, "Frame must contain more than just the channel tag"

        channel_tag = raw[0]
        assert channel_tag == 0x00, (
            f"cl_view frame must have tag 0x00 (CL), got 0x{channel_tag:02x}"
        )

        # Minimal JPEG magic: first 2 bytes of payload must be FF D8
        jpeg = raw[1:]
        assert jpeg[:2] == b"\xff\xd8", (
            "Payload must start with JPEG magic bytes FF D8"
        )
        ws.close()

    def test_ir_view_frame_tag(self):
        """IR frames must carry tag 0x01."""
        ws = self._connect("ir_view")
        ws.settimeout(5)
        try:
            raw = ws.recv()
        except websocket.WebSocketTimeoutException:
            ws.close()
            pytest.skip("No IR frame received — is --video-ir configured?")

        assert raw[0] == 0x01, (
            f"ir_view frame must have tag 0x01, got 0x{raw[0]:02x}"
        )
        ws.close()

    def test_both_views_receives_frames(self):
        """both_views subscription must receive frames from at least one channel."""
        ws = self._connect("both_views")
        ws.settimeout(5)
        try:
            raw = ws.recv()
        except websocket.WebSocketTimeoutException:
            ws.close()
            pytest.fail("No frame received on both_views within 5 seconds")

        assert raw[0] in (0x00, 0x01), (
            f"Frame tag must be 0x00 or 0x01, got 0x{raw[0]:02x}"
        )
        ws.close()

    def test_multiple_concurrent_clients(self):
        """Multiple WebSocket clients must all receive frames independently."""
        ws1 = self._connect("cl_view")
        ws2 = self._connect("cl_view")
        ws1.settimeout(5)
        ws2.settimeout(5)

        try:
            raw1 = ws1.recv()
            raw2 = ws2.recv()
        except websocket.WebSocketTimeoutException:
            pytest.fail("One or more concurrent clients received no frames")
        finally:
            ws1.close()
            ws2.close()

        assert raw1[0] == 0x00
        assert raw2[0] == 0x00

    def test_connection_survives_30_seconds(self):
        """WebSocket must not time out after 30 s (HTTP read timer bug regression)."""
        ws = self._connect("cl_view")
        ws.settimeout(35)
        try:
            time.sleep(31)
            # Send a ping; connection should still be alive
            ws.ping()
            # Try to receive one frame
            ws.recv()
        except (websocket.WebSocketTimeoutException, websocket.WebSocketConnectionClosedException):
            ws.close()
            pytest.fail("WebSocket disconnected after ~30 s — expires_never() regression?")
        finally:
            ws.close()
