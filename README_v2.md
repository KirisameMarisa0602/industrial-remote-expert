# Industrial Remote Expert System v2.0

A comprehensive Qt-based remote assistance system with role-based authentication, modern UI, and real-time communication capabilities.

## Features

### Authentication & Role Management
- **Unified Login/Registration**: Single interface for both Expert and Factory users
- **Role-based Access Control**: Users select role (Expert/Factory) during registration
- **Secure Authentication**: SHA-256 password hashing with per-user salt
- **Session Management**: Token-based sessions with 24-hour expiration
- **Modern UI**: Beautiful, responsive interface with modern styling

### Role-specific Functionality

#### Expert Users
- View and join available work orders
- Provide remote assistance through text chat
- Receive live video feeds from factory equipment
- Monitor multiple work order rooms

#### Factory Users
- Create work orders for assistance requests
- Share live video feed from equipment cameras
- Real-time text communication with experts
- Camera controls and auto-start options

### Technical Features
- **Real-time Communication**: Text chat and video streaming
- **Work Order Management**: Full CRUD operations for work orders
- **Multi-client Support**: Multiple users can collaborate simultaneously
- **Database Persistence**: SQLite database for users, sessions, and work orders
- **Cross-platform**: Runs on Linux, Windows, and macOS

## System Requirements

### Development Environment
- **Qt 5.12+** with the following modules:
  - QtCore
  - QtGui
  - QtWidgets
  - QtNetwork
  - QtSql
  - QtMultimedia
  - QtMultimediaWidgets
- **GCC/Clang** with C++11 support
- **SQLite 3.x**
- **qmake** build system

### Ubuntu 20.04/22.04/24.04 Installation
```bash
sudo apt update
sudo apt install -y build-essential \
    qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools \
    qtcreator qtmultimedia5-dev libqt5charts5-dev \
    libsqlite3-dev libqt5sql5-sqlite
```

## Building the System

### 1. Clone the Repository
```bash
git clone https://github.com/KirisameMarisa0602/industrial-remote-expert.git
cd industrial-remote-expert
```

### 2. Build Server
```bash
cd server
qmake -qt5
make -j$(nproc)
```

### 3. Build Expert Client
```bash
cd ../client-expert
qmake -qt5
make -j$(nproc)
```

### 4. Build Factory Client
```bash
cd ../client-factory
qmake -qt5
make -j$(nproc)
```

## Usage Instructions

### Starting the System

#### 1. Start Server
```bash
cd server
./server -p 9000
```
The server will:
- Create `./data/server.db` SQLite database if it doesn't exist
- Initialize all required tables automatically
- Listen on port 9000 for client connections

#### 2. Start Clients
```bash
# Expert client
cd client-expert
./client-expert

# Factory client (in another terminal)
cd client-factory
./client-factory
```

### Using the Application

#### Registration Process
1. **Connect to Server**: Enter server IP (127.0.0.1 for local) and port (9000)
2. **Select Role**: Choose "Expert" or "Factory" - this determines your main interface
3. **Register Account**: Enter username and password, then click "Register"
4. **Login**: Use the same credentials to login

#### Expert Workflow
1. Login with Expert role
2. Navigate to "Work Orders" tab to see available assistance requests
3. Select a work order and click "Join Selected Work Order"
4. Switch to "Communication" tab to chat and view video feed
5. Provide assistance through text communication

#### Factory Workflow
1. Login with Factory role
2. Create work orders in "Create Work Order" tab with problem description
3. Join the work order room using the generated ID
4. Start camera to share video feed with experts
5. Communicate via text chat for assistance

## Database Schema

The system uses SQLite with the following tables:

### Users Table
```sql
CREATE TABLE users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT UNIQUE NOT NULL,
    password_hash TEXT NOT NULL,
    salt TEXT NOT NULL,
    role TEXT CHECK(role IN ('expert','factory')) NOT NULL,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);
```

### Sessions Table
```sql
CREATE TABLE sessions (
    token TEXT PRIMARY KEY,
    user_id INTEGER NOT NULL,
    expires_at DATETIME NOT NULL,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users (id)
);
```

### Work Orders Table
```sql
CREATE TABLE work_orders (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    title TEXT NOT NULL,
    description TEXT,
    status TEXT CHECK(status IN ('open','in_progress','closed')) NOT NULL DEFAULT 'open',
    created_by INTEGER NOT NULL,
    assigned_to INTEGER NULL,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (created_by) REFERENCES users (id),
    FOREIGN KEY (assigned_to) REFERENCES users (id)
);
```

### Work Order Comments Table (Optional)
```sql
CREATE TABLE work_order_comments (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    work_order_id INTEGER NOT NULL,
    author_id INTEGER NOT NULL,
    body TEXT NOT NULL,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (work_order_id) REFERENCES work_orders (id),
    FOREIGN KEY (author_id) REFERENCES users (id)
);
```

## Protocol Reference

The system uses a custom TCP-based protocol with JSON messages:

### Authentication Messages
- `MSG_REGISTER`: Register new user account
- `MSG_LOGIN`: Authenticate user and receive session token

### Work Order Management
- `MSG_CREATE_WORKORDER`: Create new work order
- `MSG_LIST_WORKORDERS`: List available work orders
- `MSG_UPDATE_WORKORDER`: Update work order status/assignment
- `MSG_DELETE_WORKORDER`: Delete work order (creator only)
- `MSG_JOIN_WORKORDER`: Join work order room for communication

### Communication Messages
- `MSG_TEXT`: Text chat message
- `MSG_VIDEO_FRAME`: Video frame data (JPEG format)
- `MSG_AUDIO_FRAME`: Audio data (future use)

### Server Responses
- `MSG_SERVER_EVENT`: Server response with status code and message

## Testing the System

### Basic Functionality Test
1. Start server: `./server -p 9000`
2. Register expert user: username="expert1", role="expert"
3. Register factory user: username="factory1", role="factory"
4. Login as factory user, create work order
5. Login as expert user, join the work order
6. Test text communication and video sharing

### Database Verification
```bash
# Check created database
sqlite3 ./data/server.db
.tables
.schema users
SELECT * FROM users;
```

## Development Notes

### Adding New Features
1. Update protocol definitions in `common/protocol.h`
2. Add server handlers in `server/src/roomhub.cpp`
3. Update client UI in respective main windows
4. Test with multiple client instances

### Security Considerations
- Passwords are hashed with SHA-256 + salt
- Session tokens expire after 24 hours
- Database uses foreign key constraints
- Input validation on all server endpoints

### Performance Notes
- Video frames are rate-limited to prevent bandwidth overload
- Database queries use prepared statements
- Connection pooling for multiple clients
- Efficient packet parsing with binary protocol

## Troubleshooting

### Build Issues
- **Qt modules not found**: Ensure all Qt5 development packages are installed
- **qmake not found**: Install `qt5-qmake` package
- **Link errors**: Check that all required libraries are installed

### Runtime Issues
- **Database errors**: Ensure write permissions in server directory
- **Connection failed**: Check server is running and port is accessible
- **Video not working**: Ensure camera permissions and multimedia packages

### Common Error Messages
- "Authentication required": Login with valid credentials first
- "Role selection required": Select Expert or Factory role before login/register
- "Room not found": Ensure work order ID exists and is accessible

## License

This project is part of the Industrial Remote Expert System.

## Contributing

1. Fork the repository
2. Create feature branch
3. Make changes with comprehensive tests
4. Submit pull request with detailed description

## Support

For technical support or feature requests, please create an issue in the GitHub repository.