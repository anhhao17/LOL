"""
Tests for POST /api/v1/login and POST /api/v1/logout.

Login behaviour (login_handler.cpp):
  - Returns 200 + {"data": {csrfToken, username, role}} + session cookie on success
  - Returns 401 INVALID_CREDENTIALS for wrong password or unknown user
  - Returns 400 INVALID_INPUT for missing / oversized fields

Logout behaviour (logout_handler.cpp):
  - Returns 200 and clears the session cookie (Max-Age=0)
  - Requires valid session cookie + correct X-CSRF-Token header
  - 401 if no session; 400 CSRF_TOKEN_MISMATCH if token missing/wrong

NOTE: Brute-force / lockout tests are in test_bruteforce.py and must run last
      to avoid locking 127.0.0.1 and breaking subsequent login-dependent tests.
"""

import requests
import pytest
from conftest import (
    LOGIN_URL, LOGOUT_URL,
    ADMIN_USER, ADMIN_PASS,
    do_login, login_data, assert_error_code,
)


# ── Login: success ────────────────────────────────────────────────────────────

class TestLoginSuccess:
    def test_returns_200(self):
        resp = do_login()
        assert resp.status_code == 200

    def test_body_wrapped_in_data(self):
        resp = do_login()
        assert "data" in resp.json(), f"Expected 'data' envelope, got: {resp.json()}"

    def test_body_contains_csrf_token(self):
        data = login_data(do_login())
        assert "csrfToken" in data
        assert isinstance(data["csrfToken"], str)
        assert len(data["csrfToken"]) > 0

    def test_body_contains_username(self):
        data = login_data(do_login())
        assert data["username"] == ADMIN_USER

    def test_body_contains_role(self):
        data = login_data(do_login())
        assert "role" in data
        assert isinstance(data["role"], int)

    def test_sets_session_cookie(self):
        resp = do_login()
        assert "session" in resp.cookies, "Expected 'session' cookie in response"

    def test_sequential_logins_return_different_tokens(self):
        token1 = login_data(do_login())["csrfToken"]
        token2 = login_data(do_login())["csrfToken"]
        assert token1 != token2, "Each login should produce a unique CSRF token"


# ── Login: invalid credentials ────────────────────────────────────────────────
#
# IMPORTANT: each test here consumes one fail-counter slot on 127.0.0.1.
# kMaxFailAttempts = 5 (fail_delay.hpp). Keep total bad credential calls ≤ 4
# so the counter never reaches lockout before TestLogout's auth_session fixture
# resets it with a successful login.  Brute-force lockout tests live in
# test_bruteforce.py and run last.

class TestLoginInvalidCredentials:
    def test_wrong_password_returns_401(self):
        # Combines status + error code check — one bad credential call.
        resp = do_login(password="definitely_wrong_password")
        assert resp.status_code == 401
        assert_error_code(resp, "INVALID_CREDENTIALS")   # fail count: 1

    def test_unknown_user_returns_401(self):
        resp = do_login(username="no_such_user_xyz", password="anything")
        assert resp.status_code == 401                    # fail count: 2

    def test_unknown_user_indistinguishable_from_wrong_password(self):
        """Error code must be identical whether the user exists or not (no enumeration)."""
        # One call only — we already know wrong_password → INVALID_CREDENTIALS above.
        resp = do_login(username="another_no_such_user", password="anything")
        assert_error_code(resp, "INVALID_CREDENTIALS")   # fail count: 3

    def test_empty_password_returns_4xx(self):
        resp = do_login(password="")
        assert resp.status_code in (400, 401)             # fail count: 4 (≤ limit)


# ── Login: input validation ───────────────────────────────────────────────────

class TestLoginInputValidation:
    def test_missing_username_returns_400(self):
        resp = requests.post(LOGIN_URL, json={"password": ADMIN_PASS})
        assert resp.status_code == 400
        assert_error_code(resp, "INVALID_INPUT")

    def test_missing_password_returns_400(self):
        resp = requests.post(LOGIN_URL, json={"username": ADMIN_USER})
        assert resp.status_code == 400
        assert_error_code(resp, "INVALID_INPUT")

    def test_empty_body_returns_400(self):
        resp = requests.post(LOGIN_URL, json={})
        assert resp.status_code == 400

    def test_non_json_body_returns_400(self):
        resp = requests.post(LOGIN_URL, data="not json",
                             headers={"Content-Type": "text/plain"})
        assert resp.status_code == 400

    def test_username_too_long_returns_400(self):
        resp = do_login(username="a" * 65)
        assert resp.status_code == 400
        assert_error_code(resp, "INVALID_INPUT")

    def test_password_too_long_returns_400(self):
        resp = do_login(password="p" * 257)
        assert resp.status_code == 400
        assert_error_code(resp, "INVALID_INPUT")


# ── Logout ────────────────────────────────────────────────────────────────────

class TestLogout:
    def test_logout_returns_200(self, auth_session):
        session, csrf_token = auth_session
        resp = session.post(LOGOUT_URL, headers={"X-CSRF-Token": csrf_token})
        assert resp.status_code == 200

    def test_logout_clears_session_cookie(self, auth_session):
        session, csrf_token = auth_session
        resp = session.post(LOGOUT_URL, headers={"X-CSRF-Token": csrf_token})
        set_cookie = resp.headers.get("Set-Cookie", "")
        assert "session" in set_cookie
        assert "Max-Age=0" in set_cookie or "max-age=0" in set_cookie.lower()

    def test_logout_invalidates_session(self, auth_session):
        """After logout the old session cookie must no longer grant access."""
        session, csrf_token = auth_session
        session.post(LOGOUT_URL, headers={"X-CSRF-Token": csrf_token})
        # Second logout attempt with same (now-invalid) session must fail
        resp = session.post(LOGOUT_URL, headers={"X-CSRF-Token": csrf_token})
        assert resp.status_code == 401

    def test_logout_without_session_returns_401(self, anon):
        resp = anon.post(LOGOUT_URL, headers={"X-CSRF-Token": "any"})
        assert resp.status_code == 401

    def test_logout_without_csrf_token_returns_400(self, auth_session):
        session, _ = auth_session
        resp = session.post(LOGOUT_URL)
        assert resp.status_code == 400
        assert_error_code(resp, "CSRF_TOKEN_MISMATCH")

    def test_logout_with_wrong_csrf_token_returns_400(self, auth_session):
        session, _ = auth_session
        resp = session.post(LOGOUT_URL, headers={"X-CSRF-Token": "wrong_token"})
        assert resp.status_code == 400
        assert_error_code(resp, "CSRF_TOKEN_MISMATCH")
