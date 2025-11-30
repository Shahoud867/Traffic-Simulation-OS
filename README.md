# OS Traffic Simulation Project

A multi-process, multi-threaded simulation of traffic flow across two intersections (F10 and F11) with parking management and emergency vehicle preemption.

**Language**: C++ (C++17 standard)

## Current Status
**Phase 1 Complete**: Basic simulation skeleton with:
- Main process spawning a Controller process (F10).
- Controller spawning a Vehicle Generator thread.
- Vehicle threads simulating arrival and travel time.
- Logging system.

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
./bin/traffic_sim
```

To stop the simulation, press `Ctrl+C`.

## Output

- **Console**: Displays real-time logs of vehicles arriving and exiting.
- **Log File**: Events are persisted to `logs/traffic_sim.log`.

## Project Structure

```text
.
├── Makefile            # Build script
├── README.md           # This file
├── include/            # Header files
│   ├── common.h        # Shared structs and constants
│   ├── controller.h    # Controller logic prototypes
│   ├── vehicle.h       # Vehicle logic prototypes
│   └── utils.h         # Logging and helper functions
├── src/                # Source code
│   ├── main.c          # Entry point, forks controller
│   ├── controller.c    # Manages vehicle generator
│   ├── vehicle.c       # Vehicle thread routine
│   └── utils.c         # Logging implementation
└── logs/               # Generated log files
```
