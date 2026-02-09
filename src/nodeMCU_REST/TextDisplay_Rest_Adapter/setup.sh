#!/bin/bash
# setup.sh - Cross-platform setup script for Unix-like systems (macOS, Linux, WSL)
#
# This script helps set up the development environment for this PlatformIO project
# Run this after cloning the repository

set -e  # Exit on error

echo "========================================="
echo "TextDisplay Rest Adapter - Setup Script"
echo "========================================="
echo ""

# Check if we're in the correct directory
if [ ! -f "platformio.ini" ]; then
    echo "❌ Error: platformio.ini not found!"
    echo "   Please run this script from the project root directory."
    exit 1
fi

echo "✓ Found platformio.ini"
echo ""

# Check for Python
echo "Checking for Python..."
if command -v python3 &> /dev/null; then
    PYTHON_VERSION=$(python3 --version)
    echo "✓ $PYTHON_VERSION installed"
elif command -v python &> /dev/null; then
    PYTHON_VERSION=$(python --version)
    echo "✓ $PYTHON_VERSION installed"
else
    echo "❌ Python not found!"
    echo "   Please install Python 3.x from python.org"
    exit 1
fi
echo ""

# Check for PlatformIO
echo "Checking for PlatformIO..."
if command -v pio &> /dev/null; then
    PIO_VERSION=$(pio --version)
    echo "✓ PlatformIO $PIO_VERSION installed"
else
    echo "⚠️  PlatformIO CLI not found"
    echo "   Install it with: pip install -U platformio"
    echo "   Or use the VS Code PlatformIO extension"
fi
echo ""

# Check for secrets.h
echo "Checking for secrets.h..."
if [ -f "src/secrets.h" ]; then
    echo "✓ src/secrets.h exists"
else
    echo "⚠️  src/secrets.h not found"
    if [ -f "src/secrets.h.template" ]; then
        echo "   Creating from template..."
        cp src/secrets.h.template src/secrets.h
        echo "✓ Created src/secrets.h from template"
        echo ""
        echo "⚠️  IMPORTANT: Edit src/secrets.h with your credentials!"
        echo "   Update: WiFi SSID, WiFi password, and other secrets"
    else
        echo "❌ Template src/secrets.h.template not found!"
        exit 1
    fi
fi
echo ""

# Offer to install PlatformIO dependencies
echo "========================================="
echo "Next Steps:"
echo "========================================="
echo ""
echo "1. Edit src/secrets.h with your credentials (if not done already)"
echo "2. Build the project:"
echo "   pio run -e texteck-esp32-debug"
echo ""
echo "Or use VS Code with PlatformIO extension:"
echo "   - Open this folder in VS Code"
echo "   - Click PlatformIO icon in sidebar"
echo "   - Select environment and click Build"
echo ""
echo "For more details, see PLATFORM_SETUP.md"
echo ""
echo "✓ Setup complete!"
