# ProjectX-Submission
Line Maze Solver

I will be honest, clearly i havent written the code in task 9(vibe coded) due to time restrictions, but I can assure you that I know and can explain each and every line in the code
# LMS / Meshmerize: Fresher Selection Tasks

Submission for all nine tasks. The target platform is the ESP32-WROOM-32 (Arduino framework, C++) with the hardware in Appendix A of the task sheet.

## Repository layout

| Task | Topic | Where to find it |
|---|---|---|
| 1 | Basic coding | `src/task1.cpp`, `docs/task1_notes.md` |
| 2 | Datasheet literacy | `docs/Datasheets.jpeg` |
| 3 | PID control | `docs/task3_written.jpeg`, `docs/task3_pseudocode.md` |
| 4 | Bill of materials | `BOM/BOM.md`, and the power block diagram in `BOM/` |
| 5 | Power budget and battery | `BOM/PowerBudget.jpeg` |
| 6 | Electronics and hardware | the wiring diagram in `BOM/`, `docs/troubleshooting.md` |
| 7 | Wireless sensor diagnostics | `docs/protocol_1.jpeg`, `docs/protocol_2.jpeg` |
| 8 | Maze-solving algorithms | `docs/task8_maze.md`, `docs/Annotated_maze.png` |
| 9 | Line-following implementation | `src/task9_line_follower.ino`, `docs/task9_notes.md` |

## How to run

### Task 1: line position and motor limiting
```
cd src
g++ -std=c++11 task1.cpp -o task1
./task1
```
Prints example inputs and outputs for both functions.

### Task 9: ESP32 line follower
1. Install the ESP32 board package (version 3.x, because the code uses `analogWrite`) in Arduino IDE and select an ESP32 board.
2. Open `src/task9_line_follower.ino`. The IDE will offer to move the sketch into a folder with the same name (`task9_line_follower`). Accept.
3. **Sample inputs (default):** `SIM_MODE` is `1` at the top of the sketch. It runs on a hard-coded table of sensor readings and never drives the motors. Upload it and open the serial monitor at 115200 baud.
4. **Real robot:** set `SIM_MODE` to `0`. At startup, sweep the sensor array over the line and the background for 3 seconds (calibration), then place the robot on the line.

### Task 9: sample output
The serial output of the sketch on the sample inputs (selected lines):
```
SIM_MODE: hard-coded samples, motors not driven
ms,r1,r2,r3,r4,r5,error,left,right,mode
20,0,0,1000,0,0,0.000,150,150,FOLLOW
60,0,0,600,600,0,0.500,204,96,FOLLOW
70,0,0,0,1000,0,1.000,255,38,FOLLOW
90,0,0,0,0,1000,2.000,255,-63,FOLLOW
100,0,0,1000,0,0,0.000,56,244,FOLLOW
130,0,600,600,0,0,-0.500,50,250,FOLLOW
150,0,0,0,0,0,-0.500,75,75,HOLD
170,0,0,0,0,0,-0.500,-120,120,RECOVER
250,0,0,300,700,0,0.700,182,118,FOLLOW
290,0,0,0,0,0,0.700,75,75,HOLD
300,0,0,0,0,0,0.700,120,-120,RECOVER
800,0,0,0,0,0,0.700,120,-120,RECOVER
810,0,0,0,0,0,0.700,0,0,STOPPED
SIM complete
```
Columns: time in ms, five calibrated sensor readings (0-1000), error, left and right motor commands, mode. The output shows following the line (the side with the line gets less speed on the opposite wheel), motor clamping at 255, the lost-line sequence (HOLD, then RECOVER, then back to FOLLOW), and the final timeout (STOPPED). This output was produced by running the sketch logic on a PC with a stand-in for the Arduino library.

## Assumptions
- Hardware is as given in Appendix A: five analog IR sensors that read higher over the line, an L298N driver, and a 3S LiPo supply. If the real sensors read lower over a white line, set `INVERT_READINGS` to `true` in the Task 9 code.
- Sensor readings are calibrated to a 0-1000 scale with min-max calibration at startup.
- The control loop runs at a fixed 10 ms period.
- Task 8: the dry run uses DFS with encoder-based position tracking and stops at the first route found. The actual run replays that route as a list of turns.

## Limitations
- The Task 9 code was tested only on a PC with a stand-in for the Arduino library. It has not been compiled with the ESP32 core or run on a robot.
- The PID gains and thresholds are starting values and have not been tuned on hardware.
- The sample inputs change instantly, so the motor values in the sample output look more extreme than real behaviour would.
- The Task 9 code follows a line and handles a lost line. Junction detection and the maze logic are not in the code. The Task 8 maze solver is given as pseudocode only.
- Because the dry run stops at the first route found, the replayed route is valid but not guaranteed to be the shortest.
- The wireless diagnostics in Task 7 are a development tool. The competition rules require wireless devices to be switched off.

## Deviation from Appendix A
In the Task 9 code, IN3 and IN4 are on GPIO 16 and GPIO 17 instead of GPIO 12 and GPIO 11. On the ESP32-WROOM-32, GPIO 6-11 are connected to the onboard flash, and GPIO 12 is a strapping pin that selects the flash voltage at power-up. Using them as driver outputs can crash the chip or stop it booting. To use Appendix A's pins exactly, change the two constants at the top of the sketch.

## References
- Task sheet and Appendix A (mock hardware specification), LMS / Meshmerize
- Techfest 2026-27 Meshmerize rules and arena specification
- L298 datasheet (STMicroelectronics) and the Robu product pages linked in `BOM/BOM.md`
- ESP32-WROOM-32 datasheet and ESP32 technical reference manual (Espressif)

