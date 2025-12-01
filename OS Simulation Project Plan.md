### **Recommended Tech Stack**
*   **Language**: **C++ (C++17 standard)**. This project uses C++ while leveraging POSIX primitives (`fork`, `pipe`, `signal`, `pthread`) for systems programming tasks.
*   **Build System**: GNU Make.
*   **Libraries**:
    *   `pthread` (Threads)
    *   `rt` (Real-time extensions for shared memory/semaphores if needed, though usually standard in glibc now)
    *   `ncurses` (For text-based visualization)
*   **Environment**: Linux (WSL2 or Native).

---

### **Phase 0: Design & Environment Setup**
**Goal**: Establish the project structure, build system, and define core data structures.

*   **Tasks**:
    1.  Create directory structure: `src/`, `include/`, `logs/`, `bin/`.
    2.  Create a `Makefile` that compiles source files into a binary named `traffic_sim`.
    3.  Define `common.h` containing:
        *   `Vehicle` struct (id, type, entry_point, destination, state).
        *   `Constants` (Intersection IDs, Vehicle Types).
*   **Deliverables**:
    *   `Makefile`
    *   `include/common.h`
    *   `src/main.cpp` (Empty skeleton)

---

### **Phase 1: Basic Traffic Controller & Vehicle Spawning**
**Goal**: Implement the process/thread hierarchy for a single intersection (F10) and get vehicles flowing (without collision logic).

*   **Objectives**:
    *   `main` process forks one child process: `Controller_F10`.
    *   `Controller_F10` spawns a `VehicleGenerator` thread.
    *   `VehicleGenerator` spawns `Vehicle` threads at random intervals.
    *   Vehicles print their status to stdout and a log file, then exit.
*   **Data Structures**:
    ```c
    // include/common.h
    typedef enum { AMBULANCE, FIRETRUCK, BUS, CAR, BIKE, TRACTOR } VehicleType;
    typedef enum { NORTH, SOUTH, EAST, WEST } Direction;

    typedef struct {
        int id;
        VehicleType type;
        Direction entry;
        Direction dest;
        // Add timing/speed fields here
    } VehicleArgs;
    ```
*   **Tasks**:
    1.  **`main.cpp`**: Implement `fork()` to spawn `Controller_F10`. Wait for `SIGINT` to kill child.
    2.  **`controller.cpp`**: Implement the controller logic. For now, it just creates the generator thread and waits.
    3.  **`vehicle.cpp`**: Implement `void* vehicle_routine(void* arg)`.
        *   Behavior: Print "Vehicle [ID] [Type] Arrived at [Entry]" -> Sleep(simulated travel time) -> Print "Vehicle [ID] Exited".
    4.  **Logging**: Write a simple `log_event(const char* msg)` function that writes to `traffic_sim.log` with a timestamp.
*   **Acceptance Criteria**:
    *   Running `./traffic_sim` starts the simulation.
    *   Console shows a stream of vehicles appearing and exiting.
    *   `Ctrl+C` terminates the program cleanly (no zombie processes).
    *   Log file contains entries for vehicle creation and exit.
*   **Test Scenario**:
    *   Run for 10 seconds. Check log for ~10-20 vehicles (depending on spawn rate).
*   **Deliverables**: `src/main.cpp`, `src/controller.cpp`, `src/vehicle.cpp`, `src/utils.cpp` (logging), `include/common.h`, `Makefile`.

---

### **Phase 2: Safe Intersection Crossing (Synchronization)**
**Goal**: Implement the "Safe crossing constraint" using synchronization primitives. Vehicles must not collide.

