CONSTANTS: Kp, Kd, BASE_SPEED, MAX_PWM, MIN_SIGNAL, RECOVERY_SPEED, TIMEOUT

STATE: prevError, lastError, lostCount, stopped

LOOP every 10 ms: (i have set the dt to be constant)

    read the 5 sensors (calibrated, 0 to 1000)
    total       = sum of readings
    weightedSum = sum of (weight[i] * reading[i])    // weights: -2, -1, 0, 1, 2

    IF stopped:
        left = 0, right = 0

    ELSE IF total > MIN_SIGNAL:                      // line seen
        error      = weightedSum / total             // -2 (left) to +2 (right)
        derivative = (error - prevError) / dt
        correction = Kp * error + Kd * derivative

        left  = BASE_SPEED + correction
        right = BASE_SPEED - correction

        prevError = error
        lastError = error
        lostCount = 0

    ELSE:                                            // line lost
        lostCount = lostCount + 1
        IF lostCount is small:
            drive slowly straight                    // ignore a one-off glitch
        ELSE IF time since lost < TIMEOUT:
            spin toward the side of lastError        // look for the line
        ELSE:
            stopped = true
            left = 0, right = 0

    clamp left and right to [-MAX_PWM, MAX_PWM]
    send left and right to the motors
