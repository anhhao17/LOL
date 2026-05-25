# QUANTUM Architecture

## Overview

This document describes the architecture of the QUANTUM application — a C++ backend service with a Vue.js UI, modeled after the OpenBMC bmcweb pattern. Phase 1 delivers a secure login page and a real-time WebSocket streaming page.

---

## System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     Application Layer                        │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │   App Class  │  │   Config     │  │   Lifecycle  │      │
│  │              │  │   Manager    │  │   Manager    │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
│  ┌──────────────┐                                           │
│  │ Audit Logger │  ← structured access + security events   │
│  └──────────────┘                                           │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                 Static File Serving Layer                    │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  GET /ui/**  →  serve embedded or filesystem SPA     │  │
│  │  (Vue 3 build output, path-traversal protected)      │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                      Routing Layer                          │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │    Router    │  │     Trie     │  │   Rules      │      │
│  │              │  │  (URL Match) │  │              │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                    Middleware Layer                         │
│                                                             │
│  Request pipeline (in order):                              │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐      │
│  │   Auth   │→│   CSRF   │→│  Rate    │→│ Logging  │      │
│  │          │ │  Check   │ │  Limit   │ │          │      │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘      │
│  ┌──────────┐                                              │
│  │   CORS   │                                              │
│  └──────────┘                                              │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                      HTTP Layer                             │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │   AsyncResp  │  │   Request    │  │   Response   │      │
│  │              │  │   Wrapper    │  │   Wrapper    │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                      Server Layer                            │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │   HTTP       │  │ Connection   │  │   WebSocket  │      │
│  │   Server     │  │  Manager     │  │   Manager    │      │
│  │  (SSL/TLS)   │  │              │  │  (Security)  │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                      Session Layer                           │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │   Session    │  │   Cookie     │  │   Token      │      │
│  │   Store      │  │  Management  │  │  Generator   │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
│  ┌──────────────┐  ┌──────────────┐                        │
│  │  Password    │  │  Fail Delay  │  ← brute-force guard   │
│  │  Hasher      │  │  (backoff)   │                        │
│  │  (Argon2id)  │  └──────────────┘                        │
│  └──────────────┘                                           │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                       I/O Layer                              │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │  Boost::Asio │  │   Sockets    │  │     SSL      │      │
│  │  Event Loop  │  │   Manager    │  │   Support    │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
└─────────────────────────────────────────────────────────────┘
```

---

## Project Structure

```
Avenger/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── app/
│   │   ├── app.hpp / app.cpp              # Bootstrap, io_context, thread pool
│   │   └── config_manager.hpp / .cpp      # File + env + CLI args
│   ├── server/
│   │   ├── http_server.hpp / .cpp         # Acceptor, SSL context
│   │   ├── connection.hpp / .cpp          # Per-connection state machine
│   │   └── websocket_connection.hpp/.cpp  # HTTP→WS upgrade, frame I/O
│   ├── http/
│   │   ├── request.hpp                    # Parsed HTTP request wrapper
│   │   ├── response.hpp                   # Builder for HTTP responses
│   │   └── async_resp.hpp                 # RAII shared-ptr completion guard
│   ├── routing/
│   │   ├── router.hpp / .cpp              # Route registry + dispatch
│   │   └── trie.hpp / .cpp                # O(k) URL pattern matching
│   ├── middleware/
│   │   ├── i_middleware.hpp               # Interface: beforeRequest/afterResponse
│   │   ├── auth_middleware.hpp / .cpp     # Session token validation
│   │   ├── csrf_middleware.hpp / .cpp     # Double-submit cookie check
│   │   ├── rate_limit_middleware.hpp/.cpp # Per-IP sliding window counter
│   │   ├── cors_middleware.hpp / .cpp     # Strict origin allowlist
│   │   └── logging_middleware.hpp / .cpp  # Structured access log
│   ├── session/
│   │   ├── session_store.hpp / .cpp       # map<token, Session>, mutex-guarded
│   │   └── token_generator.hpp / .cpp     # CSPRNG 256-bit (OpenSSL RAND_bytes)
│   ├── auth/
│   │   ├── user_store.hpp / .cpp          # SQLite user database
│   │   ├── password_hasher.hpp / .cpp     # Argon2id via libsodium
│   │   └── fail_delay.hpp / .cpp          # Exponential back-off on login failure
│   ├── api/
│   │   ├── i_api_handler.hpp              # Interface: match() + handle()
│   │   ├── login_handler.hpp / .cpp       # POST /api/v1/login
│   │   ├── logout_handler.hpp / .cpp      # POST /api/v1/logout
│   │   └── static_handler.hpp / .cpp      # GET /ui/** → Vue SPA
│   └── streaming/
│       ├── websocket_handler.hpp / .cpp   # OnUpgrade/OnHandshake/OnClose
│       └── stream_manager.hpp / .cpp      # Per-view broadcast register
├── ui/                                    # Vue 3 + Vite + TypeScript
│   ├── src/
│   │   ├── views/
│   │   │   ├── LoginView.vue
│   │   │   └── StreamView.vue
│   │   ├── components/
│   │   │   ├── VideoCanvas.vue            # Renders binary frames from WS
│   │   │   └── StreamControls.vue         # View type selector
│   │   ├── store/
│   │   │   ├── auth.ts                    # Pinia: user, csrfToken, isAuthenticated
│   │   │   └── stream.ts                  # Pinia: WS state, selected view
│   │   ├── services/
│   │   │   ├── api.ts                     # Axios + CSRF header auto-attach
│   │   │   └── websocket.ts               # WS wrapper with reconnect back-off
│   │   ├── router/index.ts                # /login (public) + /stream (auth guard)
│   │   └── App.vue
│   ├── package.json
│   └── vite.config.ts
├── tests/
│   ├── unit/
│   └── integration/
└── docs/
    ├── ARCHITECTURE.md
    └── CODING_RULES.md
```

---

## Layer Responsibilities

### Application Layer
- **Purpose**: Main application coordination and lifecycle management
- **Responsibilities**:
  - Application initialization and shutdown
  - Configuration management
  - Component coordination
  - Route registration orchestration
  - Structured audit logging (auth events, access violations)

### Static File Serving Layer
- **Purpose**: Serve the Vue 3 SPA to the browser
- **Responsibilities**:
  - Serve pre-built `index.html` + JS/CSS bundles from `/ui/**`
  - Path-traversal protection (all paths resolved within web root)
  - Cache-Control headers for static assets
  - Fallback to `index.html` for Vue Router client-side routes

### Routing Layer
- **Purpose**: URL matching and request dispatching
- **Responsibilities**:
  - URL pattern matching using trie data structure — O(k) per path segment
  - Route handler registration
  - Path parameter extraction
  - HTTP method routing (GET, POST, PUT, DELETE, etc.)

### Middleware Layer
- **Purpose**: Cross-cutting concerns applied before and after every handler
- **Request pipeline order**: Auth → CSRF → Rate Limit → Logging → CORS
- **Responsibilities**:
  - **Auth**: validate session token; skip for `/api/v1/login`, `/ui/**`, `/health`
  - **CSRF**: for POST/PUT/DELETE, verify `X-CSRF-Token` header matches session value
  - **Rate Limit**: per-IP sliding window; reject with `429` on excess
  - **Logging**: structured access log (method, path, status, latency, client IP)
  - **CORS**: strict origin allowlist; reject mismatched origins

### HTTP Layer
- **Purpose**: HTTP abstraction and async response handling
- **Responsibilities**:
  - Request/response wrapping
  - Async operation coordination via `AsyncResp` RAII guard
  - JSON serialization/deserialization (nlohmann/json)
  - Header management and status codes

### Server Layer
- **Purpose**: Network communication and connection management
- **Responsibilities**:
  - HTTP/HTTPS server (Boost.Beast)
  - Connection lifecycle management
  - WebSocket upgrade handling with security validation
  - SSL/TLS (OpenSSL) for HTTPS and WSS on the same port
  - Connection pooling and resource limits

### Session Layer
- **Purpose**: User session management, authentication primitives
- **Responsibilities**:
  - Session creation and in-memory storage
  - `HttpOnly Secure SameSite=Strict` cookie management
  - CSPRNG token generation (OpenSSL `RAND_bytes`, 256-bit)
  - CSRF token generation (128-bit)
  - Session sliding expiry and cleanup
  - **Password Hasher**: Argon2id via libsodium for constant-time verification
  - **Fail Delay**: exponential back-off counter per IP after failed logins

### I/O Layer
- **Purpose**: Low-level async I/O and event loop
- **Responsibilities**:
  - Non-blocking async I/O (Boost.Asio)
  - Socket lifecycle
  - Event loop (multi-threaded `io_context`)
  - SSL/TLS encryption

---

## Request Flow

```
┌─────────────┐
│   Browser   │
└──────┬──────┘
       │ HTTPS Request
       ↓
┌─────────────────────────────────────────────────────────────┐
│                     I/O Layer                               │
│  Socket receives data → Boost::Asio event loop              │
└─────────────────────────────────────────────────────────────┘
       ↓
┌─────────────────────────────────────────────────────────────┐
│                   Server Layer                              │
│  Connection manager parses HTTP request (Boost.Beast)        │
└─────────────────────────────────────────────────────────────┘
       ↓
┌─────────────────────────────────────────────────────────────┐
│                   HTTP Layer                                │
│  Request wrapper created; AsyncResp initialized             │
└─────────────────────────────────────────────────────────────┘
       ↓
┌─────────────────────────────────────────────────────────────┐
│                Middleware Layer (Before)                     │
│  Auth → CSRF → Rate Limit → Logging → CORS                  │
└─────────────────────────────────────────────────────────────┘
       ↓
┌─────────────────────────────────────────────────────────────┐
│                  Routing Layer                              │
│  Trie matches URL → Route handler selected                  │
│  Path parameters extracted                                  │
└─────────────────────────────────────────────────────────────┘
       ↓
┌─────────────────────────────────────────────────────────────┐
│                Application Layer                            │
│  Handler executes business logic (async if needed)          │
└─────────────────────────────────────────────────────────────┘
       ↓ (response path)
┌─────────────────────────────────────────────────────────────┐
│                Middleware Layer (After)                      │
│  CORS → Logging → Security headers appended                 │
└─────────────────────────────────────────────────────────────┘
       ↓
┌─────────────────────────────────────────────────────────────┐
│             Server + I/O Layer (Return)                     │
│  Response serialized and written to TLS socket              │
└─────────────────────────────────────────────────────────────┘
       ↓
┌─────────────┐
│   Browser   │
└─────────────┘
```

---

## Phase 1 — Login Flow

```
Browser                              Backend
   │                                    │
   │── GET /ui/login ──────────────────▶│
   │◀── 200 index.html (Vue SPA) ───────│
   │                                    │
   │  [User fills form]                 │
   │                                    │
   │── POST /api/v1/login ─────────────▶│
   │   Content-Type: application/json   │
   │   { "username": "...",             │
   │     "password": "..." }            │
   │                                    ├─▶ rate_limit_middleware
   │                                    │     check per-IP bucket (sliding window)
   │                                    │     → 429 if exceeded
   │                                    │
   │                                    ├─▶ fail_delay
   │                                    │     exponential back-off if IP in lockout
   │                                    │     → 401 if locked
   │                                    │
   │                                    ├─▶ user_store.lookup(username)
   │                                    │     → 401 (generic) if not found
   │                                    │
   │                                    ├─▶ password_hasher.verify(input, stored_hash)
   │                                    │     constant-time Argon2id comparison
   │                                    │     → 401 (generic) if mismatch
   │                                    │
   │                                    ├─▶ session_store.create()
   │                                    │     token     = RAND_bytes(32) → hex
   │                                    │     csrfToken = RAND_bytes(16) → hex
   │                                    │     expiry    = now + 30 min (sliding)
   │                                    │
   │◀── 200 OK ─────────────────────────│
   │    Set-Cookie: session=<token>;    │
   │      HttpOnly; Secure;             │
   │      SameSite=Strict; Path=/       │
   │    { "csrfToken": "...",           │
   │      "username": "...",            │
   │      "role": 1 }                   │
   │                                    │
   │  [Vue Router → /stream]            │
```

---

## Phase 1 — Streaming Flow (WebSocket)

```
Browser                                    Backend
   │                                          │
   │── WS Upgrade GET /api/v1/stream ────────▶│
   │   Connection: Upgrade                    │  ← RFC 6455 required
   │   Upgrade: websocket                     │
   │   Sec-WebSocket-Key: <base64>            │
   │   Sec-WebSocket-Version: 13              │
   │   Sec-WebSocket-Protocol:                │
   │     view=cl_view, token=<session-token>  │
   │   Cookie: session=<token>                │
   │                                          │
   │                                   websocket_handler.OnUpgrade()
   │                                          │
   │                                          ├─▶ 1. Validate headers
   │                                          │      Upgrade: websocket ✓
   │                                          │      Connection: Upgrade ✓
   │                                          │      Sec-WebSocket-Key present ✓
   │                                          │      Sec-WebSocket-Version == 13 ✓
   │                                          │
   │                                          ├─▶ 2. Parse Sec-WebSocket-Protocol
   │                                          │      view=<type>  → validate type
   │                                          │      token=<val>  → extract token
   │                                          │
   │                                          ├─▶ 3. Validate token
   │                                          │      session_store.lookup(token)
   │                                          │      not expired, IP matches
   │                                          │      → 401 if invalid
   │                                          │
   │                                          ├─▶ 4. Check session capacity
   │                                          │      max concurrent WS per config
   │                                          │      → 503 if exceeded
   │                                          │
   │◀── 101 Switching Protocols ──────────────│
   │    Sec-WebSocket-Protocol: view=cl_view  │
   │                                          │
   │                                   stream_manager.register(con, VIEW_COLOR)
   │                                          │
   │◀── [binary frame] ───────────────────────│  timer-driven broadcast
   │◀── [binary frame] ───────────────────────│  (MainAppVideoGrabber pattern)
   │◀── [binary frame] ───────────────────────│
   │                                          │
   │── Close ────────────────────────────────▶│
   │                                   stream_manager.remove(con)
```

**Supported view types** (Sec-WebSocket-Protocol values):

| Value | Description |
|-------|-------------|
| `cl_view` | Color main stream |
| `ir_view` | Infrared main stream |
| `both_views` | Color + IR main stream |
| `cl_sub_view` | Color sub stream |
| `ir_sub_view` | Infrared sub stream |
| `both_sub_views` | Color + IR sub stream |

---

## Authentication Flow

```
┌─────────────┐
│   Client    │
└──────┬──────┘
       │ POST /api/v1/login
       ↓
┌─────────────────────────────────────────────────────────────┐
│                Middleware Layer                            │
│  Rate limit check → Fail delay check                        │
└─────────────────────────────────────────────────────────────┘
       ↓
┌─────────────────────────────────────────────────────────────┐
│                Auth Layer                                  │
│  user_store.lookup() → password_hasher.verify() (Argon2id)  │
└─────────────────────────────────────────────────────────────┘
       ↓
┌─────────────────────────────────────────────────────────────┐
│                Session Layer                               │
│  Session created: token (256-bit) + csrfToken (128-bit)     │
│  Stored in session_store (in-memory, mutex-guarded)         │
└─────────────────────────────────────────────────────────────┘
       ↓
┌─────────────────────────────────────────────────────────────┐
│                HTTP Layer (Return)                           │
│  Set-Cookie: session=<token>; HttpOnly; Secure; SameSite=Strict │
│  Body: { csrfToken, username, role }                        │
└─────────────────────────────────────────────────────────────┘
       ↓
┌─────────────┐
│   Client    │
└─────────────┘
```

---

## WebSocket Security Flow

```
┌─────────────┐
│   Client    │
└──────┬──────┘
       │ WebSocket Upgrade Request
       ↓
┌─────────────────────────────────────────────────────────────┐
│              WebSocket Security Validation                  │
│                                                             │
│  Step 1 — RFC 6455 Header Validation                        │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Upgrade: websocket              (required)          │  │
│  │  Connection: Upgrade             (RFC 6455 §4.1)     │  │
│  │  Sec-WebSocket-Key: <base64>     (required)          │  │
│  │  Sec-WebSocket-Version: 13       (must be 13)        │  │
│  └──────────────────────────────────────────────────────┘  │
│                                                             │
│  Step 2 — Protocol Header Validation                        │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Sec-WebSocket-Protocol: view=<type>, token=<value>  │  │
│  │  Valid types: cl_view | ir_view | both_views |        │  │
│  │               cl_sub_view | ir_sub_view |             │  │
│  │               both_sub_views                          │  │
│  └──────────────────────────────────────────────────────┘  │
│                                                             │
│  Step 3 — Authentication Validation                         │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Token source (in priority order):                    │  │
│  │    1. Sec-WebSocket-Protocol token=<value>            │  │
│  │    2. Cookie: session=<value>                         │  │
│  │  Validate: token exists in session_store              │  │
│  │  Validate: session not expired                        │  │
│  └──────────────────────────────────────────────────────┘  │
│                                                             │
│  Step 4 — Capacity Check                                    │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Active sessions < configured max → accept           │  │
│  │  Otherwise → 503 Service Unavailable                 │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
       ↓ (validation passed)
┌─────────────────────────────────────────────────────────────┐
│              WebSocket Connection Established              │
│  Session registered in StreamClientRegister                 │
│  Timer-driven frame broadcast begins                        │
└─────────────────────────────────────────────────────────────┘
```

---

## Security Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                   Security Layers                           │
│                                                             │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Transport: TLS 1.2+ (HTTPS/WSS), no SSLv2/SSLv3    │  │
│  └──────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Auth: Argon2id password hashing (libsodium)         │  │
│  │        constant-time verify — no timing oracle       │  │
│  └──────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Session: CSPRNG 256-bit token (OpenSSL RAND_bytes)  │  │
│  │           HttpOnly; Secure; SameSite=Strict cookie   │  │
│  └──────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  CSRF: double-submit — CSRF token in response body   │  │
│  │         X-CSRF-Token header required for mutations   │  │
│  └──────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Rate Limit: per-IP sliding window (configurable)    │  │
│  │  Fail Delay: exponential back-off on login failures  │  │
│  └──────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  WebSocket: protocol header + session token check    │  │
│  │             capacity limit enforced                  │  │
│  └──────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Headers: Content-Security-Policy, X-Frame-Options,  │  │
│  │           X-Content-Type-Options, Referrer-Policy    │  │
│  └──────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Static files: path traversal protection             │  │
│  │  Errors: generic messages — no field enumeration     │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### Threat Model Summary

| Threat | Mitigation |
|--------|-----------|
| Credential brute force | `FailDelay` exponential back-off + per-IP rate limiter |
| Password storage compromise | Argon2id with random salt (`crypto_pwhash`) |
| Session hijacking | 256-bit CSPRNG token, `HttpOnly Secure SameSite=Strict` cookie |
| CSRF attacks | Double-submit: CSRF token in body + `X-CSRF-Token` header check |
| XSS → token theft | `HttpOnly` cookie (JS cannot read); CSP header blocks inline scripts |
| WebSocket hijacking | Token validated server-side on every upgrade |
| Man-in-the-middle | TLS mandatory; HTTP → HTTPS redirect in production |
| Timing oracle on login | Constant-time Argon2id verification |
| Directory traversal | Static handler validates all paths within web root |
| Username enumeration | Login always returns the same generic error |
| Clickjacking | `X-Frame-Options: DENY` response header |

---

## Component Interactions

### Async Operation Flow

```
┌──────────────┐
│   Handler    │
└──────┬───────┘
       │
       ↓
┌─────────────────────────────────────────────────────────────┐
│                  AsyncResp (RAII guard)                     │
│  Completion handler registered                              │
│  Async operation initiated (DB, file, timer, etc.)          │
└─────────────────────────────────────────────────────────────┘
       ↓
┌──────────────────────────────────────────────────────┐
│         External Async Operation                      │
│  Operation completes → Callback invoked              │
└──────────────────────────────────────────────────────┘
       ↓
┌─────────────────────────────────────────────────────────────┐
│                  AsyncResp (Complete)                       │
│  Response populated → destructor triggers send             │
└─────────────────────────────────────────────────────────────┘
```

### Middleware Pipeline Flow

```
┌──────────────┐
│   Request    │
└──────┬───────┘
       ↓
┌─────────────────────────────────────────────────────────────┐
│              Middleware Chain (Before)                       │
│  ┌──────┐  ┌──────┐  ┌────────────┐  ┌─────────┐  ┌──────┐ │
│  │ Auth │→ │ CSRF │→ │ Rate Limit │→ │ Logging │→ │ CORS │ │
│  └──────┘  └──────┘  └────────────┘  └─────────┘  └──────┘ │
└─────────────────────────────────────────────────────────────┘
       ↓
┌──────────────┐
│   Handler    │
└──────┬───────┘
       ↓
┌─────────────────────────────────────────────────────────────┐
│              Middleware Chain (After)                        │
│  ┌──────┐  ┌─────────┐  ┌───────────────┐                  │
│  │ CORS │→ │ Logging │→ │ Sec Headers   │                  │
│  └──────┘  └─────────┘  └───────────────┘                  │
└─────────────────────────────────────────────────────────────┘
       ↓
┌──────────────┐
│  Response    │
└──────────────┘
```

---

## Vue Frontend Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                   Vue 3 SPA (Vite + TypeScript)             │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │                  Vue Router                          │   │
│  │  /login  → LoginView   (public)                     │   │
│  │  /stream → StreamView  (auth guard: redirect login) │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                             │
│  ┌──────────────────┐    ┌──────────────────────────────┐  │
│  │    LoginView     │    │         StreamView           │  │
│  │                  │    │                              │  │
│  │  Username field  │    │  ┌────────────────────────┐  │  │
│  │  Password field  │    │  │     VideoCanvas.vue     │  │  │
│  │  Submit button   │    │  │  (binary frame → blob  │  │  │
│  │                  │    │  │   URL → <img>)         │  │  │
│  │  On success:     │    │  └────────────────────────┘  │  │
│  │  → store csrf    │    │  ┌────────────────────────┐  │  │
│  │  → route /stream │    │  │   StreamControls.vue   │  │  │
│  └──────────────────┘    │  │  cl_view | ir_view |   │  │  │
│                          │  │  both_views | sub_*    │  │  │
│  ┌──────────────────────────────────────────────────┐  │  │
│  │                  Pinia Stores                    │  │  │
│  │  auth.ts: { username, role, csrfToken, isAuthed }│  │  │
│  │  stream.ts: { wsState, selectedView, frameRate } │  │  │
│  └──────────────────────────────────────────────────┘  │  │
│                                                         │  │
│  ┌──────────────────────────────────────────────────┐  │  │
│  │                  Services                        │  │  │
│  │  api.ts       Axios, auto X-CSRF-Token header    │  │  │
│  │  websocket.ts WS client, reconnect with back-off │  │  │
│  └──────────────────────────────────────────────────┘  │  │
└─────────────────────────────────────────────────────────────┘
```

---

## Data Flow Patterns

### Synchronous Request
```
Client → I/O → Server → HTTP → Middleware → Routing → Handler →
Middleware → HTTP → Server → I/O → Client
```

### Asynchronous Request
```
Client → I/O → Server → HTTP → Middleware → Routing → Handler →
AsyncResp → External Op → Callback → AsyncResp complete →
Middleware → HTTP → Server → I/O → Client
```

### WebSocket Upgrade
```
Client → I/O → Server → HTTP → Middleware → Routing →
WebSocket Handler (security validation) →
WebSocket Connection Established →
Persistent binary frame broadcast
```

---

## Performance Considerations

### Async-First Design
- All I/O operations are non-blocking (Boost.Asio strands)
- Multi-threaded `io_context` (configurable thread count)
- No blocking calls on the event loop

### Efficient Routing
- Trie-based URL matching — O(k) per path segment, k = segment count
- Path parameter extraction without regex overhead

### Memory Management
- Smart pointers throughout (no raw `new`/`delete`)
- RAII for all resources (sockets, DB handles, locks)
- `AsyncResp` shared ownership ensures response lifetime is correct

---

## Configuration Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                   Configuration Sources                      │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │   Config     │  │ Environment  │  │   Command    │      │
│  │   File       │  │   Variables  │  │   Line Args  │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                   Config Manager                             │
│  Validation → Merging → Runtime Access (read-only after init)│
└─────────────────────────────────────────────────────────────┘
```

### CLI Arguments

| Flag | Default | Description |
|------|---------|-------------|
| `--ssl, -s` | off | Enable SSL/TLS (switches default port to 8443) |
| `--cert <file>` | — | SSL certificate file path |
| `--key <file>` | — | SSL private key file path |
| `--port <port>` | 8080 / 8443 | Listening port |
| `--web-root <dir>` | `./ui/dist` | Path to Vue SPA build output |
| `--max-ws-sessions <n>` | 6 | Maximum concurrent WebSocket streaming sessions |
| `--help, -h` | — | Show help |

---

## Error Handling Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                   Error Handling Flow                        │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │   Handler    │→ │   AsyncResp  │→ │   Response   │      │
│  │   Exception  │  │   Error CB   │  │   Status     │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                   Error Response Rules                       │
│  Auth failures    → 401 with generic "invalid credentials"  │
│  Rate exceeded    → 429 with Retry-After header             │
│  Not found        → 404 (no path enumeration)               │
│  Server error     → 500 (no internal detail to client)      │
│  All errors logged with context (server-side only)          │
└─────────────────────────────────────────────────────────────┘
```

---

## Monitoring and Observability

```
┌─────────────────────────────────────────────────────────────┐
│                   Monitoring Layers                           │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Security: auth failures, token validation, lockouts │  │
│  └──────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  HTTP: method, path, status code, latency, client IP │  │
│  └──────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  WebSocket: active sessions, upgrade success/fail    │  │
│  └──────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Server: connection count, resource usage            │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### Logging Levels

| Level | Usage |
|-------|-------|
| `TRACE` | Frame-level WS events (dev only) |
| `DEBUG` | Request routing, session ops |
| `INFO` | Startup, login success, WS connections |
| `WARN` | Rate limit hits, auth failures |
| `ERROR` | Handler exceptions, DB errors |
| `CRITICAL` | Service cannot continue |

### Log Outputs
- **Console**: structured format with timestamp, level, source file, message
- **File**: persistent `quantum.log` with rotation
- **Audit CSV**: security events (login success/fail, session create/revoke)

---

## Dependencies

### Backend (C++)

| Library | Version | Purpose |
|---------|---------|---------|
| Boost.Asio / Beast | 1.82+ | Async I/O, HTTP, WebSocket |
| OpenSSL | 3.x | TLS, CSPRNG |
| libsodium | 1.0.18+ | Argon2id password hashing |
| SQLite3 | 3.x | User database |
| nlohmann/json | 3.x | JSON parsing |
| spdlog | 1.x | Structured logging |
| {fmt} | 10.x | String formatting |

### Frontend (Vue 3)

| Library | Purpose |
|---------|---------|
| Vue 3 + Vite | SPA framework + build tooling |
| TypeScript | Type safety |
| Pinia | State management (auth, stream) |
| Vue Router 4 | Client-side routing with auth guard |
| Axios | HTTP client with interceptors |

---

## Scalability Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                   Horizontal Scaling                         │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │  Instance 1  │  │  Instance 2  │  │  Instance N  │      │
│  │  (Port 8443) │  │  (Port 8443) │  │  (Port 8443) │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                   Load Balancer (TLS termination)            │
│  Session affinity required (in-memory session store)         │
│  Or: externalize session store (Redis) for stateless scale   │
└─────────────────────────────────────────────────────────────┘
```

> **Note**: HTTP and WebSocket share the same port. WebSocket uses HTTP upgrade — no separate port needed.
