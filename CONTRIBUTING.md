# Contributing to Industrial Remote Expert System

Thank you for your interest in contributing to the Industrial Remote Expert System! This document provides guidelines and information for contributors.

## Getting Started

### Prerequisites
- Qt 5.12+ (with Charts and SerialPort modules)
- CMake 3.16+ or qmake
- C++11 compatible compiler (GCC 7+, Clang 6+, MSVC 2017+)
- Git for version control

### Development Environment Setup

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    qtbase5-dev \
    qttools5-dev \
    qtmultimedia5-dev \
    libqt5charts5-dev \
    libqt5serialport5-dev \
    libsqlite3-dev \
    git
```

#### Windows
1. Install Visual Studio 2019 or later
2. Install Qt 5.15+ from qt.io
3. Install Git for Windows

#### macOS
```bash
brew install qt@5
brew install sqlite3
```

### Building the Project
```bash
# Clone the repository
git clone https://github.com/KirisameMarisa0602/industrial-remote-expert.git
cd industrial-remote-expert

# Build server
cd server
qmake && make
cd ..

# Build clients
cd client-factory
qmake && make
cd ..

cd client-expert
qmake && make
cd ..
```

## Project Structure

```
industrial-remote-expert/
├── common/                 # Shared code (protocol, device interfaces)
│   ├── protocol.h/cpp     # Network protocol implementation
│   ├── device.h/cpp       # Device data interfaces
│   └── recording.h/cpp    # Recording infrastructure
├── server/                # Server application
│   └── src/
│       ├── main.cpp       # Server entry point
│       ├── roomhub.h/cpp  # Core server logic
│       └── database.h/cpp # SQLite persistence
├── client-factory/        # Factory client application
│   └── src/
├── client-expert/         # Expert client application
│   └── src/
├── docs/                  # Documentation
└── .github/workflows/     # CI/CD pipelines
```

## Coding Standards

### C++ Style Guide
We follow a modified version of the Google C++ Style Guide with Qt-specific adaptations.

#### Naming Conventions
```cpp
// Classes: PascalCase
class DeviceManager {};

// Functions and variables: camelCase
void processMessage();
int messageCount;

// Constants: UPPER_SNAKE_CASE
static const int MAX_CONNECTIONS = 1000;

// Private members: camelCase with trailing underscore
class Example {
private:
    int privateValue_;
    QTimer* timer_;
};
```

#### Code Formatting
Use clang-format with the following configuration:

```yaml
# .clang-format
BasedOnStyle: Google
IndentWidth: 4
ColumnLimit: 100
PointerAlignment: Left
AllowShortIfStatementsOnASingleLine: false
AllowShortLoopsOnASingleLine: false
```

#### Header Guards
Use `#pragma once` instead of traditional header guards:
```cpp
#pragma once
// File content here
```

### Qt-Specific Guidelines

#### Signal-Slot Connections
Prefer new-style connections:
```cpp
// Good
connect(timer, &QTimer::timeout, this, &MyClass::onTimeout);

// Avoid
connect(timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
```

#### Memory Management
- Use Qt's parent-child ownership model
- Prefer stack allocation for temporary objects
- Use smart pointers for explicit ownership

```cpp
// Good - Qt manages memory
auto* timer = new QTimer(this);

// Good - Stack allocation
QJsonObject config;

// Good - Smart pointer for explicit ownership
std::unique_ptr<DeviceManager> manager;
```

#### Qt Containers vs STL
Prefer Qt containers for Qt-related code:
```cpp
// Good for Qt objects
QList<QTcpSocket*> connections;
QHash<QString, ClientData> clients;

// Good for algorithms
std::vector<int> numbers;
std::unordered_map<std::string, int> counters;
```

## Git Workflow

### Branching Strategy
We use GitFlow with the following branches:
- `main`: Production-ready code
- `develop`: Integration branch for features
- `feature/*`: Feature development
- `hotfix/*`: Critical bug fixes
- `release/*`: Release preparation

