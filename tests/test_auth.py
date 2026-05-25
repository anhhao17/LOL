"""
Tests for POST /api/v1/login and POST /api/v1/logout.

Login behaviour (from login_handler.cpp):
  - Returns 200 + JSON {csrfToken, username, role} + session cookie on success
  - Returns 401 INVALID_CREDENTIALS for wrong password or unknown user
  - Returns 400 INVALID_INPUT for missing / oversized fields
  - Returns 429 TOO_MANY_REQUESTS after 5 consecutive failures (5-min lockout)

Logout behaviour (from logout_handler.cpp):
  - Returns 200 and clears the session cookie
  - Protected: requires valid session cookie + X-CSRF-Token header
  - Returns 401 if no session; 400 if CSRF token missing/wrong
"""

import requests
import pytest
from conftest import (
    LOGIN_URL, LOGOUT_URL,
    ADMIN_USER, ADMIN_PASS,
    do_login, assert_error_code,
)


# ── Login: success ────────────────────────────────────────────────────────────

class TestLoginSuccess:
    def test_returns_200(self):
        resp = do_login()
        assert resp.status_code == 200

    def test_body_contains_csrf_token(self):
        resp = do_login()
        body = resp.json()
        assert "csrfToken" in body
        assert isinstance(body["csrfToken"], str)
        assert len(body["csrfToken"]) > 0

    def test_body_contains_username(self):
        resp = do_login()
        assert resp.json()["username"] == ADMIN_USER

    def test_body_contains_role(self):
        resp = do_login()
        body = resp.json()
        assert "role" in body
        assert isinstance(body["role"], int)

    def test_sets_session_cookie(self):
        resp = do_login()
        assert "session" in resp.cookies, "Expected 'session' cookie in response"

    def test_session_cookie_is_httponly(self):
        resp = do_login()
        cookie = resp.cookies._cookies.get("127.0.0.1", {}).get("/", {}).get("session")
        if cookie is not None:
            assert cookie.has_nonstandard_attr("HttpOnly") or "httponly" in str(cookie).lower()

    def test_sequential_logins_return_different_tokens(self):
        token1 = do_login().json()["csrfToken"]
        token2 = do_login().json()["csrfToken"]
        assert token1 != token2, "Each login should produce a unique CSRF token"


# ── Login: invalid credentials ────────────────────────────────────────────────

class TestLoginInvalidCredentials:
    def test_wrong_password_returns_401(self):
        resp = do_login(password="definitely_wrong_password")
        assert resp.status_code == 401

    def test_wrong_password_error_code(self):
        resp = do_login(password="definitely_wrong_password")
        assert_error_code(resp, "INVALID_CREDENTIALS")

    def test_unknown_user_returns_401(self):
        resp = do_login(username="no_such_user", password="anything")
        assert resp.status_code == 401

    def test_unknown_user_same_error_as_wrong_password(self):
        """Server must not reveal whether the username exists (enumeration prevention)."""
        resp_bad_user = do_login(username="no_such_user", password="anything")
        resp_bad_pass = do_login(username=ADMIN_USER, password="wrong")
        assert resp_bad_user.json()["error"]["code"] == resp_bad_pass.json()["error"]["code"]

    def test_empty_password_returns_401(self):
        resp = do_login(password="")
        assert resp.status_code in (400, 401)


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
        resp = requests.post(LOGIN_URL, data="not json", headers={"Content-Type": "text/plain"})
        assert resp.status_code == 400

    def test_username_too_long_returns_400(self):
        resp = do_login(username="a" * 65)
        assert resp.status_code == 400
        assert_error_code(resp, "INVALID_INPUT")

    def test_password_too_long_returns_400(self):
        resp = do_login(password="p" * 257)
        assert resp.status_code == 400
        assert_error_code(resp, "INVALID_INPUT")


# ── Login: brute-force lockout ────────────────────────────────────────────────

class TestLoginBruteForce:
    # kMaxFailAttempts = 5, kLockoutDuration = 5 min (fail_delay.hpp)
    MAX_ATTEMPTS = 5

    def test_lockout_after_max_failures(self):
        """After MAX_ATTEMPTS failures the next attempt must return 429."""
        for _ in range(self.MAX_ATTEMPTS):
            do_login(username=ADMIN_USER, password="bad_pass")

        resp = do_login(username=ADMIN_USER, password="bad_pass")
        assert resp.status_code == 429
        assert_error_code(resp, "TOO_MANY_REQUESTS")

    def test_lockout_blocks_correct_password(self):
        """Even the correct password is blocked during lockout."""
        for _ in range(self.MAX_ATTEMPTS):
            do_login(username=ADMIN_USER, password="bad_pass")

        resp = do_login(username=ADMIN_USER, password=ADMIN_PASS)
        assert resp.status_code == 429


# ── Logout ────────────────────────────────────────────────────────────────────

class TestLogout:
    def test_logout_returns_200(self, auth_session):
        session, csrf_token = auth_session
        resp = session.post(LOGOUT_URL, headers={"X-CSRF-Token": csrf_token})
        assert resp.status_code == 200

    def test_logout_clears_session_cookie(self, auth_session):
        session, csrf_token = auth_session
        resp = session.post(LOGOUT_URL, headers={"X-CSRF-Token": csrf_token})
        # Server must set Max-Age=0 or Expires=past to clear the cookie
        set_cookie = resp.headers.get("Set-Cookie", "")
        assert "session" in set_cookie
        assert "Max-Age=0" in set_cookie or "max-age=0" in set_cookie.lower()

    def test_logout_invalidates_session(self, auth_session):
        """After logout the old session cookie must no longer grant access."""
        session, csrf_token = auth_session
        session.post(LOGOUT_URL, headers={"X-CSRF-Token": csrf_token})

        # A subsequent protected request with the same cookie must fail
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
