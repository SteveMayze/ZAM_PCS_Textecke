# Cross-Platform Development Guide

This project is designed to work seamlessly on both Windows and macOS. Follow this guide to ensure smooth operation across platforms.

## Prerequisites

Both platforms require:
- **Python 3.x** (for PlatformIO)
- **VS Code** with PlatformIO extension
- **Git** for version control

## Platform-Specific Setup

### Windows Setup
1. Install Python from [python.org](https://www.python.org/downloads/)
   - During installation, check "Add Python to PATH"
2. Install Git from [git-scm.com](https://git-scm.com/)
3. Install VS Code and the PlatformIO IDE extension
4. Open this project folder in VS Code
5. PlatformIO will automatically install required tools

### macOS Setup
1. Install Homebrew (if not already installed):
   ```bash
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   ```
2. Install Python and Git:
   ```bash
   brew install python git
   ```
3. Install VS Code and the PlatformIO IDE extension
4. Open this project folder in VS Code
5. PlatformIO will automatically install required tools

## First-Time Setup on Any Platform

After cloning the repository:

1. **PlatformIO will automatically**:
   - Download platform-specific toolchains
   - Install library dependencies
   - Generate `.vscode/c_cpp_properties.json` with correct paths for your system

2. **Create your `secrets.h` file**:
   ```bash
   # Copy the template (if one exists) or create from scratch
   cp src/secrets.h.template src/secrets.h  # If template exists
   ```
   Edit `src/secrets.h` with your credentials (this file is git-ignored)

3. **First build**:
   - Click the PlatformIO icon in VS Code sidebar
   - Select your target environment (e.g., `texteck-esp32-debug`)
   - Click "Build"
   - PlatformIO will download all dependencies automatically

## What's Git-Ignored (Platform-Specific Files)

These files are automatically generated and differ between Windows/Mac:
- `.pio/` - Build artifacts and downloaded tools
- `.vscode/c_cpp_properties.json` - IntelliSense paths
- `.vscode/launch.json` - Debugging configurations
- `secrets.h` - Private credentials

**Never commit these files!** Each platform regenerates them automatically.

## Building & Uploading

### Building
```bash
# Using PlatformIO CLI (works on both platforms)
pio run -e texteck-esp32-debug

# Or use VS Code PlatformIO GUI:
# 1. Click PlatformIO icon
# 2. Select environment
# 3. Click "Build"
```

### Uploading to Device
```bash
# Using PlatformIO CLI
pio run -e texteck-esp32-debug --target upload

# Or use VS Code PlatformIO GUI:
# 1. Connect ESP device via USB
# 2. Select environment
# 3. Click "Upload"
```

### Serial Monitor
```bash
# Using PlatformIO CLI
pio device monitor

# Or use VS Code PlatformIO GUI:
# Click "Monitor" button
```

## Troubleshooting

### Issue: Build fails on Windows after pulling from Mac (or vice versa)
**Solution**: PlatformIO's auto-generated files may need refresh
```bash
# Clean the build
pio run --target clean

# Delete .pio folder entirely (it will be regenerated)
rm -rf .pio  # macOS/Linux
# or
rmdir /s .pio  # Windows CMD

# Rebuild
pio run -e texteck-esp32-debug
```

### Issue: IntelliSense shows errors but build succeeds
**Solution**: Regenerate `c_cpp_properties.json`
```bash
# Delete the file - PlatformIO will regenerate it
rm .vscode/c_cpp_properties.json  # macOS/Linux
# or
del .vscode\c_cpp_properties.json  # Windows

# Then rebuild project
pio run -e texteck-esp32-debug
```

### Issue: Serial port not detected
**Solution**: 
- **Windows**: Install CH340/CP2102 USB drivers from device manufacturer
- **macOS**: Some USB drivers install automatically; check System Preferences → Security if blocked

## Available Build Environments

| Environment | Platform | Logger Level | Purpose |
|------------|----------|--------------|---------|
| `texteck-esp8266` | ESP8266 | - | ESP8266 board |
| `texteck-esp32-debug` | ESP32 | ALL | Development with full logging |
| `texteck-esp32-prod` | ESP32 | OFF | Production without logging |
| `texteck-esp32-zam-debug` | ESP32 | ALL | ZAM deployment with logging |
| `texteck-esp32-zam-prod` | ESP32 | OFF | ZAM production deployment |

## Best Practices for Cross-Platform Development

1. **Use PlatformIO paths**: Never hardcode absolute paths in code
2. **Commit platformio.ini**: This file IS cross-platform safe
3. **Keep secrets.h local**: Use environment-based secrets, never commit
4. **Let PlatformIO manage dependencies**: Don't manually copy libraries
5. **Use forward slashes in code**: Even on Windows, C++ accepts `/`
6. **Test on both platforms periodically**: Catch issues early

## Syncing Between Machines

When switching between Windows and Mac:

1. **Commit and push** from current machine:
   ```bash
   git add .
   git commit -m "Your changes"
   git push
   ```

2. **On the other machine**, pull and rebuild:
   ```bash
   git pull
   pio run --target clean
   pio run -e texteck-esp32-debug
   ```

3. **Verify secrets.h exists** on the new machine (create if needed)

That's it! PlatformIO handles all the platform-specific details automatically.
