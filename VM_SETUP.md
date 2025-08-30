# VM Setup Instructions for Industrial Remote Expert System v2.0

This document provides comprehensive instructions for setting up a virtual machine to build and test the Industrial Remote Expert System.

## VM Requirements

### Recommended VM Specifications
- **OS**: Ubuntu 22.04 LTS (Desktop or Server)
- **RAM**: 4GB minimum, 8GB recommended
- **Storage**: 20GB minimum, 30GB recommended
- **CPU**: 2 cores minimum, 4 cores recommended
- **Graphics**: Enable 3D acceleration for UI testing (if using desktop version)

### VM Software Options
- **VirtualBox** (Free, cross-platform)
- **VMware Workstation/Player** (Commercial/Free)
- **Hyper-V** (Windows)
- **QEMU/KVM** (Linux)

## Ubuntu VM Setup

### 1. Create Ubuntu VM
1. Download Ubuntu 22.04 LTS from https://ubuntu.com/download
2. Create new VM with specifications above
3. Install Ubuntu with default settings
4. Update system after installation:
   ```bash
   sudo apt update && sudo apt upgrade -y
   ```

### 2. Install Development Tools
```bash
# Essential development packages
sudo apt install -y build-essential git curl wget

# Qt5 Development Environment
sudo apt install -y \
    qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools \
    qtcreator qtmultimedia5-dev libqt5charts5-dev \
    libsqlite3-dev libqt5sql5-sqlite

# Additional useful tools
sudo apt install -y \
    sqlite3 netcat-openbsd tree htop \
    gdb valgrind
```

### 3. Verify Installation
```bash
# Check Qt version
qmake --version

# Check compiler
gcc --version

# Check SQLite
sqlite3 --version
```

## Building the System in VM

### 1. Clone Repository
```bash
cd ~
git clone https://github.com/KirisameMarisa0602/industrial-remote-expert.git
cd industrial-remote-expert
```

### 2. Run Automated Test
```bash
# Make test script executable and run
chmod +x test_system.sh
./test_system.sh
```

### 3. Manual Build (Alternative)
```bash
# Build server
cd server
qmake -qt5
make -j$(nproc)

# Build expert client
cd ../client-expert
qmake -qt5
make -j$(nproc)

# Build factory client
cd ../client-factory
qmake -qt5
make -j$(nproc)
```

## Testing in VM Environment

### 1. Server Testing
```bash
# Start server
cd ~/industrial-remote-expert/server
./server -p 9000

# In another terminal, test connectivity
nc -v 127.0.0.1 9000
```

### 2. Database Verification
```bash
# Check database creation
ls -la ~/industrial-remote-expert/server/data/

# Examine database schema
sqlite3 ~/industrial-remote-expert/server/data/server.db
.tables
.schema users
.quit
```

### 3. Client Testing (GUI)
If using Ubuntu Desktop VM:
```bash
# Start server first
cd ~/industrial-remote-expert/server
./server -p 9000 &

# Start expert client
cd ~/industrial-remote-expert/client-expert
./client-expert &

# Start factory client
cd ~/industrial-remote-expert/client-factory
./client-factory &
```

### 4. Headless Testing
For server VM or headless testing:
```bash
# Use Xvfb for virtual display
sudo apt install -y xvfb

# Start virtual display
export DISPLAY=:99
Xvfb :99 -screen 0 1024x768x24 &

# Run clients
cd ~/industrial-remote-expert/client-expert
./client-expert &
```

## VM Networking Configuration

### 1. Port Forwarding (VirtualBox Example)
For testing from host machine:
1. VM Settings → Network → Advanced → Port Forwarding
2. Add rule:
   - Name: IRE-Server
   - Protocol: TCP
   - Host Port: 9000
   - Guest Port: 9000

### 2. Bridged Network
For multi-VM testing:
1. Change network adapter to "Bridged Adapter"
2. Note VM IP address: `ip addr show`
3. Connect clients from other VMs using VM IP

