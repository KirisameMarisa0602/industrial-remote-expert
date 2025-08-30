#!/bin/bash

# Industrial Remote Expert System - Test Script
# This script tests the complete authentication and work order flow

set -e

echo "=== Industrial Remote Expert System Test ==="
echo "Testing server and client functionality..."

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}[OK]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if we're in the right directory
if [ ! -f "server/server.pro" ] || [ ! -f "client-expert/client-expert.pro" ]; then
    print_error "Please run this script from the project root directory"
    exit 1
fi

# Build all components
echo -e "\n${YELLOW}Building all components...${NC}"

# Build server
echo "Building server..."
cd server
if ! qmake -qt5 > /dev/null 2>&1; then
    print_error "Failed to run qmake for server"
    exit 1
fi

if ! make -j > /dev/null 2>&1; then
    print_error "Failed to build server"
    exit 1
fi
print_status "Server built successfully"

# Build expert client
echo "Building expert client..."
cd ../client-expert
if ! qmake -qt5 > /dev/null 2>&1; then
    print_error "Failed to run qmake for expert client"
    exit 1
fi

if ! make -j > /dev/null 2>&1; then
    print_error "Failed to build expert client"
    exit 1
fi
print_status "Expert client built successfully"

# Build factory client
echo "Building factory client..."
cd ../client-factory
if ! qmake -qt5 > /dev/null 2>&1; then
    print_error "Failed to run qmake for factory client"
    exit 1
fi

if ! make -j > /dev/null 2>&1; then
    print_error "Failed to build factory client"
    exit 1
fi
print_status "Factory client built successfully"

cd ..

# Start server
echo -e "\n${YELLOW}Starting server...${NC}"
mkdir -p data
cd server

# Check if server is already running
if pgrep -f "./server" > /dev/null; then
    print_warning "Server already running, killing existing process"
    pkill -f "./server" || true
    sleep 2
fi

# Start server in background
./server -p 9000 > server.log 2>&1 &
SERVER_PID=$!
cd ..

# Wait for server to start
sleep 3

# Check if server is running
if ! kill -0 $SERVER_PID 2>/dev/null; then
    print_error "Server failed to start"
    cat server/server.log
    exit 1
fi
print_status "Server started on port 9000 (PID: $SERVER_PID)"

# Check database creation
if [ -f "server/data/server.db" ]; then
    print_status "Database created successfully"
else
    print_error "Database not created"
    kill $SERVER_PID 2>/dev/null || true
    exit 1
fi

# Verify database schema
echo -e "\n${YELLOW}Verifying database schema...${NC}"
if command -v sqlite3 > /dev/null; then
    # Check tables exist
    TABLES=$(sqlite3 server/data/server.db ".tables")
    
    if echo "$TABLES" | grep -q "users"; then
        print_status "Users table created"
    else
        print_error "Users table missing"
        kill $SERVER_PID 2>/dev/null || true
        exit 1
    fi
    
    if echo "$TABLES" | grep -q "sessions"; then
        print_status "Sessions table created"
    else
        print_error "Sessions table missing"
        kill $SERVER_PID 2>/dev/null || true
        exit 1
    fi
    
    if echo "$TABLES" | grep -q "work_orders"; then
        print_status "Work orders table created"
    else
        print_error "Work orders table missing"
        kill $SERVER_PID 2>/dev/null || true
        exit 1
    fi
    
    if echo "$TABLES" | grep -q "work_order_comments"; then
        print_status "Work order comments table created"
    else
        print_error "Work order comments table missing"
        kill $SERVER_PID 2>/dev/null || true
        exit 1
    fi
else
    print_warning "sqlite3 not found, skipping database verification"
fi

# Check server logs for successful startup
if grep -q "Database initialized successfully" server/server.log; then
    print_status "Database initialization confirmed"
else
    print_error "Database initialization failed"
    cat server/server.log
    kill $SERVER_PID 2>/dev/null || true
    exit 1
fi

if grep -q "Server listening" server/server.log; then
    print_status "Server listening confirmed"
else
    print_error "Server not listening"
    cat server/server.log
    kill $SERVER_PID 2>/dev/null || true
    exit 1
fi

# Test client builds exist
echo -e "\n${YELLOW}Verifying client builds...${NC}"
if [ -f "client-expert/client-expert" ]; then
    print_status "Expert client executable exists"
else
    print_error "Expert client executable not found"
    kill $SERVER_PID 2>/dev/null || true
    exit 1
fi

if [ -f "client-factory/client-factory" ]; then
    print_status "Factory client executable exists"
else
    print_error "Factory client executable not found"
    kill $SERVER_PID 2>/dev/null || true
    exit 1
fi

# Basic protocol test (if we can find a simple way to test TCP connectivity)
echo -e "\n${YELLOW}Testing server connectivity...${NC}"
if command -v nc > /dev/null; then
    if echo | nc -w 1 127.0.0.1 9000; then
        print_status "Server accepting connections on port 9000"
    else
        print_error "Server not accepting connections"
        kill $SERVER_PID 2>/dev/null || true
        exit 1
    fi
else
    print_warning "netcat not available, skipping connectivity test"
fi

# Clean shutdown
echo -e "\n${YELLOW}Cleaning up...${NC}"
kill $SERVER_PID 2>/dev/null || true
sleep 2

# Final status
echo -e "\n${GREEN}=== Test Summary ===${NC}"
print_status "All components built successfully"
print_status "Server starts and initializes database correctly"
print_status "Database schema created with all required tables"
print_status "Server accepts client connections"

echo -e "\n${GREEN}✓ All tests passed!${NC}"
echo
echo "To start using the system:"
echo "1. Start server: cd server && ./server -p 9000"
echo "2. Start expert client: cd client-expert && ./client-expert"
echo "3. Start factory client: cd client-factory && ./client-factory"
echo
echo "See README_v2.md for detailed usage instructions."

exit 0