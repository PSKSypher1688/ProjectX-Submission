# Task 8: Maze-solving algorithms

## 1. DFS versus a wall-preference rule

**DFS (depth-first search).** The robot follows one path as far as it goes. At a dead end it goes back to the last junction that still has an unexplored branch and tries that one. It builds a map of the maze as it goes.
- It needs to remember the graph and to know where it is, so it can recognise a junction it has already visited.
- It reaches the end of any maze where the end can be reached, including mazes with loops.
- The cost is more code, more memory, and a way to measure distance (encoders).

**Wall-preference rule (left-hand or right-hand rule).** On a line maze this means: at every junction prefer left, then straight, then right, and turn back only at a dead end.
- It needs no map and no position, so it is very cheap to build.
- It follows one boundary of the maze, so it is only guaranteed to reach the end in a maze without loops, or when the end lies on the boundary being followed. In a loop with a branch inside it, the robot can go round and round and never take the branch.
- If the turns are logged and the dead ends are removed afterwards, the result is the shortest route only in a maze without loops.

**Choice for an unknown maze: DFS with encoder-based position tracking.** The task sheet says the real arena will have alterations to the sample path, so loops cannot be ruled out, and the wall-preference rule can fail on them. DFS also leaves a graph that the actual run can use to choose a route. The price is the two encoders (added to the BOM) and more code. Position is tracked by dead reckoning: the heading is one of N, E, S, W, updated at each 90-degree turn, and the distance is

distance = encoder ticks x (wheel circumference / ticks per wheel revolution)

## 2. Junctions and paths as nodes and edges

- **Nodes** are the places where the robot must decide or turn: the start, the end, every junction, every corner and every dead end. Each node stores its (x, y) position.
- **Edges** are the stretches of line between two nodes. Each edge stores its length, measured from encoder ticks.
- A loop in the maze is a cycle in the graph.
- If the robot arrives within a tolerance of a stored node, it treats it as that same node. This is how loops are recognised.

Example maze (lengths are in arbitrary units; the diagram is below):

| Edge | Length |
|---|---|
| S-A | 2 |
| A-B | 3 |
| A-D | 2 |
| D-C | 3 |
| C-B | 2 |
| D-X | 2 |
| C-G | 1 |
| B-E | 3 |

S is the start, E is the end box, X and G are dead ends, and A, B, C, D are T-junctions. The loop is A-B-C-D.

![Annotated example maze] - Refer the Annotated_Maze.png

## 3. Worked traversal

Rules: the robot starts at S heading north. At each node it tries unexplored branches in the order left, straight, right. At a dead end it turns around and backtracks to the last node that still has an unexplored branch. It stops when it reaches E.

| Step | Arrives at (heading) | Branches seen | Choice | Stored |
|---|---|---|---|---|
| 1 | S (north) | straight only | go to A | S at (0,0); edge S-A, length 2 |
| 2 | A (north) | straight (D), right (B) | straight to D | A at (0,2), right branch unexplored; edge A-D, length 2 |
| 3 | D (north) | left (X), right (C) | left to X | D at (0,4), right branch unexplored; edge D-X, length 2 |
| 4 | X (west) | none | dead end: U-turn, back to D | X marked dead end |
| 5 | D (east) | straight (C) | straight to C | edge D-C, length 3 |
| 6 | C (east) | left (G), right (B) | left to G | C at (3,4), right branch unexplored; edge C-G, length 1 |
| 7 | G (north) | none | dead end: U-turn, back to C | G marked dead end |
| 8 | C (south) | straight (B) | straight to B | edge C-B, length 2 |
| 9 | B (south) | left (E), right (A) | left to E | B at (3,2), right branch unexplored; edge B-E, length 3 |
| 10 | E (east) | end zone | stop, light the LED | dry run complete |

Result: the route found is S, A, D, C, B, E, with length 2 + 2 + 3 + 2 + 3 = 12. The detours into X and G are not part of the route. The robot drove 18 units in total.