## Performance Optimization

### 1. VM Settings
- **VirtualBox**:
  - Enable VT-x/AMD-V
  - Enable PAE/NX
  - Allocate max CPU cores
  - Enable 3D acceleration

- **VMware**:
  - Enable hardware acceleration
  - Allocate sufficient RAM
  - Enable 3D graphics

### 2. Ubuntu Optimizations
```bash
# Install VM guest additions/tools
# VirtualBox:
sudo apt install virtualbox-guest-additions-iso
# VMware:
sudo apt install open-vm-tools open-vm-tools-desktop

# Reduce unnecessary services
sudo systemctl disable bluetooth
sudo systemctl disable cups-browsed
```

## Development Workflow in VM

### 1. Code Editing
```bash
# Install preferred editor
sudo apt install -y code  # VS Code
# or
sudo apt install -y vim-gtk3

# Open project
code ~/industrial-remote-expert
```

### 2. Qt Creator Setup
```bash
# Launch Qt Creator
qtcreator

# Open project files:
# - server/server.pro
# - client-expert/client-expert.pro
# - client-factory/client-factory.pro
```

### 3. Debugging
```bash
# Build debug version
cd ~/industrial-remote-expert/server
qmake CONFIG+=debug
make

# Run with debugger
gdb ./server
(gdb) set args -p 9000
(gdb) run
```

## VM Snapshot Management

### 1. Create Snapshots
Create snapshots at key stages:
1. **Base Ubuntu**: After Ubuntu installation and updates
2. **Dev Environment**: After installing development tools
3. **Working System**: After successful build and test

### 2. Snapshot Strategy
- Take snapshot before major changes
- Name snapshots descriptively
- Document what each snapshot contains

## Multi-VM Testing Setup

### 1. Server VM
- Ubuntu Server 22.04
- Minimal install
- Server application only
- Static IP or port forwarding

### 2. Client VMs
- Ubuntu Desktop 22.04
- GUI clients for testing
- Connect to server VM

### 3. Testing Scenarios
```bash
# Scenario 1: Expert and Factory on different VMs
VM1: Server + Expert client
VM2: Factory client

# Scenario 2: Multiple experts
VM1: Server
VM2: Expert client 1
VM3: Expert client 2
VM4: Factory client

# Scenario 3: Load testing
VM1: Server
VM2-VM5: Multiple clients
```

## Troubleshooting VM Issues

### 1. Build Issues
```bash
# Missing packages
sudo apt install -y qtbase5-dev-tools

# Qt version conflicts
sudo update-alternatives --config qmake

# Permission issues
chmod +x test_system.sh
```

### 2. Runtime Issues
```bash
# Database permissions
chmod 755 ~/industrial-remote-expert/server/data
chmod 644 ~/industrial-remote-expert/server/data/server.db

# Network connectivity
sudo ufw allow 9000/tcp
```

### 3. GUI Issues
```bash
# X11 forwarding for SSH
ssh -X user@vm-ip

# Display issues
export DISPLAY=:0
xhost +local:
```

## VM Backup and Distribution

### 1. Prepare VM for Distribution
```bash
# Clean system
sudo apt autoremove -y
sudo apt autoclean

# Clear history
history -c
rm ~/.bash_history

# Zero free space (optional, for compression)
sudo dd if=/dev/zero of=/tmp/zero bs=1M
sudo rm /tmp/zero
```

### 2. Export VM
- VirtualBox: File → Export Appliance → OVF format
- VMware: File → Export to OVF

### 3. Documentation
Include with VM distribution:
- This setup guide
- README_v2.md
- Test results
- Known issues

## Conclusion

This VM setup provides a complete development and testing environment for the Industrial Remote Expert System. The automated test script validates that all components work correctly in the VM environment.

For questions or issues with VM setup, refer to the main project documentation or create an issue in the GitHub repository.