# Industrial Remote Expert System - Design Document

## System Architecture

### Overview
The Industrial Remote Expert System is built using a client-server architecture with Qt/C++ for native performance and cross-platform compatibility. The system emphasizes reliability, security, and real-time communication for industrial environments.

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│  Factory Client │    │  Expert Client  │    │  Mobile Client  │
│   (Qt Widgets)  │    │   (Qt Widgets)  │    │ (Qt for Mobile) │
└─────────┬───────┘    └─────────┬───────┘    └─────────┬───────┘
          │                      │                      │
          └──────────────────────┼──────────────────────┘
                                 │
                    ┌────────────┴────────────┐
                    │     Network Layer       │
                    │  (Enhanced Protocol)    │
                    └────────────┬────────────┘
                                 │
              ┌──────────────────┴──────────────────┐
              │           Server (RoomHub)          │
              │  ┌─────────────┐ ┌─────────────┐   │
              │  │ Room Manager│ │   Database  │   │
              │  └─────────────┘ └─────────────┘   │
              │  ┌─────────────┐ ┌─────────────┐   │
              │  │Device Manager│ │  Recording  │   │
              │  └─────────────┘ └─────────────┘   │
              └─────────────────────────────────────┘
                                 │
          ┌──────────────────────┼──────────────────────┐
          │                      │                      │
    ┌─────┴─────┐         ┌─────┴─────┐         ┌─────┴─────┐
    │  Serial   │         │ CAN Bus   │         │  Modbus   │
    │ Devices   │         │ Devices   │         │ Devices   │
    └───────────┘         └───────────┘         └───────────┘
```

## Core Components

### 1. Enhanced Protocol Layer

#### Frame Structure
```cpp
struct FrameHeader {
    quint32 magic;          // 'REXP' magic number
    quint16 version;        // Protocol version (1)
    quint16 msgType;        // Message type enum
    quint16 flags;          // Control flags
    quint16 reserved;       // Future use
    quint32 length;         // Total frame length
    char roomId[16];        // Room identifier
    char senderId[16];      // Sender identifier
    quint64 timestampMs;    // Timestamp
    quint32 seq;            // Sequence number
    quint32 jsonSize;       // JSON payload size
    // Followed by: [jsonPayload][binaryPayload]
} __attribute__((packed));
```

#### Message Types
- **Session Management**: REGISTER, LOGIN, JOIN_WORKORDER, LEAVE_WORKORDER
- **Communication**: TEXT, AUDIO_FRAME, VIDEO_FRAME
- **Device Control**: DEVICE_DATA, CONTROL_CMD, DEVICE_STATUS
- **Protocol Control**: HEARTBEAT, ACK, NACK, ERROR
- **Room Management**: ROOM_MEMBER_JOIN, ROOM_MEMBER_LEAVE, ROOM_STATE

#### Protocol Features
- **Validation**: Magic number, version, and size validation
- **Routing**: Room-based message routing with sender identification
- **Reliability**: Sequence numbers, ACK/NACK, and retransmission
- **Security**: Frame flags for encryption and compression
- **Performance**: Binary encoding with compact JSON payloads

### 2. Server Architecture (RoomHub)

#### Core Services
```cpp
class RoomHub {
private:
    QTcpServer server_;           // Network listener
    DatabaseManager db_;          // SQLite persistence
    RateLimiter rateLimiter_;     // Rate limiting
    QHash<QTcpSocket*, ClientCtx*> clients_;  // Client connections
    QMultiHash<QString, QTcpSocket*> rooms_;  // Room memberships
};
```

#### Client Context
```cpp
struct ClientCtx {
    QTcpSocket* sock;           // Network connection
    QString userId;             // User identifier
    QString roomId;             // Current room
    QString clientInfo;         // Connection metadata
    QDateTime lastHeartbeat;    // Heartbeat tracking
    QTimer* heartbeatTimer;     // Timeout management
    qint64 bytesReceived;       // Rate limiting stats
    qint64 messagesReceived;    // Rate limiting stats
    QDateTime connectionTime;   // Connection timestamp
    bool isAuthenticated;       // Auth status
};
```

#### Threading Model
- **Main Thread**: Network I/O, message routing, client management
- **Database Thread**: SQLite operations (via Qt's SQL connection pooling)
- **Timer Thread**: Heartbeat and cleanup operations

### 3. Client Architecture (Enhanced ClientConn)

#### Connection Management
```cpp
class ClientConn {
private:
    QTcpSocket* socket_;           // Network connection
    ConnectionState state_;        // Connection state
    QTimer* heartbeatTimer_;       // Heartbeat sender
    QTimer* heartbeatTimeoutTimer_; // Heartbeat timeout
    QTimer* reconnectTimer_;       // Auto-reconnect
    QQueue<QueuedMessage> sendQueue_; // Message queue
    NetworkQuality networkQuality_; // Connection quality
};
```

#### State Machine
```
Disconnected ──connect()──→ Connecting ──success──→ Connected
     ↑                           │                     │
     │                        failure                  │
     │                           ↓                     │
     └──────────────── Reconnecting ←──────────────────┘
                           ↑
                           │
                        Error
