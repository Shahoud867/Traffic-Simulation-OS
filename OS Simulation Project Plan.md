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
    *   `include/common.hpp`
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
*   **Deliverables**: `src/main.cpp`, `src/controller.cpp`, `src/vehicle.cpp`, `src/utils.cpp` (logging), `include/common.hpp`, `Makefile`.

---

### **Phase 2: Safe Intersection Crossing (Synchronization)**
**Goal**: Implement the "Safe crossing constraint" using synchronization primitives. Vehicles must not collide.

*   **Objectives**:
    *   Divide the intersection into critical sections (Quadrants: NW, NE, SW, SE) or use a central Intersection Mutex (simpler, but less concurrent). **Recommendation**: Use 4 Mutexes (one per quadrant) to allow non-conflicting traffic (e.g., N->S and S->N can pass simultaneously if they don't share quadrants).
    *   Vehicles must acquire locks for the quadrants they need before "crossing".

*   **Conceptual Explanation**:
    
    **Quadrant-Based Intersection Model**:
    The intersection is divided into four quadrants (NW, NE, SW, SE) representing physical sections of the road. Each quadrant is protected by its own mutex. This design allows multiple vehicles to cross simultaneously if their paths don't overlap, maximizing concurrency while ensuring safety.

    **Path Analysis**:
    - **Straight paths** require 2 quadrants (e.g., North→South uses NW and SW quadrants)
    - **Right turns** require only 1 quadrant (e.g., North→East uses only NE quadrant)
    - **Left turns** require 3 quadrants (e.g., North→West uses NW, SW, and SE quadrants)

    **Deadlock Prevention Strategy**:
    All vehicles acquire quadrants in sorted numerical order (0→1→2→3). This consistent ordering prevents circular wait conditions that could cause deadlocks. Even if two vehicles need overlapping quadrants, the sorted acquisition ensures one waits for the other rather than creating a deadlock.

    **Vehicle Type-Based Crossing Times**:
    Different vehicle types have different crossing speeds:
    - Emergency vehicles (Ambulance, Firetruck): Fastest crossing times
    - Regular vehicles (Car, Bike): Medium crossing times  
    - Heavy vehicles (Bus, Tractor): Slower crossing times
    
    This realistic simulation affects intersection throughput and waiting times.

*   **Implementation Requirements**:
    1.  **Intersection State Structure**: Create a shared structure containing 4 mutex locks (one per quadrant), emergency mode flag, and vehicle counters.
    2.  **Quadrant Mapping Function**: Implement logic to determine which quadrants a vehicle needs based on entry point and destination direction.
    3.  **Lock Acquisition Logic**: Before entering intersection, vehicle threads must acquire all required quadrant locks in sorted order.
    4.  **Crossing Simulation**: Hold the locks while simulating crossing time (sleep), then release all locks.
    5.  **Logging**: Record when vehicles acquire/release quadrants to verify correct synchronization.

*   **Tasks**:
    1.  **`include/common.hpp`**: Add `Quadrant` enum (NW, NE, SW, SE), `IntersectionState` struct with 4 mutexes.
    2.  **`controller.cpp`**: 
        *   Initialize 4 mutexes in shared memory or global structure.
        *   Pass mutex array to vehicle threads via shared state.
    3.  **`vehicle.cpp`**:
        *   Implement quadrant determination function based on entry/destination.
        *   Modify vehicle routine to acquire quadrants before crossing.
        *   Simulate crossing time based on vehicle type.
        *   Release quadrants after crossing.
    4.  **`utils.cpp`**: Add timing logs to show concurrent access patterns.

*   **Acceptance Criteria**:
    *   Vehicles from conflicting paths (e.g., N->S and E->W) do NOT cross at the same time (verified by logs timestamps).
    *   Vehicles from non-conflicting paths (e.g., N->S and S->N) CAN cross simultaneously.
    *   No deadlocks occur even under heavy load (test with 50+ vehicles).
    *   Logs show quadrant acquisition/release in correct order.

*   **Testing Approach**:
    - Spawn vehicles with specific paths and verify mutual exclusion
    - Check log timestamps to confirm concurrent non-conflicting crossings
    - Stress test with many simultaneous vehicles to detect deadlocks

*   **Deliverables**: Updated `controller.cpp`, `vehicle.cpp`, `common.hpp`.

---

### **Phase 3: Dual Intersection & IPC**
**Goal**: Add the second intersection (F11) and enable communication.

*   **Objectives**:
    *   `main` forks two controllers: `F10` and `F11`.
    *   Create a pipe (or two for bidirectional) between F10 and F11.
    *   Vehicles leaving F10 Eastbound enter F11 Westbound.

*   **Conceptual Explanation**:

    **Inter-Process Communication (IPC) via Pipes**:
    Two separate controller processes (F10 and F11) run independently but communicate through Unix pipes. This demonstrates process-to-process communication, a core OS concept. Bidirectional communication requires two pipes: one for F10→F11 messages and another for F11→F10 messages.

    **Message Protocol Design**:
    A structured message format carries information between intersections:
    - **Message Type**: TRANSIT (vehicle moving between intersections), EMERGENCY_CLEAR (emergency vehicle approaching), ACK (acknowledgment), SHUTDOWN (graceful termination)
    - **Vehicle Data**: ID, type, entry point at new intersection, destination
    - **Timestamp**: For logging and debugging

    **Vehicle Transit Flow**:
    1. Vehicle completes crossing at F10 heading East
    2. F10 controller writes TRANSIT message to pipe
    3. F11 controller reads message from pipe (via IPC listener thread)
    4. F11 spawns new vehicle thread with same ID but new entry point (West)
    5. Vehicle continues journey through F11 intersection
    
    This creates seamless vehicle movement between intersections while maintaining process independence.

    **Process Architecture**:
    - **Parent Process**: Creates pipes, forks both controllers, manages cleanup
    - **F10 Controller Process**: Runs independently, reads from one pipe, writes to another
    - **F11 Controller Process**: Runs independently, reads from other pipe, writes to first pipe
    - Each controller has its own vehicle generator thread and IPC listener thread

    **Pipe Management**:
    Proper file descriptor handling is critical:
    - Parent creates pipes before forking
    - Each child closes unused pipe ends (read end of write pipe, write end of read pipe)
    - Parent closes all pipe ends after forking (children have their own copies)
    - This prevents pipe blocking and ensures clean shutdown

    **Graceful Shutdown**:
    SIGINT handler sends SHUTDOWN messages through both pipes, allowing controllers to:
    - Stop generating new vehicles
    - Wait for active vehicles to complete
    - Clean up resources (mutexes, threads)
    - Terminate cleanly without zombie processes

*   **Implementation Requirements**:
    1. **Message Structure**: Define IPCMessage with all necessary fields for vehicle transit
    2. **Bidirectional Pipes**: Create two pipes before forking for full duplex communication
    3. **IPC Listener Thread**: Each controller runs a dedicated thread reading from its input pipe
    4. **Vehicle Handoff**: When vehicle exits toward other intersection, write transit message
    5. **Vehicle Recreation**: On receiving transit message, spawn new thread with preserved vehicle ID
    6. **File Descriptor Management**: Properly close unused pipe ends in each process
    7. **Signal Handling**: Coordinate shutdown across both processes via pipes

*   **Tasks**:
    1.  **`include/common.hpp`**: Add `IPCMessage` struct, `MessageType` enum, `ControllerContext` struct.
    2.  **`main.cpp`**: 
        *   Create two pipes for bidirectional communication.
        *   Fork F10 and F11 controllers with appropriate pipe FDs.
        *   Setup signal handlers for graceful shutdown of both processes.
    3.  **`controller.cpp`**:
        *   Accept pipe FDs as parameters.
        *   Create IPC listener thread.
        *   Handle incoming TRANSIT messages by spawning vehicle threads.
    4.  **`vehicle.cpp`**:
        *   Check destination after crossing.
        *   If heading to other intersection, write TRANSIT message to pipe.
        *   Otherwise, exit normally.
    5.  **`utils.cpp`**: Add IPC-related logging functions.

*   **Acceptance Criteria**:
    *   A car spawning at F10 West -> going East -> "disappears" from F10 and "appears" at F11 West -> exits F11 East.
    *   Logs show the complete journey with timestamps.
    *   Vehicles maintain their ID when transitioning between intersections.
    *   Both intersections operate concurrently without blocking each other.
    *   No pipe deadlocks or orphaned processes.

*   **Testing Scenario**:
    *   Spawn 10 vehicles at F10, 5 heading East (to F11).
    *   Verify F11 receives exactly 5 vehicles.
    *   Verify vehicle IDs match in both logs.

*   **Deliverables**: `src/ipc.cpp` (helper functions), updated `main.cpp`, `controller.cpp`, `vehicle.cpp`, `common.hpp`.

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