// ============================================================================
// LMS / Meshmerize - Task 9: PD line follower (ESP32-WROOM-32, Arduino C++)
//
// Pipeline each loop:
//   5 analog IR readings -> min-max calibration -> weighted centroid (position)
//   -> error -> PD correction -> left/right speeds -> clamp -> L298N
// Lost line -> brief hold, then spin toward the last known side, then stop.
//
// SIM_MODE 1 : runs on hard-coded sample readings; motors are NEVER driven.
// SIM_MODE 0 : real robot (calibrates at startup, then follows the line).
// Requires the arduino-esp32 core 3.x (analogWrite).
// ============================================================================
#include <Arduino.h>

#define SIM_MODE 1

// ---------------------------------------------------------------------------
// Pins (Appendix A, except IN3 and IN4).
//
// DEVIATION FROM APPENDIX A: Appendix A assigns IN3 = GPIO 12 and
// IN4 = GPIO 11. On the ESP32-WROOM-32, GPIO 6-11 are connected to the
// onboard SPI flash, so using GPIO 11 as an output can crash or reboot-loop
// the chip. GPIO 12 is a strapping pin: its level at power-up selects the
// flash voltage, so a driver input holding it high can stop the chip booting.
// IN3 and IN4 are therefore moved to GPIO 16 and GPIO 17, which are free on
// the WROOM-32 and not used elsewhere. To follow Appendix A exactly, change
// the two constants below back to 12 and 11.
// ---------------------------------------------------------------------------
const uint8_t SENSOR_PINS[5] = {34, 35, 32, 33, 25};   // leftmost -> rightmost
const float   WEIGHTS[5]     = {-2.0f, -1.0f, 0.0f, 1.0f, 2.0f};

const uint8_t ENA = 14, IN1 = 27, IN2 = 26;            // left motor
const uint8_t ENB = 13, IN3 = 16, IN4 = 17;            // right motor (moved)

// ---------------------------------------------------------------------------
// Tunables (starting values, to be refined on the robot)
// ---------------------------------------------------------------------------
const uint32_t LOOP_MS = 10;                  // fixed control period
const float    DT_SEC  = LOOP_MS / 1000.0f;

const float KP        = 45.0f;                // position units -> PWM units
const float KD        = 3.0f;                 // damping on d(error)/dt (per s)
const float D_FILTER  = 0.7f;                 // derivative low-pass (0..1)
const int   BASE_SPEED = 150;                 // PWM, about 60% of 255
const int   MAX_PWM    = 255;

const float CAL_SCALE   = 1000.0f;            // calibrated readings are 0..1000
const float NOISE_FLOOR = 50.0f;              // calibrated values below = 0
const float MIN_SIGNAL  = 200.0f;             // sum of readings below = no line
const bool  INVERT_READINGS = false;          // true if line reads LOW on hardware

const int      LOST_DEBOUNCE        = 3;      // bad loops before "line lost"
const int      RECOVERY_SPEED       = 120;    // PWM while spinning to find line
const uint32_t RECOVERY_TIMEOUT_MS  = 500;    // then stop for good

const uint32_t CAL_MS        = 3000;          // startup calibration sweep time
const int      MIN_CAL_RANGE = 300;           // raw counts; smaller = bad cal

// ---------------------------------------------------------------------------
// Calibration data and helpers
// ---------------------------------------------------------------------------
int  calMin[5], calMax[5];
bool calOK = false;

// Line position as a weighted centroid, in [-2, +2].
// Returns false (position untouched) when the total signal is too weak.
bool estimateLinePosition(const float r[5], float &pos) {
  float weighted = 0, total = 0;
  for (int i = 0; i < 5; i++) {
    weighted += WEIGHTS[i] * r[i];
    total    += r[i];
  }
  if (total <= MIN_SIGNAL) return false;      // all-zero / noise-only case
  pos = weighted / total;
  return true;
}

// Clamp a signed command to [-maxMag, +maxMag], keeping its sign.
int clampSigned(int v, int maxMag) {
  int lim = abs(maxMag);
  if (v >  lim) return  lim;
  if (v < -lim) return -lim;
  return v;
}

