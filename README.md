# GT3 Wing Controller

LED controller for a GT3-style rear wing on a BMW Z4.

- `ios/`: SwiftUI iPhone app that controls the wing LEDs over Bluetooth. It's built in the cloud by Codemagic and sent to TestFlight (see `ios/CODEMAGIC_SETUP.md`).
- `firmware/GT3_Wing_C6/`: Arduino sketch for the Seeed XIAO ESP32-C6 (pins: D0 left, D1 right, D9 reverse, D10 brake, D8 LED data). Not yet compiled or tested on hardware.

The original ESP32-C3 Super Mini sketch (`ESP32_BLE_v2.ino.ino`) is still in the git history if you ever need it.
