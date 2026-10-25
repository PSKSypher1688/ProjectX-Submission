# Task 9 notes: PID tuning choices and deviations

## Controller
PD, not full PID (reasoning in the Task 3 notes). The error is the weighted-centroid position, from -2 (line far left) to +2 (line far right). With error > 0 the left wheel speeds up and the right slows down, so the robot turns toward the line.

## Tuning choices
All values are starting points. They were checked only on hard-coded sample inputs, not on a robot.

- **Base speed 150 (about 60% of 255).** This leaves headroom for steering. The largest error is 2, so the largest P correction is about 90, and 150 + 90 = 240 stays under the 255 limit.
- **Kp = 45.** The error spans only +/-2 but the output is in PWM units, so Kp has to be in the tens. I sized it so the maximum error gives a strong but not saturating correction.
- **Kd = 3, with a low-pass filter on the derivative (0.7).** Kd damps the overshoot that P alone would cause. The loop runs at a fixed 10 ms, so the time step is constant, and the filter stops sensor noise from being amplified into twitching motors.
- **First loop and after recovery:** the previous error is reset so the derivative term does not produce a kick.
- **Procedure on the real robot:** start slow, set Kd = 0, raise Kp until the robot follows the line with a slight wobble, then add Kd until it settles. Raise the speed only after that and retune.
- **Oscillation:** slow wide swings mean lower Kp or raise Kd; fast twitching means lower Kd or filter more; wobble only at high speed means lower the base speed.

## Lost-line handling
- **Detection:** the sum of the calibrated readings (0-1000 scale) is at or below 200, for 3 loops in a row (30 ms). The debounce ignores a single bad reading from a bump.
- **Response:** hold a slow straight speed during the debounce, then spin in place at 120 PWM toward the side where the line was last seen.
- **Timeout:** if the line is not found within 500 ms, stop. The stop is latched so the robot does not restart by itself.

## Deviation from Appendix A
IN3 and IN4 are on GPIO 16 and GPIO 17 instead of GPIO 12 and GPIO 11. On the ESP32-WROOM-32, GPIO 6-11 connect to the onboard flash, so using GPIO 11 as an output can crash the chip. GPIO 12 is a strapping pin that selects the flash voltage at power-up, so a driver input holding it high can stop the chip booting. To use Appendix A's pins exactly, change the two constants in the code back to 12 and 11.

## Assumptions and limitations
- `SIM_MODE` is 1 by default: the code runs on the sample table and never drives the motors. Set it to 0 for the robot.
- Sensors are assumed to read higher over the line, as in Appendix A. Set `INVERT_READINGS` to `true` if the hardware reads lower.
- Calibration needs the array swept over the line and the background during the first 3 seconds.
- Needs the arduino-esp32 core 3.x (`analogWrite`). Not compiled with the ESP32 core or run on hardware.
- The sample inputs change instantly, so the motor values in the sample output look more extreme than real behaviour would.
- The code follows a line and handles a lost line. Junction detection and the maze logic (Task 8) are not included.
