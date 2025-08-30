# Industrial Remote Expert System

An advanced real-time collaboration platform for industrial environments, enabling seamless communication between factory floor operators and remote experts for troubleshooting, maintenance, and knowledge transfer.

## Features

### MVP Foundation (v1.0.0) ✅
- **Enhanced Binary Protocol**: 64-byte frame header with versioning, routing, and validation
- **SQLite Persistence**: Users, workorders, sessions, messages, and recordings
- **Heartbeat & Auto-reconnect**: Network quality monitoring with exponential backoff
- **Device Data Integration**: Serial port, CAN bus, and simulator sources
- **Recording Infrastructure**: JSONL format for messages and device data
- **Rate Limiting**: Per-client request limiting and connection management
- **Structured Logging**: Debug categories for protocol, network, database, and devices

### Core Components
- **Server (RoomHub)**: Central message routing with room-based organization
- **Factory Client**: On-site operator interface with device integration
- **Expert Client**: Remote expert interface with collaboration tools
- **Device Manager**: Multi-source device data aggregation
- **Recording Manager**: Session-based recording with JSONL output

## Quick Start

### Prerequisites (Ubuntu 20.04+)
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    qtbase5-dev \
    qttools5-dev \
    qtmultimedia5-dev \
    libqt5charts5-dev \
    libqt5serialport5-dev \
    libsqlite3-dev
```

### Build and Run
```bash
# Clone repository
git clone https://github.com/KirisameMarisa0602/industrial-remote-expert.git
cd industrial-remote-expert

# Quick demo (builds and runs server + 2 clients)
./tools/dev-run.sh

# Or build components individually
cd server && qmake && make && cd ..
cd client-factory && qmake && make && cd ..
cd client-expert && qmake && make && cd ..
```

### Basic Usage
1. **Start Server**: `./server/server --port 9000 --database /tmp/demo.db --debug`
2. **Connect Clients**: Launch factory and expert clients
3. **Join Room**: Connect to `localhost:9000`, join room `DEMO123`
4. **Test Features**: Send messages, view device data, test heartbeat

## Architecture

### System Overview
```
┌─────────────────┐    ┌─────────────────┐
│  Factory Client │    │  Expert Client  │
│   (Qt Widgets)  │    │   (Qt Widgets)  │
└─────────┬───────┘    └─────────┬───────┘
          │                      │
          └──────────┬───────────┘
                     │
        ┌────────────┴────────────┐
        │     Server (RoomHub)    │
        │  ┌─────────┬─────────┐  │
        │  │Database │Recording│  │
        │  └─────────┴─────────┘  │
        └────────────┬────────────┘
                     │
          ┌──────────┼──────────┐
          │          │          │
    ┌─────┴─────┐ ┌─┴──┐ ┌─────┴─────┐
    │  Serial   │ │CAN │ │Simulator  │
    │ Devices   │ │Bus │ │  Devices  │
    └───────────┘ └────┘ └───────────┘
```

### Protocol Stack
```
┌─────────────────────────────────────┐
│           Application Layer         │
│     (Messages, Device Data)         │
├─────────────────────────────────────┤
│         Enhanced Protocol           │
│  ┌─────────────────────────────────┐ │
│  │        Frame Header             │ │
│  │  Magic│Ver│Type│Flags│Length   │ │
│  │  RoomID│SenderID│Timestamp│Seq  │ │
│  └─────────────────────────────────┘ │
├─────────────────────────────────────┤
│            TCP/IP                   │
└─────────────────────────────────────┘
```

## Configuration

### Server Options
```bash
./server --help
Options:
  -p, --port <port>      Listen port (default: 9000)
  -d, --database <path>  Database file path
  --verbose              Enable verbose logging
  --debug                Enable debug logging
  --heartbeat <seconds>  Heartbeat interval (default: 30)
  --timeout <seconds>    Heartbeat timeout (default: 90)
