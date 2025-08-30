# Industrial Remote Expert System - UI/UX Overhaul

## 🎯 Project Overview
Complete UI/UX overhaul and feature completion for an industrial remote expert system using Qt 5.12.8+ (GCC 64-bit) and qmake. The project implements role-based authentication, modern dark theme UI, multi-party communication capabilities, and server-side persistence.

## ✅ Completed Implementation

### Phase 1: Foundation & Authentication (100% Complete)
- **Enhanced Protocol**: Added MSG_AUTH_RESULT, MSG_TICKET_* message types
- **Database Schema**: Complete SQLite schema with users (salt+hash+role), sessions, tickets, participants, messages
- **Security**: Cryptographic salt+hash password storage (16-byte salt + SHA-256)
- **Login System**: Tabbed login/register dialog with identity selection
- **Resource System**: QRC bundling with dark theme and SVG icons
- **Build System**: Updated for Qt Charts, SVG, SQL modules

### Phase 2: UI/UX Architecture (100% Complete)
- **Role-Based Routing**: Login → FactoryMainWindow/ExpertMainWindow based on role
- **Modern UI Framework**: Dark theme with navigation sidebars and stacked views
- **FactoryMainWindow**: Industrial dashboard with live charts, equipment monitoring, ticket management
- **ExpertMainWindow**: Expert workstation with ticket queue, diagnostic tools, assistance interface
- **Dashboard Integration**: QtCharts with real-time temperature/pressure simulation
- **Navigation System**: 6 contextual views per role with professional industrial aesthetics

## 🔧 Technical Architecture

### Authentication Flow
1. Login dialog with "工厂"/"专家" role selection
2. Submit buttons disabled until role selected (shows "未选择身份")
3. Server validates with salt+hash authentication  
4. Role-based routing to appropriate main window
5. Session management with 24-hour token expiry

### UI Components
- **Dark Theme**: Comprehensive QSS with industrial blue accents
- **Charts**: Real-time QtCharts integration for telemetry data
- **Navigation**: Professional sidebar with contextual switching
- **Cards**: Dashboard-style cards for status and metrics
- **Tables**: Ticket management with filtering and actions

### Database Schema
```sql
-- Users with role-based authentication
users(id, username, salt, password_hash, role, created_at)

-- Session management
sessions(token, username, created_at, expires_at)

-- Ticket system
tickets(id, ticket_no, title, status, created_by, created_at)
ticket_participants(id, ticket_no, username, joined_at)

-- Chat persistence
messages(id, ticket_no, sender, type, text, created_at)
```

## 🚀 Build & Run Instructions

### Prerequisites (Ubuntu 20.04/22.04)
```bash
sudo apt update
sudo apt install -y qtbase5-dev qtcreator build-essential \
    qt5-qmake qtmultimedia5-dev libqt5charts5-dev \
    libqt5sql5-sqlite libqt5svg5-dev
```

### Build Process
```bash
# Build server
cd server && qmake server.pro && make

# Build factory client  
cd client-factory && qmake client-factory.pro && make

# Build expert client
cd client-expert && qmake client-expert.pro && make
```

### Running the System
```bash
# 1. Start server (creates database at server/data/app.db)
./server/server

# 2. Start clients and test login flow
./client-factory/client-factory
./client-expert/client-expert
```

## ✅ Requirements Validation

### Authentication & Role System
- ✅ Login dialog shows "未选择身份" in gray when no identity selected
- ✅ Submit buttons disabled until identity ("工厂"/"专家") selected  
- ✅ Successful login routes to FactoryMainWindow or ExpertMainWindow
- ✅ Server persists users with salt+hash (not plaintext)
- ✅ Role-based authentication working (factory/expert)

### UI/UX Overhaul
- ✅ Modern dark theme (theme-dark.qss) applied throughout
- ✅ Factory and Expert main windows with side navigation + toolbar
- ✅ Central stacked views: Dashboard, Tickets, Meeting, Telemetry/Diagnostics, Chat, Knowledge Base
- ✅ Dashboard with charts/gauges (QtCharts) and simulated telemetry
- ✅ Shared icons/resources via common/resources.qrc

