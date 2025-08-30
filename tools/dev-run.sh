#!/bin/bash
# ===============================================
# tools/dev-run.sh
# Development script to launch server + 2 clients for local demo
# ===============================================

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

echo "Industrial Remote Expert - Development Demo"
echo "==========================================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
SERVER_PORT=9000
DATABASE_PATH="/tmp/industrial_demo.db"
LOG_LEVEL="debug"

# Function to print colored output
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to kill background processes on exit
cleanup() {
    print_status "Cleaning up processes..."
    if [ ! -z "$SERVER_PID" ]; then
        kill $SERVER_PID 2>/dev/null || true
    fi
    if [ ! -z "$FACTORY_PID" ]; then
        kill $FACTORY_PID 2>/dev/null || true
    fi
    if [ ! -z "$EXPERT_PID" ]; then
        kill $EXPERT_PID 2>/dev/null || true
    fi
    
    # Clean up database
    rm -f "$DATABASE_PATH" 2>/dev/null || true
    
    print_status "Cleanup complete"
}

# Set up signal handlers
trap cleanup EXIT INT TERM

# Function to build component
build_component() {
    local component=$1
    local component_dir="$PROJECT_DIR/$component"
    
    print_status "Building $component..."
    
    if [ ! -d "$component_dir" ]; then
        print_error "Directory $component_dir not found"
        return 1
    fi
    
    cd "$component_dir"
    
    # Clean and build
    make clean >/dev/null 2>&1 || true
    qmake >/dev/null 2>&1
    
    if ! make -j$(nproc) >/dev/null 2>&1; then
        print_error "Failed to build $component"
        return 1
    fi
    
    print_status "$component built successfully"
    return 0
}

# Function to wait for server to be ready
wait_for_server() {
    local max_attempts=10
    local attempt=0
    
    print_status "Waiting for server to start..."
    
    while [ $attempt -lt $max_attempts ]; do
        if nc -z localhost $SERVER_PORT 2>/dev/null; then
            print_status "Server is ready on port $SERVER_PORT"
            return 0
        fi
        
        attempt=$((attempt + 1))
        sleep 1
    done
    
    print_error "Server failed to start within $max_attempts seconds"
    return 1
}

# Function to start server
start_server() {
    local server_binary="$PROJECT_DIR/server/server"
    
    if [ ! -x "$server_binary" ]; then
        print_error "Server binary not found: $server_binary"
        return 1
    fi
    
    print_status "Starting server..."
    
    # Remove old database
    rm -f "$DATABASE_PATH" 2>/dev/null || true
    
    # Start server in background
    "$server_binary" \
        --port $SERVER_PORT \
        --database "$DATABASE_PATH" \
        --$LOG_LEVEL \
        --heartbeat 30 \
        --timeout 90 \
        > /tmp/server.log 2>&1 &
    
    SERVER_PID=$!
    
    if ! wait_for_server; then
        print_error "Server startup failed. Check /tmp/server.log for details"
        return 1
    fi
    
    print_status "Server started (PID: $SERVER_PID)"
    return 0
}

# Function to start client
start_client() {
    local client_type=$1
    local client_binary="$PROJECT_DIR/client-$client_type/client-$client_type"
    
    if [ ! -x "$client_binary" ]; then
        print_error "Client binary not found: $client_binary"
        return 1
    fi
    
    print_status "Starting $client_type client..."
    
    # Check if display is available
    if [ -z "$DISPLAY" ]; then
        print_warning "No DISPLAY set, client UI may not be visible"
    fi
    
    # Start client in background
    "$client_binary" > "/tmp/client-$client_type.log" 2>&1 &
    
    if [ "$client_type" = "factory" ]; then
        FACTORY_PID=$!
        print_status "Factory client started (PID: $FACTORY_PID)"
    else
        EXPERT_PID=$!
        print_status "Expert client started (PID: $EXPERT_PID)"
    fi
    
    return 0
}

# Function to show status
show_status() {
    echo
    print_status "System Status:"
    echo "  Server:        Running on port $SERVER_PORT (PID: $SERVER_PID)"
    echo "  Database:      $DATABASE_PATH"
    echo "  Factory Client: Running (PID: $FACTORY_PID)"
    echo "  Expert Client:  Running (PID: $EXPERT_PID)"
    echo
    echo "  Logs:"
    echo "    Server:        /tmp/server.log"
    echo "    Factory Client: /tmp/client-factory.log"
    echo "    Expert Client:  /tmp/client-expert.log"
    echo
    echo "  Usage:"
    echo "    1. Connect both clients to localhost:$SERVER_PORT"
    echo "    2. Join the same room (e.g., 'DEMO123')"
    echo "    3. Start chatting and testing features"
    echo
    echo "  Press Ctrl+C to stop all processes"
}

# Function to show real-time logs
show_logs() {
    if command_exists "multitail"; then
        print_status "Showing real-time logs (press 'q' to exit)..."
        multitail -s 3 \
            -T 2 -t "Server" /tmp/server.log \
            -T 2 -t "Factory" /tmp/client-factory.log \
            -T 2 -t "Expert" /tmp/client-expert.log
    else
        print_status "Install 'multitail' for better log viewing"
        print_status "Showing server log (press Ctrl+C to stop)..."
        tail -f /tmp/server.log
    fi
}

# Main execution
main() {
    cd "$PROJECT_DIR"
    
    # Check prerequisites
    print_status "Checking prerequisites..."
    
    if ! command_exists "qmake"; then
        print_error "qmake not found. Please install Qt development tools."
        exit 1
    fi
    
    if ! command_exists "make"; then
        print_error "make not found. Please install build tools."
        exit 1
    fi
    
    if ! command_exists "nc"; then
        print_warning "netcat (nc) not found. Server readiness check may not work."
    fi
    
    # Parse command line arguments
    BUILD_ONLY=false
    LOGS_ONLY=false
    
    for arg in "$@"; do
        case $arg in
            --build-only)
                BUILD_ONLY=true
                shift
                ;;
            --logs)
                LOGS_ONLY=true
                shift
                ;;
            --help|-h)
                echo "Usage: $0 [options]"
                echo ""
                echo "Options:"
                echo "  --build-only    Build components but don't run them"
                echo "  --logs          Show real-time logs (requires running system)"
                echo "  --help, -h      Show this help message"
                echo ""
                echo "This script builds and runs the complete Industrial Remote Expert system"
                echo "for local development and testing."
                exit 0
                ;;
            *)
                print_error "Unknown option: $arg"
                exit 1
                ;;
        esac
    done
    
    # Show logs if requested
    if [ "$LOGS_ONLY" = true ]; then
        show_logs
        exit 0
    fi
    
    # Build all components
    print_status "Building all components..."
    
    if ! build_component "server"; then
        exit 1
    fi
    
    if ! build_component "client-factory"; then
        exit 1
    fi
    
    if ! build_component "client-expert"; then
        exit 1
    fi
    
    print_status "All components built successfully"
    
    # Exit if build-only
    if [ "$BUILD_ONLY" = true ]; then
        print_status "Build complete. Use '$0' to run the system."
        exit 0
    fi
    
    # Start all components
    if ! start_server; then
        exit 1
    fi
    
    # Give server a moment to fully initialize
    sleep 2
    
    if ! start_client "factory"; then
        cleanup
        exit 1
    fi
    
    if ! start_client "expert"; then
        cleanup
        exit 1
    fi
    
    # Show status and wait
    show_status
    
    # Wait for user interrupt
    print_status "Demo system is running. Press Ctrl+C to stop."
    while true; do
        sleep 1
    done
}

# Run main function
main "$@"