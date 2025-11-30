#ifndef COMMON_HPP
#define COMMON_HPP

#include <sys/types.h>

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

// Vehicle Structure
typedef struct {
    int id;
    VehicleType type;
    Direction entry;
    Direction dest;
    IntersectionID current_intersection;
    // Add more fields as needed (speed, arrival_time, etc.)
} Vehicle;

// IPC Message Structure (for Phase 3+)
typedef struct {
    int type; // 0=Transit, 1=Emergency_Clear, 2=Ack
    int vehicle_id;
    VehicleType v_type;
} IPC_Message;

#endif // COMMON_HPP
