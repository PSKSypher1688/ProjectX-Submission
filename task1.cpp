// Task 1 - Basic Coding
// Problem 1: line-position estimation (weighted centroid)
// Problem 2: motor-command limiting (clamping with sign preserved)
//
// Build & run:  g++ -std=c++11 task1.cpp -o task1 && ./task1
//
// Plain C++ (no Arduino calls) so it runs on a PC and ports straight to ESP32.
// All example sensor readings use one scale: calibrated 0-1000.

#include <cstdio>
#include <cstdlib>

constexpr int NUM_SENSORS = 5;
constexpr float WEIGHTS[NUM_SENSORS] = {-2.0f, -1.0f, 0.0f, 1.0f, 2.0f};

// ---------------------------------------------------------------------------
// Problem 1: weighted-centroid line position
//
//   position = sum(weight_i * reading_i) / sum(reading_i)
//
// Returns true  -> line seen, `position` holds the estimate in [-2, +2].
// Returns false -> total signal is not above `minSignal` (all-zero case):
//                  no line detected, `position` is left untouched.
//
// A bool is returned because 0.0 is a valid position ("line centred"),
// so it cannot also mean "no line". Floats avoid integer-division truncation.
// ---------------------------------------------------------------------------
bool estimateLinePosition(const float readings[NUM_SENSORS], float &position,
                          float minSignal = 0.0f) {
    float weightedSum = 0.0f;
    float total = 0.0f;

    for (int i = 0; i < NUM_SENSORS; i++) {
        weightedSum += WEIGHTS[i] * readings[i];
        total += readings[i];
    }

    // Readings are non-negative, so total == 0 only if every reading is 0.
    // A minSignal > 0 lets the caller treat tiny noisy sums as lost line too.
    if (total <= minSignal) {
        return false;
    }

    position = weightedSum / total;
    return true;
}

// ---------------------------------------------------------------------------
// Problem 2: motor-command limiting
//
// Clamps a signed command to [-maxMagnitude, +maxMagnitude]; sign is kept.
// Both ends of the range are checked, so -300 becomes -200 (not +200).
// A negative maxMagnitude is treated as its absolute value so a bad argument
// cannot invert the range.
// ---------------------------------------------------------------------------
int clampSigned(int value, int maxMagnitude) {
    int limit = std::abs(maxMagnitude);
    if (value > limit)  return limit;
    if (value < -limit) return -limit;
    return value;
}

// Each motor is clamped independently.
void limitMotorCommands(int &left, int &right, int maxMagnitude) {
    left  = clampSigned(left,  maxMagnitude);
    right = clampSigned(right, maxMagnitude);
}

// ---------------------------------------------------------------------------
// Examples
// ---------------------------------------------------------------------------
static void runPositionExample(const char *label, const float r[NUM_SENSORS]) {
    float pos = 0.0f;
    if (estimateLinePosition(r, pos)) {
        std::printf("%-34s -> position = %.3f\n", label, pos);
    } else {
        std::printf("%-34s -> NO LINE (all zero)\n", label);
    }
}

int main() {
    std::printf("--- Problem 1: line position (readings 0-1000) ---\n");
    const float centred[5]     = {0, 0, 1000, 0, 0};
    const float rightOnly[5]   = {0, 0, 0, 0, 1000};
    const float weakRight[5]   = {0, 0, 0, 0, 10};
    const float centreRight[5] = {0, 0, 1000, 1000, 0};
    const float symmetric[5]   = {500, 800, 1000, 800, 500};
    const float lost[5]        = {0, 0, 0, 0, 0};

    runPositionExample("only centre sensor", centred);                  // 0.000
    runPositionExample("only rightmost sensor", rightOnly);             // 2.000
    runPositionExample("rightmost, weak signal (10)", weakRight);       // 2.000
    runPositionExample("centre + right-of-centre equal", centreRight);  // 0.500
    runPositionExample("symmetric readings", symmetric);                // 0.000
    runPositionExample("all zero", lost);                               // NO LINE

    std::printf("\n--- Problem 2: motor limiting (max = 200) ---\n");
    int tests[][2] = {{150, -50}, {300, -300}, {-250, 200}, {0, 201}};
    for (auto &t : tests) {
        int l = t[0], r = t[1];
        limitMotorCommands(l, r, 200);
        std::printf("in (%4d, %4d) -> out (%4d, %4d)\n", t[0], t[1], l, r);
    }

    // A negative max is treated as its absolute value.
    int l = 300, r = -300;
    limitMotorCommands(l, r, -200);
    std::printf("in ( 300, -300), max = -200 -> out (%4d, %4d)\n", l, r);
    return 0;
}
