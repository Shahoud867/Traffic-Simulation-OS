# OS Traffic Simulation Project

A multi-process, multi-threaded simulation of traffic flow across two intersections (F10 and F11) with parking management and emergency vehicle preemption.

**Language**: C++ (C++17 standard)

## Current Status

**Phase 1 Complete**: Basic simulation skeleton with:
- Main process spawning a Controller process (F10).
- Controller spawning a Vehicle Generator thread.
- Vehicle threads simulating arrival and travel time.
- Logging system.

**Phase 2 Complete**: Safe intersection crossing with synchronization:
- 4-quadrant intersection model (NW, NE, SW, SE) with individual mutexes for each quadrant.
- Deadlock-free vehicle crossing using sorted lock acquisition on required quadrants.
- Concurrent access for non-conflicting paths (e.g., N→S and S→N simultaneously).
- Vehicle-type based crossing times (Ambulance: 1s, Car: 1.5s, Tractor: 3s).

**Phase 3 Complete**: Dual intersections with IPC:
- Two separate controller processes (F10 and F11) communicating via bidirectional pipes.
- Seamless vehicle transit between intersections preserving vehicle IDs and metadata.
- IPC message protocol supporting TRANSIT, EMERGENCY_CLEAR, SHUTDOWN, and ACK messages.
- Dedicated IPC listener threads in each controller for real-time message processing.

## Prerequisites

This project requires a POSIX-compliant environment (Linux).
If you are on **Windows**, you **MUST** use **WSL (Windows Subsystem for Linux)**.

### Required Tools
- `g++` (GNU C++ Compiler with C++17 support)
- `make` (Build automation)
- `libncurses-dev` (For future visualization)

### Installation (Ubuntu/Debian/WSL)
Run the following commands in your terminal:
```bash
sudo apt update
sudo apt install build-essential libncurses-dev
```

## Building the Project

1. Navigate to the project root directory.
2. Run `make`:
   ```bash
   make
   ```
   This will compile the source code and create the `bin/` directory and `logs/` directory.

## Running the Simulation

Execute the binary:
```bash
# Run Phase 2 (Single Intersection with Quadrants)
./bin/traffic_sim 2

# Run Phase 3 (Dual Intersections with IPC) - Default
./bin/traffic_sim 3
```

To stop the simulation, press `Ctrl+C`.

## Output

- **Console**: Displays real-time logs of vehicles arriving and exiting.
- **Log File**: Events are persisted to `logs/traffic_sim.log`.

## Project Structure

```text
.
├── Makefile                        # Build script
├── README.md                       # This file
├── OS Simulation Project Plan.md   # Detailed phase-wise implementation plan
├── bin/                            # Compiled binary
│   └── traffic_sim
├── include/                        # Header files
│   ├── common.hpp                  # Shared structs, enums, and constants
│   ├── controller.hpp              # Controller logic prototypes
│   ├── vehicle.hpp                 # Vehicle logic prototypes
│   └── utils.hpp                   # Logging and helper functions
├── src/                            # Source code
│   ├── main.cpp                    # Entry point, process forking, IPC setup
│   ├── controller.cpp              # Controller logic, IPC listener, intersection state
│   ├── vehicle.cpp                 # Vehicle threads, quadrant logic, transit handling
│   └── utils.cpp                   # Logging implementation
├── logs/                           # Generated log files
│   └── traffic_sim.log
└── obj/                            # Compiled object files
```
