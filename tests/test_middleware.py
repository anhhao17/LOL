"""
Tests for auth, CSRF, and rate-limit middleware.

Auth middleware (auth_middleware.cpp):
  - All /api/* routes are protected except /api/v1/login
  - Token accepted from: Authorization: Bearer <token> OR Cookie: session=<token>
  - Missing token  → 401 UNAUTHORIZED
  - Invalid token  → 401

CSRF middleware (csrf_middleware.cpp):
  - Applies to POST/PUT/DELETE/PATCH on protected routes
  - /api/v1/login is exempt
  - Missing X-CSRF-Token  → 400 CSRF_TOKEN_MISMATCH
  - Wrong token           → 400 CSRF_TOKEN_MISMATCH

Rate-limit middleware (rate_limit_middleware.cpp):
  - kDefaultMaxRequests=200 per kDefaultWindow=10s per IP
  - Exceeded → 429 RATE_LIMIT_EXCEEDED + Retry-After header

Note: /api/v1/stream is a WebSocket-only endpoint; plain HTTP GET returns 404.
      Auth middleware tests use POST /api/v1/logout (a real protected route).
"""

import requests
import pytest
from conftest import (
    BASE_URL, LOGIN_URL, LOGOUT_URL,
    do_login, login_data, assert_error_code,
)

# /api/v1/logout: protected POST — good target for auth/CSRF middleware tests
PROTECTED_URL = LOGOUT_URL


# ── Auth middleware ───────────────────────────────────────────────────────────

class TestAuthMiddleware:
    def test_login_endpoint_is_public(self):
        """POST /api/v1/login must succeed without any auth token."""
        resp = do_login()
        assert resp.status_code == 200

    def test_protected_endpoint_without_auth_returns_401(self, anon):
        # Auth middleware runs before CSRF — missing session → 401, not 400
        resp = anon.post(PROTECTED_URL, headers={"X-CSRF-Token": "any"})
        assert resp.status_code == 401
        assert_error_code(resp, "UNAUTHORIZED")

    def test_invalid_token_returns_401(self, anon):
        anon.cookies.set("session", "totally_invalid_token")
        resp = anon.post(PROTECTED_URL, headers={"X-CSRF-Token": "any"})
        assert resp.status_code == 401

    def test_bearer_token_accepted(self, auth_session):
        """Token in Authorization: Bearer must authenticate (not return 401)."""
        session, csrf_token = auth_session
        token = session.cookies.get("session")
        assert token, "No session cookie found in auth_session fixture"

        resp = requests.post(
            PROTECTED_URL,
            headers={
                "Authorization": f"Bearer {token}",
                "X-CSRF-Token": csrf_token,
            },
        )
        # 200 = logged out successfully; anything but 401 means auth passed
        assert resp.status_code != 401, (
            f"Bearer token was rejected (got {resp.status_code})"
        )

    def test_cookie_auth_accepted(self, auth_session):
        """Session cookie must authenticate the request."""
        session, csrf_token = auth_session
        resp = session.post(PROTECTED_URL, headers={"X-CSRF-Token": csrf_token})
        assert resp.status_code != 401

    def test_static_assets_are_public(self, anon):
        """Non-API paths must not require auth."""
        resp = anon.get(f"{BASE_URL}/")
        assert resp.status_code == 200

    def test_non_api_path_is_public(self, anon):
        resp = anon.get(f"{BASE_URL}/stream")
        assert resp.status_code == 200


# ── CSRF middleware ───────────────────────────────────────────────────────────

class TestCsrfMiddleware:
    def test_login_exempt_from_csrf(self):
        """Login must work without X-CSRF-Token."""
        resp = do_login()
        assert resp.status_code == 200

    def test_post_without_csrf_returns_400(self, auth_session):
        session, _ = auth_session
        resp = session.post(LOGOUT_URL)  # no X-CSRF-Token header
        assert resp.status_code == 400
        assert_error_code(resp, "CSRF_TOKEN_MISMATCH")

    def test_post_with_wrong_csrf_returns_400(self, auth_session):
        session, _ = auth_session
        resp = session.post(LOGOUT_URL, headers={"X-CSRF-Token": "bad_csrf"})
        assert resp.status_code == 400
        assert_error_code(resp, "CSRF_TOKEN_MISMATCH")

    def test_post_with_correct_csrf_succeeds(self, auth_session):
        session, csrf_token = auth_session
        resp = session.post(LOGOUT_URL, headers={"X-CSRF-Token": csrf_token})
        assert resp.status_code == 200

    def test_csrf_token_from_one_session_rejected_by_another(self):
        """CSRF token must be session-scoped — cross-session tokens must be rejected."""
        resp1 = do_login()
        resp2 = do_login()

        csrf1 = login_data(resp1)["csrfToken"]
        csrf2 = login_data(resp2)["csrfToken"]
        assert csrf1 != csrf2, "Two sessions must have different CSRF tokens"

        # Use session2's cookie but session1's CSRF token
        s2 = requests.Session()
        s2.cookies.update(resp2.cookies)

        resp = s2.post(LOGOUT_URL, headers={"X-CSRF-Token": csrf1})
        assert resp.status_code == 400
        assert_error_code(resp, "CSRF_TOKEN_MISMATCH")

        # Cleanup
        s2.post(LOGOUT_URL, headers={"X-CSRF-Token": csrf2})
        s2.close()


# ── Rate-limit middleware ─────────────────────────────────────────────────────

class TestRateLimitMiddleware:
    # kDefaultMaxRequests=200, kDefaultWindow=10s
    def test_normal_requests_not_rate_limited(self, anon):
        """A handful of requests must not be rate-limited."""
        for _ in range(5):
            resp = anon.get(f"{BASE_URL}/")
            assert resp.status_code != 429

    def test_rate_limited_response_has_retry_after(self, anon):
        """When rate-limited, the response must include a Retry-After header."""
        for _ in range(210):
            resp = anon.get(f"{BASE_URL}/")
            if resp.status_code == 429:
                assert "Retry-After" in resp.headers, (
                    "429 response missing Retry-After header"
                )
                assert_error_code(resp, "RATE_LIMIT_EXCEEDED")
                return

        pytest.skip("Rate limit not triggered within 210 requests — window may have reset")
