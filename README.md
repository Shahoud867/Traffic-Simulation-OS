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

**Phase 4 Complete**: Parking lot management with semaphores:
- Semaphore-based parking system with 10 spots and bounded waiting queue (capacity 5).
- Two-level admission control using `sem_parking_spots` and `sem_waiting_queue` semaphores.
- Vehicles acquire parking before crossing intersection to prevent blocking traffic flow.
- Parking duration simulation (3-7 seconds) with proper resource cleanup and spot release.

**Phase 5 Complete**: Emergency vehicle preemption:
- Emergency vehicles (Ambulance, Firetruck) bypass parking logic entirely.
- EMERGENCY_CLEAR signal sent to next intersection before transit.
- Normal vehicles wait while emergency mode is active (condition variable-based blocking).
- EMERGENCY_PASSED signal clears emergency mode after emergency vehicle crosses.

**Phase 6 In Progress**: SFML Visualization:
- Standalone graphical visualizer using SFML library.
- Real-time log parsing and animated vehicle rendering.
- Visual indicators for intersections, parking, and emergency mode.

## Prerequisites

This project requires a POSIX-compliant environment (Linux).
If you are on **Windows**, you **MUST** use **WSL (Windows Subsystem for Linux)**.

### Required Tools
- `g++` (GNU C++ Compiler with C++17 support)
- `make` (Build automation)
- `libsfml-dev` (Graphics/windowing for visualization)

### Installation (Ubuntu/Debian/WSL)
Run the following commands in your terminal:
```bash
sudo apt update
sudo apt install build-essential libsfml-dev
```

## Building the Project

1. Navigate to the project root directory.
2. Run `make`:
   ```bash
   make          # Build both simulation and visualizer
   make sim      # Build simulation only (no SFML required)
   make vis      # Build visualizer only (requires SFML)
   ```
   This will compile the source code and create the `bin/` directory and `logs/` directory.

## Running the Simulation

Execute the binary:
```bash
# Run Phase 2 (Single Intersection with Quadrants)
./bin/traffic_sim 2 

# Run Phase 3 (Dual Intersections with IPC + Parking)
./bin/traffic_sim 3

# Optional: limit vehicles per intersection (second arg)
# e.g., run Phase 3 and stop each controller after 50 vehicles
./bin/traffic_sim 3 50
```

To stop the simulation, press `Ctrl+C`.

## Running the Visualizer (SFML)

The visualizer runs as a separate process and monitors the log file in real-time:

```bash
# Terminal 1: Start the simulation
./bin/traffic_sim 3 50

# Terminal 2: Start the visualizer (reads logs/traffic_sim.log)
./bin/traffic_visualizer

# Or specify a custom log file path
./bin/traffic_visualizer logs/traffic_sim.log
```

**Visualizer Features:**
- Real-time animated vehicle movement across intersections
- Color-coded vehicle types (Ambulance=Red, Bus=Blue, Car=Gray, etc.)
- Parking lot occupancy display
- Emergency mode visual indicators (red flashing overlay)
- Live event log panel
- Press ESC or close window to exit

**Note for WSL users:** SFML requires a display server. Use WSLg (Windows 11) or an X server like VcXsrv/Xming.

## Output

- **Console**: Displays real-time logs of vehicles arriving and exiting.
- **Log File**: Events are persisted to `logs/traffic_sim.log`.

## Project Structure

```text
.
├── Makefile
├── README.md
├── OS Simulation Project Plan.md
├── OS Project.pdf
├── COMPLIANCE_REPORT.md
├── .gitignore
├── bin
│   ├── traffic_sim          # Main simulation binary
│   └── traffic_visualizer   # SFML visualizer binary
├── include
│   ├── common.hpp
│   ├── controller.hpp
│   ├── display.hpp          # Visualization header
│   ├── parking.hpp
│   ├── utils.hpp
│   └── vehicle.hpp
├── logs
│   └── traffic_sim.log
├── obj
│   └── *.o
└── src
    ├── controller.cpp
    ├── display.cpp          # SFML visualization implementation
    ├── main.cpp
    ├── parking.cpp
    ├── utils.cpp
    ├── vehicle.cpp
    └── visualizer_main.cpp  # Standalone visualizer entry point
```