```

#### Message Queue
- **Backpressure**: Configurable queue size with overflow handling
- **Prioritization**: Heartbeat and control messages take priority
- **Persistence**: Queue survives connection drops during reconnect
- **Age Management**: Automatic removal of stale messages

### 4. Database Schema

#### Core Tables
```sql
-- User management
CREATE TABLE users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT UNIQUE NOT NULL,
    email TEXT,
    password_hash TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    last_login_at DATETIME,
    is_active BOOLEAN DEFAULT 1
);

-- Work order tracking
CREATE TABLE workorders (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    title TEXT NOT NULL,
    description TEXT,
    status TEXT DEFAULT 'created',
    created_by_user_id INTEGER,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    metadata TEXT,
    FOREIGN KEY (created_by_user_id) REFERENCES users(id)
);

-- Session tracking
CREATE TABLE sessions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    room_id TEXT NOT NULL,
    user_id TEXT NOT NULL,
    joined_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    left_at DATETIME,
    client_info TEXT
);

-- Message persistence
CREATE TABLE messages (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    room_id TEXT NOT NULL,
    sender_id TEXT NOT NULL,
    message_type INTEGER NOT NULL,
    json_payload TEXT,
    binary_payload BLOB,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    sequence_number INTEGER
);

-- Recording index
CREATE TABLE recordings (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    room_id TEXT NOT NULL,
    filename TEXT NOT NULL,
    type TEXT DEFAULT 'messages',
    file_size INTEGER DEFAULT 0,
    start_time DATETIME,
    end_time DATETIME,
    metadata TEXT
);
```

#### Indexing Strategy
```sql
-- Performance indexes
CREATE INDEX idx_sessions_room_id ON sessions(room_id);
CREATE INDEX idx_sessions_user_id ON sessions(user_id);
CREATE INDEX idx_messages_room_id ON messages(room_id);
CREATE INDEX idx_messages_timestamp ON messages(timestamp);
CREATE INDEX idx_recordings_room_id ON recordings(room_id);
CREATE INDEX idx_workorders_status ON workorders(status);
```

### 5. Device Data Architecture

#### Interface Hierarchy
```cpp
class IDeviceSource {
public:
    virtual bool configure(const QJsonObject& config) = 0;
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual QString getDeviceId() const = 0;
    virtual QStringList getAvailableMetrics() const = 0;
signals:
    void sampleReady(const DeviceSample& sample);
    void error(const QString& message);
};

class DeviceDataSimulator : public IDeviceSource;
class SerialPortSource : public IDeviceSource;
class SocketCanSource : public IDeviceSource;  // Linux only
```

#### Data Flow
```
Device Sources → DeviceManager → RoomHub → Clients
                      ↓
               RecordingManager → JSONL Files
                      ↓
               DatabaseManager → SQLite
