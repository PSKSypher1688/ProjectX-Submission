# Bill of Materials: LMS / Meshmerize robot (budget: Rs 10,000)

All parts are bought from Robu (robu.in). Cells marked FILL IN are still to be completed from the product page. Quantities above what the robot uses are spares.

| # | Component | Exact model | Qty | Unit price (Rs) | Line total (Rs) | Link | Key specs and rationale |
|---|---|---|---|---|---|---|---|
| 1 | Controller | ESP32-WROOM-32 dev board | 2 | 720 | 1,440 | https://robu.in | Platform fixed by Appendix A. Dual core, 12-bit ADC for the sensors, built-in Bluetooth for the diagnostics in Task 7. One in use, one spare. |
| 2 | IR sensor array | Robu 5-channel TCRT5000L tracking module | 2 | 209 | 418 | https://robu.in | Five sensors 15 mm apart (60 mm span), so a 30 mm line covers about two sensors and gives a smooth centroid. Powered from 3.3 V so outputs stay within ESP32 limits. One in use, one spare. |
| 3 | Motor driver | L298N dual H-bridge module | 3 | 130 | 390 | https://robu.in | 5-35 V motor supply covers the 3S pack (12.6 V full). 2 A per bridge. Onboard 5 V regulator feeds the ESP32. One in use, two spares. |
| 4 | Motors | N20 12 V 300 RPM micro metal gear motor with encoder | 4 | 619 | 2,476 | https://robu.in | 12 V rating matches the pack. Built-in encoder (powered from 3.3 V) for the position tracking used in Task 8. Stall current: FILL IN from the page. Two in use, two spares. |
| 5 | Battery | 3S LiPo, 11.1 V, 1000 mAh, 30C | 2 | 1,049 | 2,098 | https://robu.in | Matches Appendix A. See the power budget for the runtime calculation. One in use while the other is charged. |
| 6 | Wheels | 34 mm Mini Car N20 motor rubber wheel | 4 | 59 | 236 | https://robu.in | Made for N20 motor shafts. Rubber tyres for grip on the vinyl arena. Two in use, two spares. |
| 7 | Caster | Mini 3PI Car N20 caster robot ball wheel | 2 | 39 | 78 | https://robu.in | Third support point so the robot sits level on two drive wheels. One in use, one spare. |
| 8 | Motor brackets | Mounting bracket for N20 micro gear motors (2 pcs) | 1 pair | 19 | 19 | https://robu.in | Fixes the two N20 motors to the chassis. |
| 9 | Chassis | Custom 3D-printed PLA frame (Robu 3D printing service) | 1 | 1,000 | 1,000 | https://robu.in | Rigid, light, designed to fit the 220 x 220 x 220 mm limit with mounting points for motors, sensor array and boards. Price is approximate. |
| 10 | Wiring | Male-to-male jumper wires, 40 pcs | 1 set | 49 | 49 | https://robu.in | Signal and rail connections. |
| 11 | Wiring | Female-to-female jumper wires, 40 pcs | 1 set | 49 | 49 | https://robu.in | Connections to the sensor array, encoders and module pins. |
| | **Subtotal** | | | | **8,253** | | |
| | Shipping | | | | 0 | | |
| | Contingency | | | | 1,747 | | Budget minus subtotal |
| | **Total** | | | | **10,000** | | At most Rs 10,000 |

## Motor choice: N20 12 V 300 RPM with encoder
- **Alternative considered:** N20 12 V 600 RPM (same family).
- **Why the 300 RPM:** more torque and finer control at the 90-degree turns, and lower stall current, which keeps the L298N and the battery well inside their limits. The encoder gives the position tracking that DFS needs.
- **What the alternative would give:** a higher top speed, which would improve the time score slightly, but with a higher stall current (state both stall currents from the two product pages: FILL IN).

## Power-block diagram
See `PowerBlock+Wiring_Diagram.jpeg` in this folder.