### Commit Messages
Follow conventional commits format:
```
type(scope): description

body (optional)

footer (optional)
```

Types:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes
- `refactor`: Code refactoring
- `test`: Test changes
- `chore`: Build/maintenance tasks

Examples:
```
feat(protocol): add heartbeat mechanism

Implement client-server heartbeat with configurable intervals
and automatic reconnection on timeout.

Closes #123
```

```
fix(database): prevent SQL injection in user queries

Use parameterized queries for all database operations
to prevent SQL injection vulnerabilities.
```

### Pull Request Process

1. **Fork and Clone**
   ```bash
   git clone https://github.com/your-username/industrial-remote-expert.git
   cd industrial-remote-expert
   git remote add upstream https://github.com/KirisameMarisa0602/industrial-remote-expert.git
   ```

2. **Create Feature Branch**
   ```bash
   git checkout -b feature/your-feature-name
   ```

3. **Make Changes**
   - Write code following our style guide
   - Add tests for new functionality
   - Update documentation if needed

4. **Test Your Changes**
   ```bash
   # Build all components
   ./build-all.sh
   
   # Run basic tests
   ./test-integration.sh
   ```

5. **Commit and Push**
   ```bash
   git add .
   git commit -m "feat(component): your change description"
   git push origin feature/your-feature-name
   ```

6. **Create Pull Request**
   - Use the PR template
   - Reference related issues
   - Ensure CI passes

### PR Template
```markdown
## Description
Brief description of changes

## Type of Change
- [ ] Bug fix
- [ ] New feature
- [ ] Breaking change
- [ ] Documentation update

## Testing
- [ ] Unit tests pass
- [ ] Integration tests pass
- [ ] Manual testing completed

## Checklist
- [ ] Code follows style guidelines
- [ ] Self-review completed
- [ ] Documentation updated
- [ ] No new warnings introduced
```

## Development Guidelines

### Adding New Features

#### Protocol Changes
When modifying the protocol:
1. Maintain backward compatibility
2. Update protocol version if needed
3. Document changes in DESIGN.md
4. Add validation tests

```cpp
// Example: Adding new message type
enum MsgType : quint16 {
    // Existing types...
    MSG_NEW_FEATURE = 100,  // Add at end to maintain compatibility
};
```

#### Database Schema Changes
For database modifications:
1. Create migration scripts
2. Update schema version
3. Test with existing data
4. Document in DESIGN.md

```cpp
// Example: Database migration
bool DatabaseManager::upgradeSchema(int fromVersion, int toVersion) {
    if (fromVersion == 1 && toVersion == 2) {
        return executeQuery("ALTER TABLE users ADD COLUMN last_activity DATETIME");
    }
    return false;
}
```

#### Device Interface Extensions
When adding device support:
1. Implement IDeviceSource interface
2. Add configuration options
3. Include error handling
4. Write integration tests

### Testing Guidelines

#### Unit Tests
```cpp
// Example unit test
class TestProtocol : public QObject {
    Q_OBJECT
    
private slots:
    void testPacketEncoding() {
        QJsonObject json{{"test", "value"}};
        QByteArray packet = buildPacket(MSG_TEXT, json);
        
        QVector<Packet> parsed;
        QByteArray buffer = packet;
        QVERIFY(drainPackets(buffer, parsed));
        QCOMPARE(parsed.size(), 1);
        QCOMPARE(parsed[0].type, MSG_TEXT);
        QCOMPARE(parsed[0].json["test"].toString(), "value");
    }
};
```

#### Integration Tests
```bash
#!/bin/bash
# test-integration.sh

# Start server
./server --port 9999 --database /tmp/test.db &
SERVER_PID=$!
sleep 2

# Test connectivity
nc -z localhost 9999
if [ $? -eq 0 ]; then
    echo "Server connectivity: PASS"
else
    echo "Server connectivity: FAIL"
    exit 1
fi

# Cleanup
kill $SERVER_PID
```