## 4. Exploration versus replay

**Exploratory traversal (the dry run).** The maze is unknown. The robot decides at every junction, handles dead ends, tracks its position and builds the graph.

**Optimal-route replay (the actual run).** The route is already chosen, so there is no search. The robot only needs a list of turns and a pointer into it. At each junction it reads the next entry (L, S or R), turns, and moves on. It meets no dead ends and needs no coordinates, so encoder drift does not matter, because it only counts junctions. It can also run faster on straights, because the next segment is known.

Turn list for the route S, A, D, C, B, E:

| Node | Arrive heading | Leave heading | Turn |
|---|---|---|---|
| A | north | north | S |
| D | north | east | R |
| C | east | south | R |
| B | south | east | L |

The stored list is **S, R, R, L**.

## 5. Pseudocode

```
CONSTANTS
    ORDER = [LEFT, STRAIGHT, RIGHT]     // branch preference at every node
    TOL   = position tolerance for matching nodes (set from measured drift)

---------------- DRY RUN: DFS EXPLORATION ----------------
// Assumption: stops at the first route found; no further exploration.

heading ← NORTH ; pos ← (0, 0) ; ticks ← 0 ; mode ← FORWARD
S ← new_node(pos) ; path ← [S]          // path = nodes from S to current node

LOOP
    follow_line_with_PID()
    ticks ← ticks + encoder_ticks()
    event ← read_sensor_pattern()       // NONE, JUNCTION, CORNER, DEAD_END, END_BOX
    IF event == NONE: CONTINUE

    pos   ← pos + ticks_to_distance(ticks) × unit_vector(heading)
    ticks ← 0

    IF event == END_BOX:
        turn LED on ; stop motors
        route ← path + [E]              // path holds no dead-end detours
        BREAK

    // ---- arrive at a node ----
    IF mode == BACKTRACK:
        node ← path.top                 // returning to a known node
    ELSE:
        node ← stored node within TOL of pos
        IF node exists:                 // loop closed: node already visited
            record edge (path.top, node, length)
            U-turn ; mode ← BACKTRACK   // return along the same edge
            CONTINUE
        node ← new_node(pos)
        node.back_heading ← opposite(heading)
        node.unexplored   ← branches seen by sensors, minus the one we came from
        record edge (path.top, node, length) ; path.push(node)

    // ---- choose next move at the node ----
    IF node.unexplored is not empty:
        b ← first branch in ORDER that is in node.unexplored
        remove b from node.unexplored
        node.leave_heading ← heading after turning to b
        turn to b ; mode ← FORWARD
    ELSE IF node == S:
        stop ; no route found
    ELSE:                               // dead end, or node fully explored
        heading ← node.back_heading ; turn to face it
        path.pop() ; mode ← BACKTRACK
END LOOP

---------------- BUILD TURN LIST ----------------

turns ← []
FOR i = 1 TO length(route) − 2:         // every node between S and E
    IF route[i] is a junction (2 or more branches):
        arrive ← route[i−1].leave_heading
        leave  ← route[i].leave_heading
        turns.append( turn_between(arrive, leave) )     // L, S or R

---------------- ACTUAL RUN: REPLAY ----------------

i ← 0
LOOP
    follow_line_with_PID()
    event ← read_sensor_pattern()
    IF event == END_BOX:
        turn LED on ; stop motors ; BREAK
    IF event == JUNCTION:
        execute_turn(turns[i]) ; i ← i + 1
    IF event == CORNER:
        turn toward the only available branch           // no list entry
END LOOP
```

## 6. Limitation

The dry run stops at the first route found, so the replayed route is valid but not guaranteed to be the shortest. In the example, the shortcut A to B (length 3) was never driven. Using it would give the route S, A, B, E with length 8 and the turn list R, S, but the robot cannot confirm that the shortcut exists without exploring it.
