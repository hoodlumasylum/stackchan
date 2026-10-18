[日本語](README.md)

# stackchan-idf

Firmware for Stack-chan running on M5Stack CoreS3 / AtomS3R / AtomS3 / StopWatch (C152),
written against ESP-IDF 5.5 / C++20. Supports AI voice conversation (OpenAI / Gemini /
XiaoZhi / xAI Grok), three configuration paths (BLE / Wi-Fi STA / SoftAP), and device-side OTA.

## Web Flasher / Settings page

Released firmware can be flashed straight from the browser (Chrome / Edge):

- **Flash**: <https://ciniml.github.io/stackchan-idf/>
- **BLE Settings**: <https://ciniml.github.io/stackchan-idf/settings.html> (Web Bluetooth, desktop Chrome / Edge only)
- **Wi-Fi Settings**: once the device is on Wi-Fi, `http://stackchan-XXXXXX.local/` (mDNS)
- **iOS / SoftAP Settings**: trigger AP mode on-device (per-board, see below), scan the
  Wi-Fi QR shown on the LCD with iPhone Camera → join → captive portal pops the
  settings page automatically

Push a tag `vX.Y.Z` and CI builds for all four boards, attaches the artifacts to
a Release, and the Pages site picks them up automatically.

## Supported boards

| Board | Slug | Display | Notable |
|---|---|---|---|
| CoreS3 + Stack-chan base | `cores3` | 320×240 IPS + touch | Default. 2 servos, head touch sensor, INA226 battery gauge |
| CoreS3 + Takao Base | `cores3` | same | Half-duplex servo on Port A; no servo VM control / no battery gauge |
| AtomS3R + Atomic ECHO BASE ("AtomNyan") | `atoms3r` | 128×128 LCD | No servos, ES8311 audio, BtnA-driven UI / AP toggle |
| AtomS3 (no PSRAM) + ECHO BASE | `atoms3` | 128×128 LCD | Slim profile, no conversation / RTP |
| M5 StopWatch (C152) | `stopwatch` | 466×466 round AMOLED + touch | No servos, gaze-follow on touch, ES8311 audio |

Build with `make build BOARD=<slug>` (default `cores3`). The board is detected at
boot and broadcast via `set_board_kind()` so UI tabs and feature toggles grey out
appropriately.

## Features

- **AI voice conversation**: WebSocket to one of OpenAI Realtime / Google Gemini
  Live / xAI Grok Voice / a XiaoZhi server, streaming mic input → reply audio with mouth-sync.
  The half-duplex CoreS3 mutes the mic while speaking; interrupt a reply
  (barge-in) with an LCD tap or a head touch (Si12T). Turns are detected by
  server-side VAD. OpenAI / Gemini / Grok drive expression, head pose and robotic
  speech via tools (`set_expression` / `set_head_pose` / `speak_katakoto`);
  XiaoZhi maps its reply emotion to an expression. Reply text streams into
  the balloon.
- **Avatar rendering**: 30 fps with M5GFX. Breath / saccade / blink animators,
  six expressions (Neutral / Happy / Sad / Angry / Doubt / Sleepy). The face
  layout and animation is driven by an **Avatar DSL** (`.avdsl` source →
  `.avbc` bytecode) that can be replaced live over BLE / Wi-Fi.
- **Mic-driven lip-sync**: FFT + per-band log + spectral flux estimate of mouth
  opening, with an EWMA noise-floor AGC for ambient drift.
- **Servos**: SCS0009 yaw + pitch over UART1 (1 Mbps). Trapezoidal-velocity
  `PathGenerator`; torque only engaged while moving. Per-board range
  calibration (ServoLimits) persisted in NVS.
- **Speaker / audio**: Boot arpeggio (C5–E5–G5, can be disabled), jtts random
  babble, AAC record + playback, BLE audio streaming, Wi-Fi RTP receive
  (L16 / μ-law / AAC). Volume is **0..200%**, live-controlled from BLE /
  Wi-Fi / on-device UI.
- **NeoPixel**: Nekomimi LED-strip animations (rainbow / solid / lip-sync
  level-meter mode).
- **LT timer**: Talk-time assistant. "Soon" notice N seconds before the end,
  on-the-dot announcement, repeated over-time call-outs (jtts).
- **On-device UI**:
  - CoreS3 / StopWatch (touch panel): tap top-right corner for a 5-tab UI
    (info / settings / control / range / conversation / LT)
  - AtomS3R / AtomS3 (button only): BtnA short-press toggles the status
    overlay, long-press cycles `operation_mode`
- **Speech balloon**: 24 px Japanese-capable Gothic font on a rounded white
  panel; long text scrolls right-to-left as a marquee.