```

### Environment Variables
```bash
export INDUSTRIAL_EXPERT_DATABASE="/path/to/database.db"
export INDUSTRIAL_EXPERT_LOG_LEVEL="debug"
export INDUSTRIAL_EXPERT_RECORDING_PATH="/path/to/recordings"
```

## Device Integration

### Supported Device Sources
- **Device Simulator**: Generates realistic sensor data (temperature, pressure, vibration)
- **Serial Port**: Reads JSON/CSV data from serial devices
- **CAN Bus (Linux)**: SocketCAN integration for automotive/industrial CAN networks
- **Modbus (Planned)**: Industrial protocol support
- **OPC UA (Planned)**: Industrial automation standard

### Device Configuration
```json
{
  "devices": {
    "simulator": {
      "enabled": true,
      "updateInterval": 1000,
      "deviceId": "SIM001",
      "metrics": [
        {"name": "temperature", "unit": "°C", "min": 15.0, "max": 35.0},
        {"name": "pressure", "unit": "bar", "min": 0.8, "max": 1.2}
      ]
    },
    "serial": {
      "enabled": false,
      "portName": "/dev/ttyUSB0",
      "baudRate": 9600,
      "deviceId": "SERIAL001"
    }
  }
}
```

## Recording and Persistence

### Database Schema
- **Users**: Authentication and user management
- **Workorders**: Job tracking and organization
- **Sessions**: Room membership and connection tracking
- **Messages**: Text communication and system events
- **Recordings**: File-based recording index

### Recording Formats
- **Messages**: JSONL format with complete message metadata
- **Device Data**: Time-series data in JSONL format
- **Audio/Video**: Planned MP4/MKV container support

### Example Recording (JSONL)
```json
{"type":"header","sessionId":"abc-123","roomId":"ROOM001","startTime":"2024-01-01T00:00:00Z"}
{"type":"message","timestamp":1704067200000,"messageType":20,"senderId":"user1","json":{"text":"Hello"}}
{"type":"device_sample","timestamp":1704067201000,"sample":{"deviceId":"SIM001","metric":"temperature","value":25.5,"unit":"°C"}}
{"type":"footer","endTime":"2024-01-01T01:00:00Z","itemCount":1500,"fileSize":245760}
```

## Development

### Project Structure
```
industrial-remote-expert/
├── common/                 # Shared protocol and interfaces
│   ├── protocol.h/cpp     # Binary protocol implementation
│   ├── device.h/cpp       # Device data interfaces
│   └── recording.h/cpp    # Recording infrastructure
├── server/                # Server application
│   └── src/
│       ├── roomhub.h/cpp  # Core server logic
│       └── database.h/cpp # SQLite persistence
├── client-factory/        # Factory client application
├── client-expert/         # Expert client application
├── docs/                  # Comprehensive documentation
└── tools/                 # Development utilities
```

### Build System
- **qmake**: Qt's native build system
- **CI/CD**: GitHub Actions for Ubuntu, Windows, macOS
- **Testing**: Integration tests with server startup validation

### Code Quality
- **Style**: clang-format with Google C++ style (modified)
- **Standards**: C++11 with Qt 5.12+ compatibility
- **Logging**: Structured logging with QLoggingCategory
- **Documentation**: Doxygen-style API documentation

## Security

### Current Security Features
- **Frame Validation**: Magic numbers, size limits, format validation
- **Rate Limiting**: Per-client request rate limiting
- **Input Sanitization**: JSON payload validation
- **SQLite Security**: Parameterized queries prevent SQL injection

### Planned Security Features
- **TLS 1.3**: Transport layer encryption
- **Authentication**: bcrypt password hashing with MFA
- **Authorization**: Role-based access control (RBAC)
- **Audit Logging**: Comprehensive security event tracking
- **End-to-End Encryption**: Message-level encryption

## Performance

### Scalability Targets
- **Concurrent Users**: 1,000+ simultaneous connections
- **Message Throughput**: 10,000+ messages/second
- **Device Data Rate**: 100,000+ samples/second
- **Network Latency**: <50ms for local networks

### Optimization Features
- **Binary Protocol**: Compact frame encoding
- **Connection Pooling**: Efficient resource management
- **Message Queuing**: Bounded queues with backpressure
- **Database Indexing**: Optimized query performance

## Roadmap

### Next Release (v1.1.0) - Audio/Video
- Audio capture and encoding (Opus codec)
- Video streaming with H.264 encoding
- RTP/RTCP transport implementation
- Real-time media synchronization

### Future Releases
- **v1.2.0**: Security & Authentication (TLS, RBAC, MFA)
- **v1.3.0**: Device Control & Automation (Modbus, OPC UA, scripting)
- **v1.4.0**: Knowledge Base & AI (document management, ML integration)
- **v2.0.0**: Mobile & Cloud (iOS/Android apps, cloud deployment)

See [docs/ROADMAP.md](docs/ROADMAP.md) for detailed planning.

## Documentation

- **[DESIGN.md](docs/DESIGN.md)**: System architecture and technical details
- **[SECURITY.md](docs/SECURITY.md)**: Security architecture and threat model
- **[CONTRIBUTING.md](CONTRIBUTING.md)**: Development guidelines and contribution process
- **[ROADMAP.md](docs/ROADMAP.md)**: Feature roadmap and future planning

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contributing

We welcome contributions! Please read [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines on:
- Setting up development environment
- Coding standards and style guide
- Pull request process
- Bug reporting and feature requests

## Support

- **GitHub Issues**: Bug reports and feature requests
- **GitHub Discussions**: Questions and community discussion
- **Documentation**: Comprehensive docs in the `docs/` directory

## Acknowledgments

- Qt Framework for cross-platform GUI and networking
- SQLite for embedded database functionality
- Contributors and the industrial automation community