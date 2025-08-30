# Industrial Remote Expert System (工业现场远程专家支持系统)

A comprehensive Qt 5.15.13 solution for industrial remote expert support, featuring role-based authentication, modern UI design, real-time video/audio communication, and work order management.

## 🏗️ System Architecture

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Launcher      │    │     Server      │    │  Client Apps    │
│  (Auth Portal)  │◄──►│  (Room Hub)     │◄──►│ Factory/Expert  │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
         ▼                       ▼                       ▼
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│ Role Selection  │    │ SQLite Database │    │ Modern UI with  │
│ (工厂/专家)      │    │ - Users/Roles   │    │ - Dashboard     │
│                 │    │ - Tickets       │    │ - Video/Audio   │
│                 │    │ - Messages      │    │ - Chat          │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

## ✨ Key Features

### 🔐 Enhanced Authentication System
- **Role-Based Access Control**: Users select "工厂" (Factory) or "专家" (Expert) roles
- **Secure Password Storage**: Salted hash with random salt generation using QCryptographicHash
- **Session Management**: Token-based authentication with 24-hour expiration
- **"未选择身份" Validation**: UI clearly indicates when no role is selected

### 🎨 Modern UI Framework
- **Dark Theme**: Industrial-style dark theme with accent colors
- **Sidebar Navigation**: Modern navigation with dashboard, communication, work orders, and settings
- **Dashboard Cards**: Real-time status monitoring with interactive widgets
- **Responsive Layout**: Professional split-pane layouts with Qt Splitter

### 🌐 Multi-Party Communication
- **Video Streaming**: MJPEG streaming over TCP using QCamera
- **Audio Support**: PCM audio streaming with QAudioInput/QAudioOutput
- **Real-time Chat**: UTF-8 text messaging with timestamps
- **Room Management**: Multi-participant rooms based on work order codes

### 📋 Work Order Management
- **Ticket System**: Create, join, and manage work orders with unique codes
- **Database Persistence**: SQLite storage with proper foreign key relationships
- **Role-based Workflow**: Factory creates tickets, experts join and assist
- **History Tracking**: Complete audit trail of all activities

## 🗄️ Database Schema

```sql
-- Enhanced user table with roles and salted passwords
CREATE TABLE users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT UNIQUE NOT NULL,
    salt BLOB NOT NULL,           -- Random salt for password hashing
    passhash BLOB NOT NULL,       -- SHA256(password + salt)
    role TEXT NOT NULL CHECK(role IN ('工厂', '专家')),
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- Work order tickets
CREATE TABLE tickets (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    code TEXT UNIQUE NOT NULL,    -- Unique room code
    title TEXT NOT NULL,
    description TEXT,
    status TEXT NOT NULL DEFAULT 'open' CHECK(status IN ('open', 'in_progress', 'closed')),
    created_by INTEGER NOT NULL,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (created_by) REFERENCES users (id)
);

-- Ticket participants (multi-party support)
CREATE TABLE ticket_participants (
    ticket_id INTEGER NOT NULL,
    user_id INTEGER NOT NULL,
    joined_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (ticket_id, user_id),
    FOREIGN KEY (ticket_id) REFERENCES tickets (id),
    FOREIGN KEY (user_id) REFERENCES users (id)
);

-- Message history with media support
CREATE TABLE messages (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    ticket_id INTEGER NOT NULL,
    sender_id INTEGER NOT NULL,
    kind TEXT NOT NULL CHECK(kind IN ('text', 'video', 'audio', 'system')),
    content BLOB,                 -- JSON for text, binary for media
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (ticket_id) REFERENCES tickets (id),
    FOREIGN KEY (sender_id) REFERENCES users (id)
);

-- Session management
CREATE TABLE sessions (
    token TEXT PRIMARY KEY,
    username TEXT NOT NULL,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    expires_at DATETIME NOT NULL,
    FOREIGN KEY (username) REFERENCES users (username)
);
```

## 🚀 Quick Start

