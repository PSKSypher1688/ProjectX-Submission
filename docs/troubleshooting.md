Fault: ESP32 resets when the motors start

Symptom. The robot works with the motors off. When the motors start, accelerate or stall, the ESP32 reboots.

Probable root causes

Supply voltage sag from motor inrush. A brushed motor draws several times its running current at start-up and at stall. This pulls the battery voltage down through the internal resistance, wiring and connectors. The 5 V from the L298N’s onboard regulator then falls, the ESP32’s 3.3 V rail drops below its brownout threshold, and the chip resets.
Motor electrical noise. Brushed motors put voltage spikes and high-frequency noise onto the shared supply and ground, which can glitch the ESP32’s supply.
Overloaded onboard regulator. It is small and also powers the ESP32 and the L298N’s logic.
Poor wiring or layout. Thin wires, loose connectors, grounds joined far from the battery, or the ESP32 mounted right next to the driver and motor wires all increase the sag and the noise coupling.
Pin conflict. Appendix A’s GPIO 11 is wired to the onboard flash on the WROOM-32 and can crash the chip when used as an output. I moved IN3 and IN4 to GPIO 16 and 17 to avoid this.

First diagnostic check. Read the reset reason on the serial monitor at 115200 baud. A brownout reset is reported as such. Then run the same code with the motor wires disconnected. If it still resets, the cause is the code or the pins (cause 5). If it only resets with the motors connected, the cause is power or noise (causes 1-4). Then measure the voltage at the ESP32’s Vin pin and at the battery terminals while the motors start, to see where the dip occurs.

Fix

Add a bulk capacitor across the L298N motor supply and a smaller capacitor plus a 100 nF ceramic at the ESP32’s Vin.
Add a 100 nF ceramic capacitor across each motor’s terminals.
Power the ESP32 from a separate 5 V buck regulator taken directly from the battery.
Use thicker, shorter motor-supply wires, and join the grounds at one point near the battery.
Mount the ESP32 away from the L298N and the motor wires.
Ramp the PWM up gradually instead of jumping to full speed.

Confirming the fix. Repeat the failure conditions many times: full-speed starts from rest, reversals, and holding the wheels stalled for a few seconds. Check that there are no resets and no brownout messages. Re-measure the minimum voltage at Vin during these events, and repeat the test with a partly discharged battery.