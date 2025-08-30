# Industrial Remote Expert

A Qt-based industrial remote support system with role-based authentication and real-time communication.

## Quick Start

### Prerequisites (Ubuntu 24.04)
```bash
sudo apt update
sudo apt install -y build-essential qtbase5-dev qtcreator qtmultimedia5-dev \
                    qml-module-qtquick-controls2 qml-module-qtcharts \
                    libqt5charts5-dev libsqlite3-dev libqt5sql5-sqlite \
                    libqt5multimedia5-plugins
```

### Build and Run

1. **Clone the repository**
```bash
git clone git@github.com:KirisameMarisa0602/industrial-remote-expert.git
cd industrial-remote-expert
```

2. **Build all components**
```bash
# Build server
cd server && qmake && make && cd ..

# Build expert client  
cd client-expert && qmake && make && cd ..

# Build factory client
cd client-factory && qmake && make && cd ..
```

3. **Run the system**

Start the server first:
```bash
cd server && ./server
```

Start client applications (in separate terminals):
```bash
# Expert client
cd client-expert && ./client-expert

# Factory client  
cd client-factory && ./client-factory
```

### Usage

1. **Login/Registration**: Each client will show a login dialog on startup
   - Select your role: **工厂** (Factory) or **专家** (Expert)
   - Register new users or login with existing credentials
   - Server host/port can be configured (default: 127.0.0.1:9000)

2. **Role-based Access**: 
   - Factory users can only access the factory client
   - Expert users can only access the expert client
   - Each user gets an authenticated session with 24-hour expiry

3. **Main Features**:
   - Real-time text communication
   - Video streaming between factory and expert users
   - Work order (room) based collaboration
   - Dark theme UI with professional look

## Architecture

### Project Structure
```
industrial-remote-expert/
├── common/                 # Shared components
│   ├── protocol.h/.cpp    # Network protocol definitions
│   ├── ui/                # Shared UI components
│   │   └── loginregisterdialog.h/.cpp
│   └── resources/         # Themes and assets
│       └── theme-dark.qss
├── server/                # Server application  
│   └── src/roomhub.cpp    # Main server logic with auth
├── client-expert/         # Expert client application
├── client-factory/        # Factory client application
└── README.md
```

### Authentication System

- **Password Security**: Salt + SHA-256 hash storage
- **Session Management**: 24-hour JWT-like tokens  
- **Role-based Access**: Factory/Expert user separation
- **Database**: SQLite with users and sessions tables

### Communication Protocol

- **Transport**: TCP with binary frame headers
- **Message Types**: LOGIN, REGISTER, TEXT, VIDEO_FRAME, JOIN_WORKORDER
- **Authentication**: Session tokens included in all requests
- **Error Handling**: Structured error codes and messages

## Development

### Message Protocol
See `common/protocol.h` for message types. Add new message types to the `MsgType` enum and handle them in the server's `handlePacket()` method.

### Adding Features
- **Server**: Extend `RoomHub` class in `server/src/roomhub.cpp`
- **Clients**: Modify `MainWindow` classes or add new UI components in `common/ui/`
- **Protocol**: Update `common/protocol.h` for new message types

### Dark Theme
The application uses a unified dark theme defined in `common/resources/theme-dark.qss`. The theme is automatically applied on application startup.

## Troubleshooting

### Build Issues
- **Qt not found**: Ensure Qt5 development packages are installed
- **Missing multimedia**: Install `libqt5multimedia5-plugins`
- **SQLite errors**: Install `libqt5sql5-sqlite`

### Runtime Issues  
- **Login fails**: Check server is running on correct port (default 9000)
- **Role mismatch**: Ensure you're using the correct client for your user role
- **Camera not working**: Check camera permissions and `libqt5multimedia5-plugins`
- **Dark theme not applied**: Verify `common/resources/resources.qrc` is properly built

### Database Issues
- **Database location**: `server/industrial_remote_expert.db` 
- **Reset database**: Delete the database file to start fresh
- **Check users**: Use SQLite browser to inspect users and sessions tables

## License

[Add your license here]