### Prerequisites
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install -y build-essential qtbase5-dev qttools5-dev qtcreator \
                    qtmultimedia5-dev qml-module-qtquick-controls2 \
                    qml-module-qtcharts libqt5charts5-dev libsqlite3-dev qt5-qmake

# Verify Qt installation
qmake --version  # Should show Qt 5.x
```

### Build All Components
```bash
# Clone and build the entire project
git clone https://github.com/KirisameMarisa0602/industrial-remote-expert.git
cd industrial-remote-expert

# Build all components with top-level project file
qmake industrial-remote-expert.pro
make -j$(nproc)

# Or build individual components
cd server && qmake && make && cd ..
cd client-expert && qmake && make && cd ..
cd client-factory && qmake && make && cd ..
cd launcher && qmake && make && cd ..
```

### Build Modern UI Version
```bash
# Build modern expert client with enhanced UI
cd client-expert
qmake CONFIG+=modern
make clean && make
# This creates 'client-expert-modern' binary
```

## 🎮 Usage Guide

### Method 1: Using the Launcher (Recommended)
```bash
# 1. Start the server
cd server
./server -p 9000

# 2. Run the launcher (handles authentication and role routing)
cd launcher
./launcher
```

The launcher provides:
- Server connection interface
- Unified authentication with role selection
- Automatic client launch based on user role
- Modern themed UI

### Method 2: Direct Client Execution
```bash
# 1. Start server
cd server && ./server -p 9000

# 2. Run clients directly (for development/testing)
cd client-expert && ./client-expert        # Basic UI
cd client-expert && ./client-expert-modern # Modern UI
cd client-factory && ./client-factory
```

## 🎯 Testing Workflow

### Complete End-to-End Test
1. **Start Server**: `./server/server -p 9000`
2. **Launch Authentication**: `./launcher/launcher`
3. **Register Users**:
   - Factory user with role "工厂"
   - Expert user with role "专家"
4. **Test Communication**:
   - Factory creates work order
   - Expert joins using work order code
   - Test video/audio/chat functionality
5. **Verify Database**: Check `server/data/app.db` for persisted data

### Role-Based Workflow Test
```bash
# Terminal 1: Server
cd server && ./server -p 9000

# Terminal 2: Factory Client
cd launcher && ./launcher
# Register as "工厂" role -> launches client-factory

# Terminal 3: Expert Client  
cd launcher && ./launcher
# Register as "专家" role -> launches client-expert-modern
```

## 🏗️ Project Structure

```
industrial-remote-expert/
├── server/                    # TCP server with room management
│   ├── src/roomhub.{h,cpp}   # Main server logic with enhanced auth
│   ├── data/app.db           # SQLite database (created automatically)
│   └── server.pro            # Qt project file
├── client-expert/            # Expert client application
│   ├── src/modernmainwindow.{h,cpp} # Modern UI implementation
│   ├── src/mainwindow.{h,cpp}       # Legacy UI
│   ├── styles/theme-dark.qss        # Dark theme
│   └── client-expert.pro            # Qt project file
├── client-factory/           # Factory client application
│   ├── src/mainwindow.{h,cpp}# Factory-specific UI
│   └── client-factory.pro    # Qt project file
├── launcher/                 # Authentication and role routing
│   ├── src/mainwindow.{h,cpp}# Launcher UI with auth flow
│   └── launcher.pro          # Qt project file
├── common/                   # Shared components
│   ├── auth/authwidget.{h,cpp}      # Unified login/register UI
│   ├── dashboard/dashboardwidget.{h,cpp} # Dashboard components
│   ├── sidebar/sidebarwidget.{h,cpp}    # Navigation sidebar
│   ├── styles/theme-dark.qss        # Application theme
│   ├── protocol.{h,cpp}             # Network protocol
│   └── common.pri                   # Shared build configuration
└── industrial-remote-expert.pro    # Top-level project file
```

## 🔧 Development Guide

### Adding New Features
```bash
# 1. Add shared components to common/
# 2. Include in common.pri
# 3. Use in client applications
# 4. Test with launcher workflow
```

### Extending the Protocol
```cpp
// Add new message types to common/protocol.h
enum MsgType : quint16 {
    MSG_CUSTOM_FEATURE = 100,  // Add new types starting from 100
    // ...
};

