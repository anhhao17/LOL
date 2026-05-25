"""
Brute-force / lockout tests for POST /api/v1/login.

MUST run after all other test files because triggering the lockout
blocks ALL login attempts from 127.0.0.1 for 5 minutes (kLockoutDuration),
which would cause every subsequent auth_session fixture to fail.

In CI this file is listed last in the pytest invocation.
Locally: pytest test_auth.py test_middleware.py ... test_bruteforce.py

Server constants (fail_delay.hpp):
  kMaxFailAttempts = 5
  kLockoutDuration = 5 minutes
"""

import pytest
from conftest import ADMIN_USER, ADMIN_PASS, do_login, assert_error_code

MAX_ATTEMPTS = 5  # kMaxFailAttempts


class TestLoginBruteForce:
    def test_lockout_after_max_failures(self):
        """After MAX_ATTEMPTS bad logins the next attempt must return 429."""
        for _ in range(MAX_ATTEMPTS):
            do_login(username=ADMIN_USER, password="bad_pass_lockout_test")

        resp = do_login(username=ADMIN_USER, password="bad_pass_lockout_test")
        assert resp.status_code == 429
        assert_error_code(resp, "TOO_MANY_REQUESTS")

    def test_lockout_blocks_correct_password(self):
        """Even the correct password is blocked while lockout is active."""
        # IP is already locked from the previous test in this class.
        resp = do_login(username=ADMIN_USER, password=ADMIN_PASS)
        assert resp.status_code == 429