```

#### Sample Format
```cpp
struct DeviceSample {
    QString deviceId;       // Device identifier
    QString metricName;     // Metric being measured
    QVariant value;         // Measured value
    QString unit;          // Unit of measurement
    QDateTime timestamp;   // Sample timestamp
    QJsonObject metadata;  // Additional context
};
```

### 6. Recording Architecture

#### Recording Types
- **Messages**: All text communication and system events
- **DeviceData**: Sensor readings and device status
- **AudioVideo**: Future A/V streams (planned)

#### File Format (JSONL)
```json
{"type":"header","version":"1.0","sessionId":"uuid","roomId":"R123","startTime":"2024-01-01T00:00:00"}
{"type":"message","timestamp":1234567890,"messageType":20,"roomId":"R123","senderId":"user1","json":{...}}
{"type":"device_sample","timestamp":1234567890,"sample":{"deviceId":"SIM001","metric":"temperature",...}}
{"type":"footer","endTime":"2024-01-01T01:00:00","itemCount":1000,"fileSize":102400,"duration":3600000}
```

#### Storage Management
- **File Rotation**: Automatic rotation when size limits are exceeded
- **Compression**: Optional gzip compression for long-term storage
- **Cleanup**: Configurable retention policies
- **Indexing**: SQLite index for fast recording retrieval

## Security Architecture

### Current Implementation
- **Transport Security**: TLS support prepared (configurable)
- **Frame Validation**: Magic numbers, size limits, format validation
- **Rate Limiting**: Per-client request rate limiting
- **Input Sanitization**: JSON payload validation

### Planned Security Features
- **Authentication**: Username/password with bcrypt hashing
- **Authorization**: Role-based access control (RBAC)
- **Encryption**: End-to-end encryption for sensitive data
- **Audit Logging**: Security event tracking
- **Certificate Management**: TLS certificate lifecycle

## Performance Characteristics

### Scalability Targets
- **Concurrent Users**: 1000+ simultaneous connections
- **Message Throughput**: 10,000+ messages/second
- **Device Data Rate**: 100,000+ samples/second
- **Database Performance**: <10ms query response time

### Memory Management
- **Connection Pooling**: Reuse of database connections
- **Message Queuing**: Bounded queues with overflow protection
- **Buffer Management**: Efficient packet buffering and parsing
- **Resource Cleanup**: Automatic cleanup of disconnected clients

### Network Optimization
- **Binary Protocol**: Compact binary encoding
- **Compression**: Optional payload compression
- **Heartbeat Efficiency**: Minimal heartbeat overhead
- **Frame Batching**: Multiple messages per network packet

## Threading Model

### Server Threads
```cpp
// Main thread: Network I/O and message routing
QTcpServer::newConnection() → onNewConnection()
QTcpSocket::readyRead() → onReadyRead() → handlePacket()

// Timer thread: Periodic maintenance
QTimer::timeout() → checkInactiveClients()
QTimer::timeout() → cleanupOldData()

// Database thread: SQLite operations
DatabaseManager::logMessage() → Worker thread
DatabaseManager::logSession() → Worker thread
```

### Client Threads
```cpp
// Main thread: UI and network I/O
QTcpSocket::connected() → onSocketConnected()
QTcpSocket::readyRead() → onReadyRead() → drainPackets()

