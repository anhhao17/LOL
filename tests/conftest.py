"""
Shared fixtures for QUANTUM server integration tests.

Set BASE_URL env var to override default (http://127.0.0.1:8080).
Set ADMIN_USER / ADMIN_PASS to override default credentials.
"""

import os
import pytest
import requests

BASE_URL    = os.environ.get("BASE_URL",    "http://127.0.0.1:8080")
ADMIN_USER  = os.environ.get("ADMIN_USER",  "admin")
ADMIN_PASS  = os.environ.get("ADMIN_PASS",  "admin")

LOGIN_URL   = f"{BASE_URL}/api/v1/login"
LOGOUT_URL  = f"{BASE_URL}/api/v1/logout"
STREAM_URL  = f"{BASE_URL}/api/v1/stream"


# ── Helpers ───────────────────────────────────────────────────────────────────

def do_login(username=ADMIN_USER, password=ADMIN_PASS) -> requests.Response:
    """POST /api/v1/login, return raw response."""
    return requests.post(LOGIN_URL, json={"username": username, "password": password})


def login_data(resp: requests.Response) -> dict:
    """Extract the data payload from a successful login response."""
    return resp.json()["data"]


def assert_error_code(response: requests.Response, expected_code: str) -> None:
    """Assert response JSON contains the expected error code string."""
    body = response.json()
    assert "error" in body, f"Expected error body, got: {body}"
    assert body["error"]["code"] == expected_code, (
        f"Expected error code '{expected_code}', got '{body['error']['code']}'"
    )


# ── Fixtures ──────────────────────────────────────────────────────────────────

@pytest.fixture(scope="session")
def base_url() -> str:
    return BASE_URL


@pytest.fixture
def anon() -> requests.Session:
    """Unauthenticated session."""
    with requests.Session() as s:
        yield s


@pytest.fixture
def auth_session():
    """
    Authenticated session.  Yields (session, csrf_token).
    Automatically logs out after the test.
    """
    resp = do_login()
    assert resp.status_code == 200, f"Login failed: {resp.status_code} {resp.text}"

    data       = login_data(resp)
    csrf_token = data["csrfToken"]

    session = requests.Session()
    session.cookies.update(resp.cookies)

    yield session, csrf_token

    try:
        session.post(LOGOUT_URL, headers={"X-CSRF-Token": csrf_token})
    except Exception:
        pass
    session.close()
