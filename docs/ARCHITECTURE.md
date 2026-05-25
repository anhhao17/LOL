# QUANTUM Architecture

## Overview

This document describes the architecture of the QUANTUM minimal implementation, following proven patterns from the OpenBMC bmcweb project.

## System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     Application Layer                        │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │   App Class  │  │   Config     │  │   Lifecycle  │      │
│  │              │  │   Manager    │  │   Manager    │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
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
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │     Auth     │  │   Logging    │  │     CORS     │      │
│  │  Middleware  │  │  Middleware  │  │  Middleware  │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
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
│  │   Server     │  │  Manager     │  │   Support    │      │
│  │  (SSL/TLS)   │  │              │  │  (Security)  │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                      Session Layer                           │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │   Session    │  │   Cookie     │  │   Token      │      │
│  │   Manager    │  │  Management  │  │  Validation  │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
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

## Layer Responsibilities

### Application Layer
- **Purpose**: Main application coordination and lifecycle management
- **Responsibilities**:
  - Application initialization and shutdown
  - Configuration management
  - Component coordination
  - Route registration orchestration

### Routing Layer
- **Purpose**: URL matching and request dispatching
- **Responsibilities**:
  - URL pattern matching using trie data structure
  - Route handler registration
  - Path parameter extraction
  - HTTP method routing (GET, POST, PUT, DELETE, etc.)

### Middleware Layer
- **Purpose**: Cross-cutting concerns and request processing pipeline
- **Responsibilities**:
  - Authentication and authorization
  - Request/response logging
  - CORS handling
  - Rate limiting
  - Request validation

### HTTP Layer
- **Purpose**: HTTP abstraction and async response handling
- **Responsibilities**:
  - Request/response wrapping
  - Async operation coordination
  - JSON serialization/deserialization
  - Header management
  - Status code handling

### Server Layer
- **Purpose**: Network communication and connection management
- **Responsibilities**:
  - HTTP server implementation
  - Connection lifecycle management
  - WebSocket upgrade handling with security validation
  - SSL/TLS support for HTTPS/WSS
  - Connection pooling
  - Dual-mode HTTP/HTTPS and WS/WSS support

### Session Layer
- **Purpose**: User session management and authentication
- **Responsibilities**:
  - Session creation and validation
  - Cookie-based session management
  - Token generation and validation
  - Session lifecycle management
  - CSRF token generation
  - Session cleanup and expiration

### I/O Layer
- **Purpose**: Low-level I/O operations and event loop
- **Responsibilities**:
  - Async I/O operations
  - Socket management
  - Event loop coordination
  - SSL/TLS encryption
  - Network protocol handling

## Request Flow

```
┌─────────────┐
│   Client    │
└──────┬──────┘
       │ HTTP Request
       ↓
┌─────────────────────────────────────────────────────────────┐
│                     I/O Layer                               │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Socket receives data → Boost::Asio event loop      │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
       ↓
┌─────────────────────────────────────────────────────────────┐
│                   Server Layer                              │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Connection manager parses HTTP request              │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
       ↓
┌─────────────────────────────────────────────────────────────┐
│                   HTTP Layer                                │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Request wrapper created                              │  │
│  │  AsyncResp wrapper initialized                        │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
       ↓
┌─────────────────────────────────────────────────────────────┐
│                Middleware Layer                            │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Auth middleware → Logging middleware → CORS         │  │
│  │  (Request processing pipeline)                        │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
       ↓
┌─────────────────────────────────────────────────────────────┐
│                  Routing Layer                              │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Trie matches URL pattern → Route handler selected   │  │
│  │  Path parameters extracted                           │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
       ↓
┌─────────────────────────────────────────────────────────────┐
│                Application Layer                            │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Route handler executes business logic               │  │
│  │  Async operations performed (DBus, file I/O, etc.)   │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
       ↓
┌─────────────────────────────────────────────────────────────┐
│                   HTTP Layer (Return)                        │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Response populated with data/status                 │  │
│  │  AsyncResp completion handler triggered              │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
       ↓
┌─────────────────────────────────────────────────────────────┐
│                  Middleware Layer (Return)                   │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Response logging → Header modification              │  │
│  │  (Response processing pipeline)                      │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
       ↓
┌─────────────────────────────────────────────────────────────┐
│                   Server Layer (Return)                      │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Response serialized and sent over socket            │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
       ↓
┌─────────────────────────────────────────────────────────────┐
│                     I/O Layer (Return)                      │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Data written to socket via Boost::Asio               │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
       ↓
┌─────────────┐
│   Client    │
└─────────────┘
```

## WebSocket Security Flow

