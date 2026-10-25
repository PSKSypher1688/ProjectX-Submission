Task 1:
Problem 1: line position (weighted centroid)

The position is the sum of each sensor’s weight times its reading, divided by the sum of the readings:

position = (-2·r1 − r2 + 0·r3 + r4 + 2·r5) / (r1 + r2 + r3 + r4 + r5)

Dividing by the total reading means the result depends on where the line is, not on how strong the readings are. A reading of 10 or 1000 on the rightmost sensor alone gives +2.
Symmetric readings give 0, which is correct: the line is centred.
The denominator is zero only when every reading is zero, because the readings are non-negative and can’t cancel. That is the “no line” case. The function returns false for it, and a true with the position otherwise. A bool is used because 0.0 is a valid position, so it can’t also mean “no line”.
Floats are used so the division isn’t truncated. An optional minSignal threshold lets a caller treat tiny noisy sums as “no line” as well.

Problem 2: motor-command limiting

Each command is clamped to [-max, +max]: above +max becomes +max, below -max becomes -max, and anything in between is unchanged. Checking both ends keeps the sign, so -300 becomes -200 and not +200. A negative max is converted to its absolute value so a bad argument can’t invert the range. The two motors are clamped independently, as the task asks. The trade-off is that if one motor saturates, the turn ratio between the two changes slightly.
