# TextDisplay REST Adapter

ESP32/ESP8266 REST adapter for text display control with OAuth authentication and database logging.

## 🚀 Quick Start

### Cross-Platform Setup (Windows & macOS)

This project works seamlessly on both Windows and macOS. The setup is automated:

**macOS/Linux:**
```bash
./setup.sh
```

**Windows:**
```cmd
setup.bat
```

The setup script will:
- Check for required tools (Python, PlatformIO)
- Create `secrets.h` from template
- Guide you through the next steps

### Manual Setup

1. **Copy the secrets template:**
   ```bash
   cp src/secrets.h.template src/secrets.h
   ```

2. **Edit `src/secrets.h`** with your credentials:
   - WiFi SSID and password
   - Oracle ORDS password (if using database logging)
   - OAuth credentials (if using OAuth)

3. **Build the project:**
   ```bash
   pio run -e texteck-esp32-debug
   ```

4. **Upload to device:**
   ```bash
   pio run -e texteck-esp32-debug --target upload
   ```

## 📋 Available Build Environments

| Environment | Board | Features | Use Case |
|------------|-------|----------|----------|
| `texteck-esp8266` | ESP8266 NodeMCU | Basic features | Legacy ESP8266 boards |
| `texteck-esp32-debug` | ESP32 | Full logging | Development on ESP32 |
| `texteck-esp32-prod` | ESP32 | No logging | Production ESP32 |
| `texteck-esp32-zam-debug` | ESP32 | PROD mode + logging | ZAM site development |
| `texteck-esp32-zam-prod` | ESP32 | PROD mode, no logging | ZAM site production |

## 🔧 Development

### Building for Different Environments

```bash
# ESP32 with debug logging
pio run -e texteck-esp32-debug

# ESP32 production (no logging)
pio run -e texteck-esp32-prod

# ZAM deployment with logging
pio run -e texteck-esp32-zam-debug
```

### Upload & Monitor

```bash
# Upload firmware
pio run -e texteck-esp32-debug --target upload

# Open serial monitor
pio device monitor

# Upload and monitor in one command
pio run -e texteck-esp32-debug --target upload --target monitor
```

## 🌐 Platform Independence

This project is designed to work on both Windows and macOS without modification:

✅ **What's cross-platform:**
- `platformio.ini` - Build configuration
- All source code
- Library dependencies
- Build targets

❌ **What's NOT committed (platform-specific):**
- `.pio/` - Build artifacts
- `.vscode/c_cpp_properties.json` - Auto-generated paths
- `secrets.h` - Private credentials

**See [PLATFORM_SETUP.md](PLATFORM_SETUP.md) for detailed cross-platform guide.**

## 📚 Documentation

- **[PLATFORM_SETUP.md](PLATFORM_SETUP.md)** - Cross-platform development guide
- **[DATABASE_LOGGING_GUIDE.md](DATABASE_LOGGING_GUIDE.md)** - Database logging setup
- **[OAUTH_IMPLEMENTATION.md](OAUTH_IMPLEMENTATION.md)** - OAuth authentication docs
- **[EVENT_QUEUE_IMPLEMENTATION.md](EVENT_QUEUE_IMPLEMENTATION.md)** - Event queue system
- **[TEMPLATE_ESCAPING_GUIDE.md](TEMPLATE_ESCAPING_GUIDE.md)** - Template placeholder escaping
- **[CHANGELOG.md](CHANGELOG.md)** - Version history

## 🔐 Security Notes

- **NEVER commit `secrets.h`** - It's git-ignored for a reason
- Use different credentials for development vs. production
- The `secrets.h.template` file IS safe to commit (contains no real credentials)

## 🛠️ Troubleshooting

### Build fails after switching machines?

```bash
# Clean and rebuild
pio run --target clean
pio run -e texteck-esp32-debug
```

### IntelliSense errors but build works?

The auto-generated `.vscode/c_cpp_properties.json` may need refresh:
```bash
# Delete and rebuild (it will regenerate)
rm .vscode/c_cpp_properties.json
pio run -e texteck-esp32-debug
```

### Can't find serial port?

- **Windows:** Install CH340/CP2102 USB drivers
- **macOS:** Check System Preferences → Security if blocked

## 📦 Features

- ✅ REST API for text display control
- ✅ OAuth 2.0 authentication (optional)
- ✅ Database logging to Oracle ORDS (optional)
- ✅ Event queue system
- ✅ Template placeholder escaping
- ✅ Multiple build configurations
- ✅ Cross-platform development (Windows/macOS)

## 🤝 Contributing

When developing across multiple machines:

1. Always commit and push before switching:
   ```bash
   git add .
   git commit -m "Description"
   git push
   ```

2. On the other machine, pull and rebuild:
   ```bash
   git pull
   pio run --target clean
   pio run -e texteck-esp32-debug
   ```

3. Ensure `secrets.h` exists on each machine (create from template if needed)

## 📝 License

[Your license here]

---

For detailed platform-specific instructions, see [PLATFORM_SETUP.md](PLATFORM_SETUP.md)