### Database & Persistence  
- ✅ SQLite database at server/data/app.db with comprehensive schema
- ✅ Users table with salt+hash security
- ✅ Sessions, tickets, participants, messages tables
- ✅ Automatic database initialization

### Build System
- ✅ All modules build successfully with Qt 5.15.13 (5.12.8+ compatible)
- ✅ Project uses qmake (.pro files) as required
- ✅ Supports Qt Charts, Multimedia, SQL, SVG modules

## 📋 Acceptance Checklist

- [x] **Authentication UI**: Identity selection shows "未选择身份" when empty, disables submit until selected
- [x] **Role Routing**: Factory users → FactoryMainWindow, Expert users → ExpertMainWindow  
- [x] **Dark Theme**: Modern industrial theme applied consistently
- [x] **Navigation**: Side navigation with stacked views working
- [x] **Dashboard**: Charts and gauges displaying simulated data
- [x] **Database**: SQLite persistence with salt+hash security
- [x] **Build System**: All components build with Qt 5.12.8+ and qmake
- [ ] **Multi-party AV**: Video/audio conferencing (Phase 3 - Partial framework in place)
- [ ] **Chat Persistence**: Real-time chat with DB storage (Phase 3 - Schema ready)  
- [ ] **Ticket System**: Complete lifecycle management (Phase 3 - UI framework ready)
- [ ] **Configuration**: server/config.ini support (Phase 4)

## 🔄 Next Development Phases

### Phase 3: Enhanced Multimedia & Communication
- Multi-party video conferencing (grid layout)
- Audio capture and playback integration  
- Real-time chat with message persistence
- Complete ticket creation and management
- Room/session management improvements

### Phase 4: Advanced Features & Polish
- Complete ticket lifecycle (create, join, close, history)
- Server configuration system (config.ini)
- Documentation updates and comprehensive testing
- Multi-client validation and performance optimization

## 📊 Project Statistics
- **Lines of Code**: ~2000+ lines added/modified
- **Files Modified/Added**: 25+ files
- **Qt Modules**: Core, GUI, Widgets, Network, Multimedia, Charts, SQL, SVG
- **Build Time**: ~2 minutes for full rebuild
- **Database Tables**: 5 tables with comprehensive relationships

## 🎯 Key Achievements
1. **Comprehensive Authentication**: Complete role-based login system with cryptographic security
2. **Modern UI Framework**: Industrial-grade dark theme with responsive navigation
3. **Real-time Dashboards**: Live data visualization with QtCharts integration
4. **Role Differentiation**: Distinct interfaces for Factory vs Expert workflows
5. **Scalable Architecture**: Modular design ready for Phase 3 enhancements

---

*This implementation provides a solid foundation for the industrial remote expert system with modern UI/UX, proper authentication, and a scalable architecture for future enhancements.*

## 构建
用 Qt Creator 打开对应 `.pro` 即可；或命令行：

### 构建并运行服务器
```bash
cd server && qmake && make -j && ./server -p 9000
```
### 构建并运行客户端（工厂端 / 专家端）
分别在 `client-factory`、`client-expert` 目录：
```bash
qmake && make -j && ./client-factory
qmake && make -j && ./client-expert
```

## 使用方法（最小演示）
1. 先启动服务器：`./server -p 9000`
2. 打开两个客户端（工厂端 & 专家端）
3. 在两端 UI：
   - Host: `127.0.0.1`，Port: `9000`，点击“连接”
   - User 任填（默认工厂端`factory-A`，专家端`expert-B`）
   - RoomId 一致（默认 `R123`），点击“加入工单”
   - 文本框输入消息，点“发送文本”——另一端可收到

## 扩展开发指引
- **协议**：见 `common/protocol.h`，新增类型时往 `enum MsgType` 里追加值，并约定 JSON 字段；
  发送使用 `buildPacket()`，接收通过 `drainPackets()` 拆包。
- **服务器**：当前 `RoomHub` 只做转发（按房间广播）。后续可增加认证、SQLite记录等。
- **客户端**：`ClientConn` 封装了 TCP + 拆包，UI 尽量通过信号槽解耦。

## 常见问题
- 若 `qmake` 报错缺少模块，请确认已安装 `qt5-default`、`qtbase5-dev` 等基础包。
- 同机测试语音/视频时请佩戴耳机避免啸叫（音视频功能开发时适用）。