// Timer thread: Connection management
QTimer::timeout() → sendHeartbeat()
QTimer::timeout() → onHeartbeatTimeout()
QTimer::timeout() → onReconnectTimer()
```

## Error Handling Strategy

### Protocol Errors
```cpp
enum ErrorCode {
    ERR_NONE = 0,
    ERR_PROTOCOL_VERSION = 1,    // Version mismatch
    ERR_INVALID_FRAME = 2,       // Malformed frame
    ERR_FRAME_TOO_LARGE = 3,     // Size limit exceeded
    ERR_JSON_PARSE = 4,          // JSON parsing failed
    ERR_UNAUTHORIZED = 5,        // Auth required
    ERR_ROOM_NOT_FOUND = 6,      // Room doesn't exist
    ERR_NOT_IN_ROOM = 7,         // User not in room
    ERR_RATE_LIMITED = 8,        // Too many requests
    ERR_INTERNAL = 9             // Server error
};
```

### Recovery Mechanisms
- **Connection Recovery**: Automatic reconnection with exponential backoff
- **Message Recovery**: Queue persistence during connection drops
- **Data Recovery**: Transaction rollback for database errors
- **State Recovery**: Client state synchronization after reconnect

## Configuration Management

### Server Configuration
```json
{
    "server": {
        "port": 9000,
        "database": "/path/to/database.db",
        "heartbeatInterval": 30,
        "heartbeatTimeout": 90,
        "maxClientsPerRoom": 50,
        "rateLimitEnabled": true
    },
    "logging": {
        "level": "info",
        "categories": ["protocol", "network", "roomhub", "device", "recording"]
    },
    "security": {
        "tlsEnabled": false,
        "certificatePath": "",
        "privateKeyPath": ""
    }
}
```

### Client Configuration
```json
{
    "connection": {
        "autoReconnect": true,
        "minReconnectDelay": 1000,
        "maxReconnectDelay": 30000,
        "heartbeatInterval": 30,
        "heartbeatTimeout": 90,
        "maxQueueSize": 100
    },
    "devices": {
        "simulator": {
            "enabled": true,
            "updateInterval": 1000,
            "deviceId": "SIM001"
        },
        "serial": {
            "enabled": false,
            "portName": "/dev/ttyUSB0",
            "baudRate": 9600
        }
    }
}
```

## Deployment Architecture

### Development Environment
```bash
# Build all components
qmake && make

# Run server
./server --port 9000 --database /tmp/test.db --debug

# Run clients
./client-factory
./client-expert
```

### Production Deployment
```yaml
# Docker composition
version: '3.8'
services:
  server:
    image: industrial-remote-expert/server:latest
    ports:
      - "9000:9000"
    volumes:
      - "/data/database:/app/data"
      - "/data/recordings:/app/recordings"
    environment:
      - DATABASE_PATH=/app/data/industrial.db
      - RECORDING_PATH=/app/recordings
      - TLS_ENABLED=true
```

## Testing Strategy

### Unit Testing
- **Protocol**: Frame encoding/decoding validation
- **Database**: CRUD operations and schema migrations
- **Device**: Simulator and data parsing logic
- **Recording**: File operations and session management

### Integration Testing
- **Client-Server**: Full protocol stack testing
- **Device Integration**: Real hardware device testing
- **Database Integration**: Multi-client database access
- **Recording Integration**: End-to-end recording workflows

### Performance Testing
- **Load Testing**: High connection count scenarios
- **Stress Testing**: Resource exhaustion scenarios
- **Latency Testing**: Real-time communication metrics
- **Throughput Testing**: Maximum message and data rates

## Future Architecture Considerations

### Microservices Migration
```
┌─────────────┐  ┌─────────────┐  ┌─────────────┐
│ Auth Service│  │Room Service │  │Media Service│
└─────────────┘  └─────────────┘  └─────────────┘
        │                │                │
        └────────────────┼────────────────┘
                         │
              ┌─────────────┐
              │ API Gateway │
              └─────────────┘
```

### Cloud-Native Features
- **Container Orchestration**: Kubernetes deployment
- **Service Mesh**: Istio for service communication
- **Observability**: Prometheus metrics and Jaeger tracing
- **Auto-scaling**: Horizontal pod autoscaling
- **GitOps**: ArgoCD for deployment automation

### Edge Computing
- **Edge Nodes**: Local processing and caching
- **Offline Mode**: Local operation during connectivity loss
- **Data Synchronization**: Eventual consistency models
- **Bandwidth Optimization**: Intelligent data prioritization