// ---------------------------------------------------------------------------
// Motor output (L298N): sign -> direction pins, magnitude -> PWM pin.
// If a wheel spins the wrong way, swap that motor's two wires.
// ---------------------------------------------------------------------------
void setMotor(uint8_t en, uint8_t inA, uint8_t inB, int cmd) {
#if !SIM_MODE
  if (cmd > 0)      { digitalWrite(inA, HIGH); digitalWrite(inB, LOW);  }
  else if (cmd < 0) { digitalWrite(inA, LOW);  digitalWrite(inB, HIGH); }
  else              { digitalWrite(inA, LOW);  digitalWrite(inB, LOW);  }
  analogWrite(en, abs(cmd));
#endif
}

void driveMotors(int left, int right) {
  setMotor(ENA, IN1, IN2, left);
  setMotor(ENB, IN3, IN4, right);
}

// ---------------------------------------------------------------------------
// PD controller with lost-line handling
// ---------------------------------------------------------------------------
enum Mode { FOLLOW, HOLD, RECOVER, STOPPED };

struct Controller {
  float    prevError = 0, lastError = 0, dFilt = 0;
  bool     hasPrev = false;          // false -> no derivative kick next loop
  int      lostCount = 0;
  Mode     mode = FOLLOW;
  uint32_t recoveryStartMs = 0;
};

Controller ctl;

const char* modeName(Mode m) {
  switch (m) {
    case FOLLOW:  return "FOLLOW";
    case HOLD:    return "HOLD";
    case RECOVER: return "RECOVER";
    default:      return "STOPPED";
  }
}

void controlStep(Controller &c, const float r[5], uint32_t nowMs,
                 int &left, int &right) {
  if (c.mode == STOPPED) { left = right = 0; return; }   // stop is latched

  float error;
  if (estimateLinePosition(r, error)) {
    // Line seen. After a real recovery, drop stale PID history.
    if (c.mode == RECOVER) { c.hasPrev = false; c.dFilt = 0; }
    c.mode = FOLLOW;
    c.lostCount = 0;

    // Filtered derivative (skipped on the first loop / after recovery).
    float d = 0;
    if (c.hasPrev) {
      float raw = (error - c.prevError) / DT_SEC;
      c.dFilt = D_FILTER * c.dFilt + (1.0f - D_FILTER) * raw;
      d = c.dFilt;
    } else {
      c.dFilt = 0;
    }

    float correction = KP * error + KD * d;
    c.prevError = error;
    c.lastError = error;
    c.hasPrev   = true;

    // error > 0: line on the right -> left wheel faster -> turn right.
    left  = clampSigned(BASE_SPEED + (int)lroundf(correction), MAX_PWM);
    right = clampSigned(BASE_SPEED - (int)lroundf(correction), MAX_PWM);
    return;
  }

  // ---- Line not seen ----
  if (c.mode != RECOVER) {
    if (++c.lostCount < LOST_DEBOUNCE) {      // ignore a one-off glitch
      c.mode = HOLD;
      left = right = BASE_SPEED / 2;
      return;
    }
    c.mode = RECOVER;
    c.recoveryStartMs = nowMs;
  }

  if (nowMs - c.recoveryStartMs > RECOVERY_TIMEOUT_MS) {
    c.mode = STOPPED;                         // safe stop
    left = right = 0;
    return;
  }

  // Spin toward the side where the line was last seen.
  if (c.lastError >= 0) { left =  RECOVERY_SPEED; right = -RECOVERY_SPEED; }
  else                  { left = -RECOVERY_SPEED; right =  RECOVERY_SPEED; }
}

// ---------------------------------------------------------------------------
// Sensor input: real hardware, or hard-coded samples in SIM_MODE
// ---------------------------------------------------------------------------
#if SIM_MODE

struct SimRow { float r[5]; uint16_t repeat; };