### Documentation Guidelines

#### Code Documentation
Use Doxygen-style comments for public APIs:
```cpp
/**
 * @brief Sends a message to the specified room
 * @param roomId Target room identifier
 * @param message Message content
 * @param type Message type
 * @return true if message was queued successfully
 */
bool sendMessage(const QString& roomId, const QString& message, MessageType type);
```

#### Architecture Documents
When adding major features:
1. Update DESIGN.md with architecture changes
2. Add sequence diagrams for complex flows
3. Document configuration options
4. Include security considerations

### Performance Guidelines

#### General Performance
- Use const references for large objects
- Prefer move semantics for temporary objects
- Avoid unnecessary memory allocations
- Use Qt's implicit sharing where appropriate

```cpp
// Good
void processMessage(const QJsonObject& message);
auto result = std::move(computation);

// Avoid
void processMessage(QJsonObject message);  // Unnecessary copy
```

#### Network Performance
- Batch small messages when possible
- Use binary encoding for large data
- Implement flow control for device data
- Monitor and limit memory usage

#### Database Performance
- Use prepared statements
- Create appropriate indexes
- Batch database operations
- Monitor query performance

## Bug Reports and Feature Requests

### Bug Reports
When reporting bugs, include:
1. **Environment**: OS, Qt version, compiler
2. **Steps to Reproduce**: Detailed steps
3. **Expected Behavior**: What should happen
4. **Actual Behavior**: What actually happens
5. **Logs**: Relevant log output
6. **Additional Context**: Screenshots, configs

Use this template:
```markdown
**Environment**
- OS: Ubuntu 20.04
- Qt Version: 5.15.2
- Build Type: Debug

**Steps to Reproduce**
1. Start server with `./server --port 9000`
2. Connect client
3. Send message "test"

**Expected Behavior**
Message should be delivered to other clients

**Actual Behavior**
Message is not delivered, server logs show error

**Logs**
```
[ERROR] Failed to broadcast message: Invalid room ID
```

**Additional Context**
This happens only with room IDs containing spaces
```

### Feature Requests
For feature requests, provide:
1. **Use Case**: Why is this needed?
2. **Proposed Solution**: How should it work?
3. **Alternatives**: Other possible approaches
4. **Implementation Notes**: Technical considerations

## Community Guidelines

### Code of Conduct
We follow the [Contributor Covenant](https://www.contributor-covenant.org/):
- Be respectful and inclusive
- Welcome newcomers and help them learn
- Focus on constructive feedback
- Respect different viewpoints and experiences

### Communication Channels
- **GitHub Issues**: Bug reports and feature requests
- **GitHub Discussions**: General questions and ideas
- **Pull Request Reviews**: Code discussion
- **Security Issues**: Send email to security@project.org

### Recognition
Contributors will be recognized in:
- CONTRIBUTORS.md file
- Release notes for significant contributions
- GitHub contributor statistics

## Release Process

### Versioning
We use Semantic Versioning (SemVer):
- `MAJOR.MINOR.PATCH`
- MAJOR: Breaking changes
- MINOR: New features, backward compatible
- PATCH: Bug fixes, backward compatible

### Release Checklist
1. Update version numbers
2. Update CHANGELOG.md
3. Run full test suite
4. Build release packages
5. Tag release
6. Update documentation
7. Announce release

## Getting Help

### Documentation
- [README.md](../README.md): Quick start guide
- [docs/DESIGN.md](DESIGN.md): System architecture
- [docs/ROADMAP.md](ROADMAP.md): Future plans
- [docs/SECURITY.md](SECURITY.md): Security considerations

### Support
- GitHub Issues: Technical questions
- GitHub Discussions: General questions
- Stack Overflow: Tag with `industrial-remote-expert`

Thank you for contributing to the Industrial Remote Expert System!