- **BLE settings service** (NimBLE GATT): configure Wi-Fi, API keys and OTA
  from `tools/settings.html` (Web Bluetooth). Bluetooth 4.2+ Just Works
  pairing plus an application-layer X25519 + AES-256-GCM session, with
  optional password auth. Settings land in NVS; Apply reboots.
- **Wi-Fi settings service**: once Wi-Fi connects, the device serves an HTTP
  server (port 80) + a built-in `settings_wifi.html`, advertised over mDNS
  (`stackchan-XXXXXX.local`). Covers SSID, provider, API keys, system prompt,
  extra HTTP headers, jtts, Avatar DSL, OTA — everything the BLE page covers.
- **SoftAP provisioning (iOS-friendly)**: when STA is unset / failing, trigger
  AP mode on-device (`Stackchan-XXXXXX` + WPA2). LCD shows a Wi-Fi QR; the
  iPhone Camera scans → joins → **the captive portal automatically opens
  `settings_wifi.html`** (DNS hijack + HTTP 404 catch-all). `require_auth`
  is bypassed while AP is up, so the settings UI is immediately reachable.
- **OTA updates**: dual OTA partitions, boot verification with rollback.
  - **BLE**: settings.html → encrypted chunks
  - **Wi-Fi local file**: upload a `.bin` from settings_wifi.html
  - **Wi-Fi device-side fetch** (v0.7.4+): `POST /api/ota/release {tag}` —
    the device pulls its own per-board binary from GitHub Pages over its
    STA link and applies it (STA must be up).

## Hardware (CoreS3 + Stack-chan base path)

- M5Stack CoreS3 (ESP32-S3, 8 MB Quad-SPI PSRAM, 16 MB flash)
- Stack-chan base (PY32 IO expander @ 0x6F, two SCS0009 servos)
  - Internal I²C: AXP2101 (0x34) / touch (0x38) — managed by M5Unified
  - PY32 pin 0: servo motor-voltage enable (wait 200 ms after ON before using the bus)
  - Servo bus (SCS0009): UART1, TX GPIO 6 / RX GPIO 7, 1 Mbps, 8 N 1
    - Yaw  ID = 1, zero_pos = 460
    - Pitch ID = 2, zero_pos = 620
  - 1 step ≈ 0.3125° (`deg = (raw - zero) * 5 / 16`)
  - Head touch sensor Si12T @ 0x68 (3 zones, for nadenade / barge-in)
- Other boards' pin layout: see each `sdkconfig.defaults.<board>` and
  `components/board/board.cpp`.

## Setup

With ESP-IDF 5.5 installed (tested against 5.5.4; 5.4.2 still builds):

```sh
git clone <this repo>
cd stackchan-idf
git submodule update --init --recursive
tools/apply-m5-patches.sh                    # apply the one-line M5Unified fix
make set-target BOARD=cores3                 # first time only; per-board build dirs
make build      BOARD=cores3
make flash      BOARD=cores3 PORT=/dev/ttyACM0
make monitor    BOARD=cores3 PORT=/dev/ttyACM0
```

Replace `BOARD=` with `atoms3r` / `atoms3` / `stopwatch` to build for those
boards; each lands under its own `build-<board>/` directory.

`tools/apply-m5-patches.sh` just zero-initialises a `buf` array in
`M5Unified` `RTC_PowerHub_Class::setAlarmIRQ` so it stops tripping the
GCC 14 `-Werror=maybe-uninitialized` check.

API keys for OpenAI / Gemini / xAI are not baked into the build — supply them at
runtime from the BLE / Wi-Fi settings interface (stored in NVS). A
compile-time default can be given via `sdkconfig.defaults.local` (gitignored).

## Using Grok (xAI)

### Quick start (CoreS3, no build needed)

