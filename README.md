# ESP32 Web Bluetooth RC Car 🚗

An ESP32 RC car controlled from a browser via Web Bluetooth. Just open the HTML, connect via Bluetooth and drive.

## 🧰 Components
**Brains**
- ESP32 WEMOS LOLIN32 Lite (CH340C USB chip)
- TC1508A Dual H-Bridge Motor Driver Module (sold as "Mini L298N", but with the TC1508A chip, not an actual L298N)

**Muscle**
- 2x TT Gearbox DC Motors (3V–6V, 1:48 gear ratio)
- 2x 5mm Yellow LEDs
- 2x 220Ω Resistors (for the LEDs)
  
**Power/Charging**
- 3.7V 1S LiPo Battery (Original: 802060-1000mAh, died. Upgraded to: --)
- TP4056 Lithium Charger Module (5V 1A, with DW01 dual protection, Mini USB & Type-C)
- MT3608 DC-DC Boost Converter (Step-Up, 2A)
- LM2596 DC-DC Buck Converter (Step-Down)
- 2x 2-Pin Rocker Switches (SPST) (one for the charger circuit, one for the main car circuit)

---
**Motors, charger and rest of logic get separate power, but there's one shared ground.**

**Ground (GND):**
Every `-` wire in the car goes to one common junction.

**Pins used:**

| Function | GPIO |
|---|---|
| Motor A | 13, 26 |
| Motor B | 33, 4 |
| LEDs | 18, 23 |


## 💻 Setup

**ESP32 (Arduino IDE):**
1. Board: WEMOS LOLIN32 Lite w/ CH340C chip
2. **Upload speed: 115200** (921600 fails with a scary flash error)
3. Libraries: `BLEDevice`, `BLEServer`, `BLEUtils`, `BLE2902`
4. Upload the `.ino` to the board

**Controller:**
1. Open `controller.html` in **Chrome or Edge** (not Safari, not Firefox)
2. Click CONNECT TO CAR
3. Pick `ESP32_RC_Car` from the browser popup

## 🎮 Driving

1. Flip Switch 2 (car circuit) ON. Leave Switch 1 (charger) OFF.
2. Connect via the web page.
3. GAS / REVERSE / LEFT / RIGHT / STOP / LIGHTS

**To charge:**
1. Flip Switch 2 (car) OFF.
2. Flip Switch 1 (charger) ON.
3. Plug in USB to the TP4056.
4. When done, unplug USB, flip Switch 1 OFF.


## ⚠️ Lessons Learned the Hard Way

- **Don't connect USB and battery at the same time.** Pick one.
- **Set LM2596 to exactly 3.30V** before wiring it to the ESP32. Not 3.5V or anything "close enough."
- **Battery → MT3608 (7V) → LM2596 (3.3V)** because the buck converter can't lower a voltage less than 5V, so we need to first increase the 3.7V to 5V, then reduce it to 3.3V.
- **Don't charge while driving.** TP4056 has no load sharing.
- **Web Bluetooth ≠ OS Bluetooth pairing.** Do NOT pair through phone/laptop settings. Use the web page. If you already paired via OS, forget it first.
- **Motors without capacitors can work fine.** Add them only if Bluetooth drops out under load.


## 🛠️ Quick Troubleshooting

| Problem | Fix |
|---|---|
| "Flash chip connection" error | Upload speed → 115200 |
| Won't boot on battery, works on USB | Check common ground, verify 3.30V under load, press RST once and wait 5s |
| Motors twitch and everything dies | Battery sag or DW01 tripping |
| Bluetooth connects, motor silent | Missing common ground between ESP32 and driver |

## 🪦 Casualties

- **1x 1000mAh LiPo (802060)** — Seriously discharged during testing and never recovered. Would charge to 4.1V in less than 10 min and drop back to 3.1V the second you unplugged it.
- **1x TC1508A Motor Driver** — Died suddenly mid-project. Suddenly stopped responding on Motor A, onnly worked for Motor B. Suspect a voltage spike during the LiPo's final death throes. Replaced with an identical unit.
- **1x GPIO 27** — Killed during a soldering operation with a bad batch of solder. Moved Motor B over to GPIO 33 instead.

---
## 📜 Status

Working. Tight turning, stable Bluetooth, headlights toggle. 
Future upgrades: better design

*Built with more troubleshooting than planned.*