```
┌─────────────┐
│   Client    │
└──────┬──────┘
       │ WebSocket Upgrade Request
       ↓
┌─────────────────────────────────────────────────────────────┐
│                   Server Layer                              │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Detect WebSocket upgrade headers                    │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
       ↓
┌─────────────────────────────────────────────────────────────┐
│              WebSocket Security Validation                 │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  1. Validate WebSocket headers                      │  │
│  │     - Upgrade: websocket                              │  │
│  │     - Connection: keep-alive                           │  │
│  │     - Sec-WebSocket-Key: present                      │  │
│  │     - Sec-WebSocket-Version: 13                        │  │
│  └──────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  2. Validate WebSocket protocol                       │  │
│  │     - Sec-WebSocket-Protocol header                   │  │
│  │     - Required: view=type, token=value               │  │
│  │     - Valid view types: cl_view, ir_view, etc.        │  │
│  └──────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  3. Validate authentication token                     │  │
│  │     - Check Authorization header (Bearer token)        │  │
│  │     - Check Cookie header (session_token)             │  │
│  │     - Validate against session store                 │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
       ↓ (if validation passes)
┌─────────────────────────────────────────────────────────────┐
│              WebSocket Connection Established              │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  WebSocket session created                            │  │
│  │  Session ID generated                                 │  │
│  │  Added to WebSocket manager                           │  │
│  │  Real-time sensor streaming enabled                    │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
       ↓
┌─────────────┐
│   Client    │
└─────────────┘
```

## Component Interactions

### Route Registration Flow

```
┌──────────────┐
│ Application  │
│   Startup    │
└──────┬───────┘
       │
       ↓
┌─────────────────────────────────────────────────────────────┐
│                    Router                                  │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Route registration (URL pattern + handler)          │  │
│  └──────────────────────────────────────────────────────┘  │
       ↓
┌──────────────────────────────────────────────────────┐
│            Trie (URL Pattern Storage)                 │
│  ┌────────────────────────────────────────────────┐  │
│  │  Pattern added to trie structure               │  │
│  │  O(1) lookup capability established          │  │
│  └────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────┘
```

### Async Operation Flow

```
┌──────────────┐
│   Handler    │
└──────┬───────┘
       │
       ↓
┌─────────────────────────────────────────────────────────────┐
│                  AsyncResp                                 │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Completion handler registered                       │  │
│  │  Async operation initiated (DBus, file, etc.)         │  │
│  └──────────────────────────────────────────────────────┘  │
       ↓
┌──────────────────────────────────────────────────────┐
│         External Async Operation                      │
│  ┌────────────────────────────────────────────────┐  │
│  │  Operation completes → Callback invoked          │  │
│  └────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────┘
       ↓
┌─────────────────────────────────────────────────────────────┐
│                  AsyncResp (Complete)                       │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Response populated                                  │  │
│  │  Completion handler triggered                        │  │
│  └──────────────────────────────────────────────────────┘  │
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
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐  │
│  │   Auth   │→ │  Logging │→ │   CORS   │→ │  Validate│  │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘  │
└─────────────────────────────────────────────────────────────┘
       ↓
┌──────────────┐
│   Handler    │
└──────┬───────┘
       ↓
┌─────────────────────────────────────────────────────────────┐
│              Middleware Chain (After)                        │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐  │
│  │ Validate │→ │   CORS   │→ │  Logging │→ │   Auth   │  │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘  │
└─────────────────────────────────────────────────────────────┘
       ↓
┌──────────────┐
│  Response    │
└──────────────┘
```

## Data Flow Patterns

### Synchronous Request Flow
```
Client → I/O → Server → HTTP → Middleware → Routing → Handler → 
Middleware → HTTP → Server → I/O → Client
```

### Asynchronous Request Flow
```
Client → I/O → Server → HTTP → Middleware → Routing → Handler → 
AsyncResp → External Operation → Callback → AsyncResp → 
Middleware → HTTP → Server → I/O → Client
```

### WebSocket Upgrade Flow
```
Client → I/O → Server → HTTP → Middleware → Routing → 
WebSocket Handler → WebSocket Connection → 
Persistent WebSocket Communication
```

## Security Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                   Security Layers                           │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Application: Route-level access control               │  │
│  └──────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Middleware: Session-based authentication              │  │
│  └──────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Server: SSL/TLS encryption (HTTPS/WSS)                │  │
│  └──────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  WebSocket: Header/protocol/token validation        │  │
│  └──────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  I/O: Secure socket handling                         │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### Authentication Flow

