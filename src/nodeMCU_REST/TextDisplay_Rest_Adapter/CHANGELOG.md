# CHANGELOG

## 2025-12-22 — Recent fixes

- Fix: Removed global `StaticJsonDocument<1024>` and switched to per-request `JsonVariant` handling to reduce static RAM pressure.
- Fix: Added informative `LOG_INFO_F` messages when incoming parameters are truncated (POST form and REST API).
- Fix: Removed per-byte debug prints in `render_and_send`; added a bounded hex preview to avoid blocking Serial and triggering the Soft WDT.
- Fix: `html_processor` debug output now logs a 64-char preview plus the full message length instead of printing entire message.
- Improvement: Increased logging UART speed: logging `Serial` now initialized at 115200; updated `platformio.ini` `monitor_speed` to 115200.
- Safety: Replaced unsafe `strcpy`/`strcat` usages with bounded copies (`toCharArray`, `strncpy`, `memcpy`).

Files changed (high level):
- `src/main.cpp` — multiple edits (bounded copies, truncation logs, reduced debug output, JSON handling, baud change)
- `platformio.ini` — `monitor_speed` set to `115200`

Notes:
- Confirm serial monitor is set to 115200 after flashing.
- If you want me to commit these changes to git now, I will run `git add .` and `git commit` in the project folder.

- Commit: Staged and committed `src/main.cpp` and `CHANGELOG.md` with final safe-string handling and logging/truncation updates.