1. Download `stackchan-grok-cores3-en.bin` from the latest
   [release](https://github.com/hoodlumasylum/stackchan/releases).
2. Plug the CoreS3 into a computer with a USB-C cable and open
   <https://espressif.github.io/esptool-js/> in desktop Chrome or Edge.
   Press **Connect** and pick the device, press **Erase Flash** and wait, then
   add the `.bin` at address **0x0** and press **Program**. (If the board is not
   found, hold the reset button until the green LED blinks, then try again.)
3. Press the reset button. On first boot, with no Wi-Fi saved, Stack-chan starts
   its setup hotspot by itself and the screen shows (English):
   `Setup: scan QR or join Wi-Fi` with a QR code and
   `1. Join Wi-Fi: Stackchan-XXXXXX`, `2. Password: sc-xxxxxxxx`,
   `3. Open http://192.168.4.1`.
4. With your phone, scan the QR code (or join that Wi-Fi with that password).
   The setup page opens by itself, or open <http://192.168.4.1>.
5. On the **Setup** tab, fill in the three fields under **Quick setup**:
   **Wi-Fi name (SSID)**, **Wi-Fi password** and **xAI API key**
   (get one at <https://console.x.ai/>), then press the big green
   **Save & Restart** button.
6. Stack-chan restarts, joins your Wi-Fi and is ready to talk: the top-right
   badge goes *Connecting* → *Ready*; speak and it shows *Listening* →
   *Thinking* → *Speaking*.

Everything else is already set: the AI provider is **xAI Grok Voice**, the
operation mode is **Conversation**, the voice is **Ara**, the built-in
personality replies in **English** (you can replace it with your own
*System prompt* on the **Chat** tab), and the clock uses UK time. The whole
on-device menu and the settings pages are in English. To change Wi-Fi later:
tap the top-right corner of the screen to open the menu → **Control** tab →
**Wi-Fi setup (hotspot)**.

### How it works

Stack-chan can talk through the **xAI Grok Voice Agent**
(`wss://api.x.ai/v1/realtime`), which is OpenAI Realtime-compatible. It reuses
the OpenAI Realtime client, so tool calling (`set_expression` /
`set_head_pose` / `speak_katakoto` / `ask_agent` …) works the same way.

**Defaults**: model `grok-voice-latest`, voice `ara` (also `eve`, `rex`,
`sal`, `leo`, or a custom voice ID). Audio is G.711 µ-law @ 8 kHz both ways,
server-side VAD. Change model / voice at build time with
`CONFIG_STACKCHAN_XAI_REALTIME_MODEL` / `CONFIG_STACKCHAN_XAI_VOICE`
(`idf.py -B build-cores3 menuconfig` → *Stackchan Conversation*, or put them in
`sdkconfig.defaults.local`).

### Build & flash

The released binaries on the upstream Web Flasher do **not** contain Grok
support — build from this repo (ESP-IDF 5.5.x; verified with 5.5.5):

```sh
git clone --recurse-submodules https://github.com/hoodlumasylum/stackchan
cd stackchan
. ~/esp-idf/5.5.5/export.sh       # or wherever your ESP-IDF 5.5 lives
tools/apply-m5-patches.sh
make set-target BOARD=cores3      # first time only (atoms3r / stopwatch also work)
make build      BOARD=cores3
make flash      BOARD=cores3 PORT=/dev/ttyACM0   # macOS: /dev/cu.usbmodem*, Windows: COMx
make monitor    BOARD=cores3 PORT=/dev/ttyACM0   # optional: watch the log
```

(`make` expects ESP-IDF at `~/esp-idf/5.5.5`; override with
`make build IDF_PATH=/path/to/esp-idf`.) If the board is not detected, hold
the reset button until the green LED blinks to enter download mode.

### Configure Wi-Fi, xAI key and provider (other ways)

On a fresh / erased device the provider already defaults to Grok and the mode
to Conversation, so normally only Wi-Fi + the xAI key are needed (see Quick
start). Other paths:

- **Wi-Fi settings page** when the device is already on your network: open
  `http://stackchan-XXXXXX.local/` (or its IP, shown on the menu's **Info**
  tab). The **Setup** tab has the Quick setup fields; the AI provider, system
  prompt etc. are on the **Chat** tab.
- **BLE settings page**: serve this repo's copy locally (Web Bluetooth needs
  `https` or `localhost`): `cd tools && python3 -m http.server 8000`, open
  <http://localhost:8000/settings.html> in desktop Chrome / Edge, press
  **Connect (Bluetooth)**, fill in the same fields and press **Save & Restart**.
- **HTTP API** (scripting): `POST /api/xai-api-key` (body = key),
  `POST /api/provider` (body = `3`), then apply/restart as the page does.
- **Build-time fallback**: `CONFIG_STACKCHAN_XAI_API_KEY="xai-..."` in the
  gitignored `sdkconfig.defaults.local` is used when no key is stored in NVS.
  Never commit a key.

Provider values are `0` OpenAI, `1` Gemini, `2` XiaoZhi, `3` Grok (default).
The on-device **Chat** tab shows *Service: xAI Grok* when it is selected.

### Troubleshooting

- `credentials empty for selected provider` in the log → the xAI key is not
  stored; re-enter it and Save & restart.
- Stuck on "Connection error" / repeated reconnects → check the key is valid and has
  credit, the network allows outbound `wss://api.x.ai:443`, and the clock /
  TLS bundle is fine (`make monitor` shows `openai-rt` logs; a 401 means a bad
  key). The client is shared with OpenAI, so its log tag is `openai-rt`.
- `server error: …` lines are non-fatal xAI `error` events (e.g. an invalid
  voice name — the previous session config stays in effect). Use one of the
  listed voices.
- Do not use the device-side "update from release" OTA or the upstream Web
  Flasher on this build: they install upstream firmware, which has no Grok
  provider. To update, flash a newer `.bin` from this repo's releases.
- Robot replies before you finish talking / does not reply → tune the room
  noise; VAD threshold, prefix padding and silence duration are shared with the
  OpenAI path (`vad_*` in `ConversationConfig`).
- Grok vs OpenAI protocol differences handled in code: `voice` and
  `turn_detection` go at the session top level, no `OpenAI-Beta` header or
  `session.type` / `tool_choice`, input transcription uses `grok-transcribe`,
  the session is considered ready only on `session.updated`, the cumulative
  `conversation.item.input_audio_transcription.updated` event is ignored, and
  after (possibly parallel) tool calls a single `response.create` is sent only
  once every tool output is submitted and the tool-call response is done.

## Boot sequence (CoreS3 default path)

1. M5 / Avatar init, startup arpeggio (C5–E5–G5, can be disabled).
2. Load settings from NVS, start the BLE settings service (always advertising).
3. If an SSID is stored, start the Wi-Fi STA connection (non-blocking).
   On STA up → start mDNS + HTTP config server + SNTP.
4. Mic loopback test (record 2 s, play it back) as a sanity check.
5. Servo power on → ping (Yaw / Pitch) → 1.5 s settle.
6. Start the render task (30 fps face, core 1) and servo task (20 ms tick,
   core 0). If conversation is enabled, the conversation task waits for
   Wi-Fi and starts the AI dialogue.
7. demo_loop begins — idle: random babble, random head pose, nadenade
   reactions; during a conversation the AI task drives avatar + audio.
8. Tap top-right for the on-device UI; tap the screen during a reply to
   barge in; the Control tab's "Wi-Fi setup (hotspot)" row enters SoftAP provisioning (started automatically on first boot when no Wi-Fi is stored).

## Repository layout

```
.
├── components/
│   ├── avatar/              face rendering + animators (breath / saccade / blink, 6 expressions)
│   ├── avatar_vm/           Avatar DSL bytecode VM + NVS storage
│   ├── board/               CoreS3 / AtomS3R / StopWatch HW bring-up (board auto-detect)
│   ├── scs_servo/           SCS0009 driver + PathGenerator (trapezoidal velocity)
│   ├── jtts/                Japanese katakoto TTS (babble / speak_katakoto / LT calls)
│   ├── conversation/        AI voice-conversation clients (OpenAI / Gemini / XiaoZhi)
│   ├── config_service/      BLE GATT settings service + NVS + OTA + X25519/AES-GCM
│   ├── wifi_config_service/ Wi-Fi HTTP config + built-in web page + release OTA
│   ├── telegram/            Telegram Bot API (TLS) notification client (oss)
│   ├── M5GFX/               submodule (upstream)
│   ├── M5Unified/           submodule (upstream + 1 patch)
│   └── tl_expected/         tl::expected backport (submodule)
├── main/                    app_main, render/servo task, demo_loop, ap_screen,
│                            captive_portal, device_ui, atom_status, wifi_sta
├── patches/                 upstream-targeted patches
├── tools/                   apply-m5-patches.sh, monitor_log.py, settings.html,
│                            avatar_dsl/ (compiler + WASM glue)
├── assets/                  .avdsl sources (default_face, omega_mouth, aokko_face)
├── partitions.csv           OTA layout (ota_0 / ota_1 / nvs / storage)
├── sdkconfig.defaults*      shared + per-board (.cores3 / .atoms3r / .atoms3 / .stopwatch)
└── Makefile                 thin wrapper around idf.py (BOARD= switch)
```

## License

First-party sources (`components/board`, `components/scs_servo`,
`components/avatar`, `components/avatar_vm`, `components/jtts`,
`components/conversation`, `components/config_service`,
`components/wifi_config_service`, `components/telegram`, `main`, `tools`)
are released under the **Boost Software License 1.0** ([LICENSE](LICENSE)).

The submodules (`components/M5GFX`, `components/M5Unified`,
`components/tl_expected/expected`) and managed_components
(`espressif/esp_audio_codec`, `espressif/esp_websocket_client`,
`espressif/mdns`, `espressif/esp_jpeg`, `espressif/esp32-camera`, etc.)
keep their respective upstream licenses.