```
┌─────────────┐
│   Client    │
└──────┬──────┘
       │ POST /api/login
       ↓
┌─────────────────────────────────────────────────────────────┐
│                Middleware Layer                            │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Auth middleware validates credentials                 │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
       ↓
┌─────────────────────────────────────────────────────────────┐
│                Session Layer                               │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Session created with unique token                    │  │
│  │  CSRF token generated                                │  │
│  │  Session stored in session store                     │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
       ↓
┌─────────────────────────────────────────────────────────────┐
│                   HTTP Layer (Return)                        │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Response with session token + CSRF token              │  │
│  │  Set-Cookie header for session persistence              │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
       ↓
┌─────────────┐
│   Client    │
└─────────────┘
```

### WebSocket Security Details

**Header Validation:**
- `Upgrade: websocket` - Must be present and correct
- `Connection: keep-alive` - Required for persistent connections
- `Sec-WebSocket-Key: <base64>` - Required for handshake
- `Sec-WebSocket-Version: 13` - Must be version 13

**Protocol Validation:**
- `Sec-WebSocket-Protocol: view=<type>, token=<value>` - Required
- Valid view types: `cl_view`, `ir_view`, `both_views`, `cl_sub_view`, `ir_sub_view`, `both_sub_views`
- Token must be valid session token

**Authentication Validation:**
- Check `Authorization: Bearer <token>` header
- Check `Cookie: session_token=<token>` header
- Validate token against session store
- Reject if token invalid or missing

## Performance Considerations

### Async-First Design
- All I/O operations are non-blocking
- Event loop prevents thread blocking
- High concurrency with minimal threads

### Efficient Routing
- Trie-based O(1) URL matching
- No regex overhead for static routes
- Path parameter extraction without parsing

### Memory Management
- Smart pointers for resource management
- Connection pooling
- Efficient buffer management

## Scalability Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                   Horizontal Scaling                         │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │  Instance 1  │  │  Instance 2  │  │  Instance N  │      │
│  │  (Port 8080) │  │  (Port 8080) │  │  (Port 8080) │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                   Load Balancer                              │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Round-robin or health-based distribution             │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

**Note:** All instances use the same port (8080) for both HTTP and WebSocket connections. WebSocket uses HTTP upgrade on the same port, not a separate port.

## Error Handling Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                   Error Handling Flow                        │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │   Handler    │→ │   AsyncResp  │→ │   Response   │      │
│  │   Exception  │  │   Error      │  │   Status      │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                   Error Logging                              │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Structured error logging with context               │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

## Configuration Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                   Configuration Sources                       │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │   Config     │  │ Environment  │  │   Command    │      │
│  │   File       │  │   Variables  │  │   Line Args  │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                   Configuration Manager                      │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Validation → Merging → Runtime Access               │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### Server Configuration Options

**Command Line Arguments:**
- `--ssl, -s` - Enable SSL/TLS (default port: 8443)
- `--cert <file>` - SSL certificate file path
- `--key <file>` - SSL private key file path
- `--port <port>` - Server port (default: 8080, 8443 with SSL)
- `--help, -h` - Show help message

**SSL/TLS Configuration:**
- Uses OpenSSL for SSL/TLS support
- Supports both HTTP/HTTPS and WS/WSS on same port
- Configurable certificate and key files
- SSL context with security options:
  - SSLv23/TLS support
  - No SSLv2 (deprecated)
  - Single DH use
  - Certificate validation

**Session Configuration:**
- Session tokens with unique identifiers
- CSRF tokens for form protection
- Cookie-based session persistence
- Session expiration and cleanup

## Monitoring and Observability

```
┌─────────────────────────────────────────────────────────────┐
│                   Monitoring Layers                           │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Application: Business metrics, custom events         │  │
│  └──────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  HTTP: Request/response metrics, error rates         │  │
│  └──────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  WebSocket: Connection metrics, message rates        │  │
│  └──────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Server: Connection metrics, resource usage          │  │
│  └──────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  I/O: Socket metrics, async operation timing        │  │
│  └──────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Security: Authentication events, access logs        │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### Logging Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                   Logging Levels                              │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │   TRACE      │  │   DEBUG      │  │   INFO       │      │
│  │  (Detailed)  │  │  (Diag)      │  │  (General)   │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │   WARN       │  │   ERROR      │  │  CRITICAL    │      │
│  │  (Warnings)  │  │  (Errors)    │  │  (Critical)  │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                   Logging Outputs                            │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Console: Structured logging with source location     │  │
│  └──────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  File: Persistent log file (jetson.log)               │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### WebSocket Monitoring

**Connection Metrics:**
- Active WebSocket connections count
- Connection success/failure rates
- WebSocket upgrade success rates
- Session ID tracking

**Security Metrics:**
- Authentication failures
- Protocol validation failures
- Token validation failures
- Unauthorized connection attempts

**Performance Metrics:**
- Message throughput
- Latency measurements
- Connection duration
- Memory usage per connection