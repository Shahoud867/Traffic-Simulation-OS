#ifndef COMMON_HPP
#define COMMON_HPP

#include <sys/types.h>
#include <pthread.h>
#include <time.h>

// Constants
#define MAX_PARKING_SPOTS 10
#define MAX_WAITING_QUEUE 5 // Example bounded queue size
#define SIMULATION_TIME_SEC 30 // Default simulation time

// Enums
typedef enum {
    AMBULANCE,
    FIRETRUCK,
    BUS,
    CAR,
    BIKE,
    TRACTOR
} VehicleType;

typedef enum {
    NORTH,
    SOUTH,
    EAST,
    WEST
} Direction;

typedef enum {
    INTERSECTION_F10,
    INTERSECTION_F11
} IntersectionID;

// Phase 2: Quadrant System for Intersection
typedef enum {
    NW = 0,  // North-West
    NE = 1,  // North-East
    SW = 2,  // South-West
    SE = 3   // South-East
} Quadrant;

// Phase 3: IPC Message Types
typedef enum {
    TRANSIT = 0,           // Vehicle transitioning between intersections
    EMERGENCY_CLEAR = 1,   // Emergency vehicle approaching
    ACK = 2,               // Acknowledgment
    SHUTDOWN = 3           // Graceful shutdown signal
} MessageType;

// Phase 2: Intersection State with Quadrant Mutexes
typedef struct {
    pthread_mutex_t quadrant_mutexes[4];
    bool emergency_mode;
    pthread_mutex_t emergency_lock;
    int vehicles_in_intersection;
    pthread_mutex_t count_lock;
} IntersectionState;

// Vehicle Structure
typedef struct {
    int id;
    VehicleType type;
    Direction entry;
    Direction dest;
    IntersectionID current_intersection;
    time_t arrival_time;
    // Add more fields as needed (speed, etc.)
} Vehicle;

// Phase 3: IPC Message Structure
typedef struct {
    MessageType type;
    int vehicle_id;
    VehicleType v_type;
    Direction entry_point;  // For destination intersection
    Direction destination;
    time_t timestamp;
} IPCMessage;

// Forward declaration for controller context
struct ControllerContext;

// Vehicle Arguments for thread creation
typedef struct {
    int id;
    VehicleType type;
    Direction entry;
    Direction dest;
    IntersectionID intersection_id;
    struct ControllerContext* controller_ctx;
} VehicleArgs;

// Phase 3: Controller Context
typedef struct ControllerContext {
    char name[10];  // "F10" or "F11"
    int write_fd;   // Pipe to send messages to other intersection
    int read_fd;    // Pipe to receive messages from other intersection
    IntersectionState* state;
    pthread_t ipc_listener_thread;
    IntersectionID id;
    volatile bool running;
} ControllerContext;

#endif // COMMON_HPP