// Handle in server (roomhub.cpp) and clients
void handleCustomFeature(ClientCtx* c, const Packet& p) {
    // Implementation
}
```

### UI Customization
- Modify `common/styles/theme-dark.qss` for global styling
- Add custom widgets to `common/` directories
- Use dashboard cards for status displays
- Implement sidebar navigation for multi-section UIs

## 🖥️ VM Setup Guide

### Ubuntu 24.04 LTS VM Configuration
```bash
# 1. Install Qt development environment
sudo apt update
sudo apt install -y build-essential qtbase5-dev qttools5-dev qtcreator \
                    qtmultimedia5-dev libqt5charts5-dev libsqlite3-dev

# 2. Clone and build project
git clone <repository-url>
cd industrial-remote-expert
qmake && make

# 3. Test with display forwarding (if using SSH)
export DISPLAY=:0  # or appropriate display
./launcher/launcher
```

### Qt Creator Setup
1. Open Qt Creator
2. File → Open File or Project
3. Select `industrial-remote-expert.pro`
4. Configure kit: Desktop Qt 5.15.13 GCC 64bit
5. Build → Build All Projects
6. Run → Run (select target: launcher, server, etc.)

## 📝 Protocol Specification

### Message Types
- `MSG_REGISTER` (1): User registration with role
- `MSG_LOGIN` (2): User authentication
- `MSG_JOIN_WORKORDER` (4): Join work order room
- `MSG_TEXT` (10): Text chat message
- `MSG_VIDEO_FRAME` (40): MJPEG video frame
- `MSG_AUDIO_FRAME` (30): PCM audio data
- `MSG_SERVER_EVENT` (90): Server notifications

### Frame Format
```
[FrameHeader:64bytes][JSONPayload][BinaryPayload]
```

### Authentication Flow
```
Client                Server
  │                     │
  ├─ MSG_REGISTER ─────►│  (username, password, role)
  │◄──── MSG_SERVER_EVENT (success/error)
  │                     │
  ├─ MSG_LOGIN ────────►│  (username, password) 
  │◄──── MSG_SERVER_EVENT (token, role)
  │                     │
  ├─ MSG_JOIN_WORKORDER ►│  (roomId, user)
  │◄──── MSG_SERVER_EVENT (joined/error)
```

## 🔍 Troubleshooting

### Common Build Issues
```bash
# Qt not found
export PATH=/usr/lib/qt5/bin:$PATH
export PKG_CONFIG_PATH=/usr/lib/x86_64-linux-gnu/pkgconfig

# Missing dependencies
sudo apt install qtbase5-dev qtmultimedia5-dev

# MOC errors
qmake && make clean && make
```

### Runtime Issues
```bash
# Database permissions
chmod 755 server/data/
chmod 644 server/data/app.db

# Network connectivity
netstat -an | grep 9000  # Check if server is listening
telnet localhost 9000    # Test server connection
```

### Theme/UI Issues
```bash
# Missing resources
qmake && make clean && make  # Rebuild with resources

# Display issues in VM
export QT_QPA_PLATFORM=xcb  # Force X11 platform
xhost +local:  # Allow local connections
```

## 📊 Performance Notes

- **Database**: SQLite with WAL mode for concurrent access
- **Video**: MJPEG at 30fps, adjustable quality settings
- **Audio**: 44.1kHz PCM, 16-bit samples
- **Network**: TCP with length-prefixed frames
- **Memory**: ~50MB per client, ~100MB for server

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Follow the existing code style
4. Add tests for new features
5. Update documentation
6. Submit a pull request

## 📄 License

This project is part of an industrial remote expert support system implementation. Please refer to the project documentation for licensing information.

---

## 修改履历 (Change History)

| Version | Date | Author | Description |
|---------|------|--------|-------------|
| 1.0.0 | 2024-08-30 | System | Initial implementation with authentication, modern UI, dashboard, work order management, and enhanced database schema |