*   **Objectives**:
    *   Divide the intersection into critical sections (Quadrants: NW, NE, SW, SE) or use a central Intersection Mutex (simpler, but less concurrent). **Recommendation**: Use 4 Mutexes (one per quadrant) to allow non-conflicting traffic (e.g., N->S and S->N can pass simultaneously if they don't share quadrants).
    *   Vehicles must acquire locks for the quadrants they need before "crossing".
*   **Tasks**:
    1.  **`controller.cpp`**: Initialize 4 mutexes/semaphores representing the intersection quadrants.
    2.  **`vehicle.cpp`**:
        *   Determine required quadrants based on Entry -> Destination.
        *   `pthread_mutex_lock(&quadrant_X)` -> `sleep(crossing_time)` -> `pthread_mutex_unlock(&quadrant_X)`.
        *   **Deadlock Prevention**: Always acquire locks in a fixed order (e.g., by Quadrant ID 0->1->2->3).
*   **Acceptance Criteria**:
    *   Vehicles from conflicting paths (e.g., N->S and E->W) do NOT cross at the same time (verified by logs timestamps).
    *   Vehicles from non-conflicting paths (e.g., N->S and S->N) CAN cross simultaneously.
*   **Deliverables**: Updated `controller.cpp`, `vehicle.cpp`.

---

### **Phase 3: Dual Intersection & IPC**
**Goal**: Add the second intersection (F11) and enable communication.

*   **Objectives**:
    *   `main` forks two controllers: `F10` and `F11`.
    *   Create a pipe (or two for bidirectional) between F10 and F11.
    *   Vehicles leaving F10 Eastbound enter F11 Westbound.
*   **IPC Protocol**:
    *   Define a message struct:
    ```c
    typedef struct {
        int type; // 0=Transit, 1=Emergency_Clear, 2=Ack
        int vehicle_id;
        VehicleType v_type;
    } IPC_Message;
    ```
*   **Tasks**:
    1.  **`main.cpp`**: Create `pipe(fd)`. Fork F10 and F11, passing pipe FDs.
    2.  **`vehicle.cpp`**:
        *   If a vehicle exits F10 towards F11, it doesn't just "exit". It writes a "Transit" message to the pipe.
    3.  **`controller.cpp` (F11)**:
        *   Listen on the pipe (reader thread or non-blocking read).
        *   When "Transit" message received -> Spawn a new Vehicle thread in F11 representing that car.
*   **Acceptance Criteria**:
    *   A car spawning at F10 West -> going East -> "disappears" from F10 and "appears" at F11 West -> exits F11 East.
*   **Deliverables**: `src/ipc.cpp` (helper functions), updated `main.cpp`, `controller.cpp`.

---

### **Phase 4: Parking Lot Management**
**Goal**: Implement the parking logic for F10.

*   **Objectives**:
    *   Implement Semaphores: `Sem_Spots` (Init=10), `Sem_Queue` (Init=N).
    *   Vehicles (Car/Bike/Tractor) attempting to park must wait for a spot.
*   **Tasks**:
    1.  **`parking.cpp`**: Initialize semaphores.
    2.  **`vehicle.cpp`**:
        *   If `dest == PARKING`:
            *   `sem_wait(&Sem_Queue)` (Enter waiting queue).
            *   `sem_wait(&Sem_Spots)` (Wait for actual spot).
            *   `sem_post(&Sem_Queue)` (Leave queue).
            *   Enter Intersection (Acquire Quadrants) -> Park -> Sleep(park_time) -> Leave.
            *   `sem_post(&Sem_Spots)` (Free spot).
*   **Safety**: Ensure vehicles waiting for parking do not hold intersection locks.
*   **Acceptance Criteria**:
    *   Only 10 cars park at once.
    *   Excess cars wait in the queue.
    *   If queue is full, cars might reroute or wait (depending on spec).
*   **Deliverables**: `src/parking.cpp`, updated `vehicle.cpp`.

---

### **Phase 5: Emergency Vehicle Preemption**
**Goal**: Handle Ambulance/Firetruck priority.

*   **Objectives**:
    *   Emergency vehicles bypass parking logic.
    *   They send a "Clear Path" signal to the *next* intersection if they are headed there.
*   **Tasks**:
    1.  **`vehicle.cpp`**:
        *   If `type == AMBULANCE`:
            *   Send `IPC_Message { type=Emergency_Clear }` to next controller.
    2.  **`controller.cpp`**:
        *   On receiving `Emergency_Clear`:
            *   Set global flag `EMERGENCY_MODE = 1`.
            *   Prevent new normal vehicles from entering intersection (block their mutex acquisition).
            *   Once Emergency vehicle passes, `EMERGENCY_MODE = 0`.
*   **Acceptance Criteria**:
    *   When Ambulance approaches F10->F11, F11 stops spawning/moving local traffic until Ambulance passes.
*   **Deliverables**: Updated `controller.cpp`, `vehicle.cpp`, `ipc.cpp`.

---

### **Phase 6: Visualization & Logging**
**Goal**: Make it look good.

*   **Objectives**:
    *   Use `ncurses` to draw a map of F10 and F11.
    *   Show vehicles as characters (A, F, B, C) moving across the grid.
    *   Show Semaphore states (Parking: [|||||.....] 5/10).
*   **Tasks**:
    1.  **`display.cpp`**:
        *   Setup ncurses window.
        *   Map vehicle coordinates to screen coordinates.
        *   Refresh loop (every 100ms).
*   **Acceptance Criteria**:
    *   Visual representation matches the log events.
*   **Deliverables**: `src/display.cpp`.

---

### **Phase 7: Final Polish & Configuration**
**Goal**: Cleanup, Config parsing, and Stress Test.

*   **Tasks**:
    1.  **`config.cpp`**: Parse `simulation.conf` (INI format) for spawn rates, parking capacity.
    2.  **Shutdown**: Ensure `SIGINT` frees all memory, destroys mutexes/semaphores, and kills processes.
    3.  **Documentation**: README with build/run instructions.
*   **Deliverables**: `simulation.conf`, `README.md`, Final Codebase.

---

### **Suggested File Structure**
```text
.
├── Makefile
├── README.md
├── simulation.conf
├── include
│   ├── common.h      // Structs, Enums, Constants
│   ├── ipc.h         // Pipe/Message definitions
│   └── display.h     // Visualization prototypes
├── src
│   ├── main.cpp        // Entry point, Process Forking
│   ├── controller.cpp  // Controller logic, Thread management
│   ├── vehicle.cpp     // Vehicle behavior, State machine
│   ├── parking.cpp     // Parking semaphore logic
│   ├── ipc.cpp         // Pipe handling
│   ├── utils.cpp       // Logging, Config parsing
│   └── display.cpp     // Ncurses visualization
└── logs
    └── traffic_sim.log
```

---

**Submit for Phase 1:** `src/main.cpp, src/controller.cpp, src/vehicle.cpp, src/utils.cpp, include/common.h, Makefile`