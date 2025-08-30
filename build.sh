#!/bin/bash
# Build script for Industrial Remote Expert

echo "Building Industrial Remote Expert..."

# Clean any previous builds
echo "Cleaning previous builds..."
find . -name "*.o" -delete
find . -name "moc_*.cpp" -delete
find . -name "Makefile" -delete

# Build all components
echo "Building shared library..."
cd shared && qmake && make -j4 || exit 1
cd ..

echo "Building common library..."
cd common && qmake && make -j4 || exit 1
cd ..

echo "Building server..."
cd server && qmake && make -j4 || exit 1
cd ..

echo "Building expert client..."
cd client-expert && qmake && make -j4 || exit 1
cd ..

echo "Building factory client..."
cd client-factory && qmake && make -j4 || exit 1
cd ..

echo "Build completed successfully!"
echo ""
echo "To run the applications:"
echo "1. Start server: cd server && ./server"
echo "2. Start expert client: cd client-expert && ./client-expert"
echo "3. Start factory client: cd client-factory && ./client-factory"