// Calibrated readings (0..1000). Each row is repeated `repeat` loops.
const SimRow SIM_TABLE[] = {
  {{0,   0, 1000,    0,    0},  3},   // centred                   (pos  0.0)
  {{0,   0,  600,  600,    0},  2},   // slightly right            (pos +0.5)
  {{0,   0,    0, 1000,    0},  2},   // under S4                  (pos +1.0)
  {{0,   0,    0,    0, 1000},  1},   // under S5                  (pos +2.0)
  {{0,   0, 1000,    0,    0},  3},   // back to centre
  {{0, 600,  600,    0,    0},  2},   // slightly left             (pos -0.5)
  {{0,   0,    0,    0,    0}, 10},   // line lost: hold, then spin
  {{0,   0,  300,  700,    0},  3},   // line found again on right (pos +0.7)
  {{0,   0,    0,    0,    0}, 60},   // lost for good: timeout -> stop
};
const size_t SIM_ROWS = sizeof(SIM_TABLE) / sizeof(SIM_TABLE[0]);
bool simDone = false;

bool nextSimSample(float out[5]) {
  static size_t row = 0;
  static uint16_t used = 0;
  if (row >= SIM_ROWS) return false;
  for (int i = 0; i < 5; i++) out[i] = SIM_TABLE[row].r[i];
  if (++used >= SIM_TABLE[row].repeat) { row++; used = 0; }
  return true;
}

#else

int rawRead(int i) {
  int raw = analogRead(SENSOR_PINS[i]);       // 12-bit: 0..4095
  return INVERT_READINGS ? 4095 - raw : raw;
}

// Min-max calibration: sweep the array over the line and the background.
void calibrate() {
  Serial.println("Calibrating: sweep sensors over line and background...");
  for (int i = 0; i < 5; i++) { calMin[i] = 4095; calMax[i] = 0; }
  uint32_t start = millis();
  while (millis() - start < CAL_MS) {
    for (int i = 0; i < 5; i++) {
      int raw = rawRead(i);
      if (raw < calMin[i]) calMin[i] = raw;
      if (raw > calMax[i]) calMax[i] = raw;
    }
    delay(5);
  }
  calOK = true;
  for (int i = 0; i < 5; i++)
    if (calMax[i] - calMin[i] < MIN_CAL_RANGE) calOK = false;
  Serial.println(calOK ? "Calibration OK" : "Calibration FAILED - motors disabled");
}

void readSensors(float out[5]) {
  for (int i = 0; i < 5; i++) {
    float span = (float)(calMax[i] - calMin[i]);
    float v = (rawRead(i) - calMin[i]) * CAL_SCALE / span;
    v = constrain(v, 0.0f, CAL_SCALE);
    out[i] = (v < NOISE_FLOOR) ? 0.0f : v;
  }
}

#endif

uint32_t nowMs() {
#if SIM_MODE
  static uint32_t t = 0;                      // simulated clock
  t += LOOP_MS;
  return t;
#else
  return millis();
#endif
}

// ---------------------------------------------------------------------------
// Arduino entry points
// ---------------------------------------------------------------------------
uint32_t nextTickMs = 0;

void setup() {
  Serial.begin(115200);
  pinMode(ENA, OUTPUT); pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  driveMotors(0, 0);

#if SIM_MODE
  calOK = true;
  Serial.println("SIM_MODE: hard-coded samples, motors not driven");
#else
  calibrate();
  delay(2000);                                // time to place the robot
#endif

  Serial.println("ms,r1,r2,r3,r4,r5,error,left,right,mode");
  nextTickMs = nowMs();
}

void loop() {
  if (!calOK) { driveMotors(0, 0); return; }
#if SIM_MODE
  if (simDone) return;
#endif

  uint32_t now = nowMs();
  if ((int32_t)(now - nextTickMs) < 0) return;   // fixed-period loop
  nextTickMs += LOOP_MS;

  float r[5];
#if SIM_MODE
  if (!nextSimSample(r)) {
    driveMotors(0, 0);
    simDone = true;
    Serial.println("SIM complete");
    return;
  }
#else
  readSensors(r);
#endif

  int left, right;
  controlStep(ctl, r, now, left, right);
  driveMotors(left, right);

  // Telemetry every loop: time, sensors, error, motor commands, state.
  Serial.printf("%lu,%.0f,%.0f,%.0f,%.0f,%.0f,%.3f,%d,%d,%s\n",
                (unsigned long)now, r[0], r[1], r[2], r[3], r[4],
                ctl.lastError, left, right, modeName(ctl.mode));
}
