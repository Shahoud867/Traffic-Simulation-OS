#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <pthread.h>
#include <cstring>
#include "vehicle.hpp"
#include "utils.hpp"

const char* VehicleTypeNames[] = {
    "Ambulance", "Firetruck", "Bus", "Car", "Bike", "Tractor"
};

const char* DirectionNames[] = {
    "North", "South", "East", "West"
};

// Phase 2: Determine which quadrants a vehicle needs based on entry and destination
// Returns number of quadrants needed, fills the quadrants array
int get_required_quadrants(Direction entry, Direction dest, int* quadrants) {
    int count = 0;
    
    // Straight paths (2 quadrants)
    if (entry == NORTH && dest == SOUTH) { 
        quadrants[0] = NW; quadrants[1] = SW; count = 2;
    }
    else if (entry == SOUTH && dest == NORTH) { 
        quadrants[0] = NE; quadrants[1] = SE; count = 2;
    }
    else if (entry == EAST && dest == WEST) { 
        quadrants[0] = NE; quadrants[1] = NW; count = 2;
    }
    else if (entry == WEST && dest == EAST) { 
        quadrants[0] = SE; quadrants[1] = SW; count = 2;
    }
    
    // Right turns (1 quadrant)
    else if (entry == NORTH && dest == EAST) { 
        quadrants[0] = NE; count = 1;
    }
    else if (entry == EAST && dest == SOUTH) { 
        quadrants[0] = SE; count = 1;
    }
    else if (entry == SOUTH && dest == WEST) { 
        quadrants[0] = SW; count = 1;
    }
    else if (entry == WEST && dest == NORTH) { 
        quadrants[0] = NW; count = 1;
    }
    
    // Left turns (3 quadrants)
    else if (entry == NORTH && dest == WEST) { 
        quadrants[0] = NW; quadrants[1] = SE; quadrants[2] = SW; count = 3;
    }
    else if (entry == WEST && dest == SOUTH) { 
        quadrants[0] = NE; quadrants[1] = SE; quadrants[2] = SW; count = 3;
    }
    else if (entry == SOUTH && dest == EAST) { 
        quadrants[0] = NE; quadrants[1] = NW; quadrants[2] = SE; count = 3;
    }
    else if (entry == EAST && dest == NORTH) { 
        quadrants[0] = NE; quadrants[1] = NW; quadrants[2] = SW; count = 3;
    }
    
    // Sort quadrants to ensure consistent lock ordering (bubble sort)
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (quadrants[j] > quadrants[j + 1]) {
                int temp = quadrants[j];
                quadrants[j] = quadrants[j + 1];
                quadrants[j + 1] = temp;
            }
        }
    }
    
    return count;
}

// Phase 2: Acquire quadrants in order
void acquire_quadrants(pthread_mutex_t* mutexes, int* quads, int count, int vehicle_id) {
    for (int i = 0; i < count; i++) {
        pthread_mutex_lock(&mutexes[quads[i]]);
        char log_msg[256];
        sprintf(log_msg, "Vehicle %d acquired quadrant %d", vehicle_id, quads[i]);
        log_event(log_msg);
    }
}

// Phase 2: Release quadrants
void release_quadrants(pthread_mutex_t* mutexes, int* quads, int count, int vehicle_id) {
    for (int i = 0; i < count; i++) {
        pthread_mutex_unlock(&mutexes[quads[i]]);
        char log_msg[256];
        sprintf(log_msg, "Vehicle %d released quadrant %d", vehicle_id, quads[i]);
        log_event(log_msg);
    }
}

// Phase 2: Get crossing time based on vehicle type (in milliseconds)
int get_crossing_time(VehicleType type) {
    switch(type) {
        case AMBULANCE: return 1000;  // 1 second (fastest)
        case FIRETRUCK: return 1200;  // 1.2 seconds
        case CAR:       return 1500;  // 1.5 seconds
        case BUS:       return 2000;  // 2 seconds
        case BIKE:      return 1800;  // 1.8 seconds
        case TRACTOR:   return 3000;  // 3 seconds (slowest)
        default:        return 2000;
    }
}

void* vehicle_routine(void* arg) {
    VehicleArgs* vargs = (VehicleArgs*)arg;
    char log_msg[256];

    sprintf(log_msg, "Vehicle %d (%s) Arrived at %s intersection %s", 
            vargs->id, VehicleTypeNames[vargs->type], 
            vargs->controller_ctx->name, DirectionNames[vargs->entry]);
    log_event(log_msg);

    // Phase 2: Get required quadrants
    int quadrants[4];  // Max 3 quadrants needed
    int quad_count = get_required_quadrants(vargs->entry, vargs->dest, quadrants);
    
    sprintf(log_msg, "Vehicle %d requesting %d quadrant(s) for %s->%s", 
            vargs->id, quad_count, DirectionNames[vargs->entry], DirectionNames[vargs->dest]);
    log_event(log_msg);

    // Phase 2: Acquire quadrants (safe crossing)
    acquire_quadrants(vargs->controller_ctx->state->quadrant_mutexes, quadrants, quad_count, vargs->id);
    
    sprintf(log_msg, "Vehicle %d (%s) ENTERING intersection %s", 
            vargs->id, VehicleTypeNames[vargs->type], vargs->controller_ctx->name);
    log_event(log_msg);

    // Simulate crossing time based on vehicle type
    int crossing_time = get_crossing_time(vargs->type);
    usleep(crossing_time * 1000);

    sprintf(log_msg, "Vehicle %d (%s) EXITING intersection %s towards %s", 
            vargs->id, VehicleTypeNames[vargs->type], 
            vargs->controller_ctx->name, DirectionNames[vargs->dest]);
    log_event(log_msg);

    // Phase 2: Release quadrants
    release_quadrants(vargs->controller_ctx->state->quadrant_mutexes, quadrants, quad_count, vargs->id);

    // Phase 3: Check if vehicle transits to other intersection
    if (vargs->controller_ctx->write_fd > 0) {  // IPC enabled
        bool should_transit = false;
        Direction new_entry = NORTH;
        
        if (strcmp(vargs->controller_ctx->name, "F10") == 0 && vargs->dest == EAST) {
            should_transit = true;
            new_entry = WEST;  // Enters F11 from west
        }
        else if (strcmp(vargs->controller_ctx->name, "F11") == 0 && vargs->dest == WEST) {
            should_transit = true;
            new_entry = EAST;  // Enters F10 from east
        }
        
        if (should_transit) {
            IPCMessage msg;
            msg.type = TRANSIT;
            msg.vehicle_id = vargs->id;
            msg.v_type = vargs->type;
            msg.entry_point = new_entry;
            // Continue in same direction after transit
            msg.destination = vargs->dest;
            msg.timestamp = time(NULL);
            
            ssize_t written = write(vargs->controller_ctx->write_fd, &msg, sizeof(IPCMessage));
            if (written > 0) {
                sprintf(log_msg, "Vehicle %d transited from %s to %s", 
                        vargs->id, vargs->controller_ctx->name, 
                        strcmp(vargs->controller_ctx->name, "F10") == 0 ? "F11" : "F10");
                log_event(log_msg);
            } else {
                sprintf(log_msg, "Vehicle %d failed to send transit message", vargs->id);
                log_event(log_msg);
            }
        } else {
            sprintf(log_msg, "Vehicle %d exited the system from %s", 
                    vargs->id, vargs->controller_ctx->name);
            log_event(log_msg);
        }
    } else {
        sprintf(log_msg, "Vehicle %d exited the system", vargs->id);
        log_event(log_msg);
    }

    delete vargs;
    return NULL;
}

void* vehicle_generator(void* arg) {
    IntersectionID* id = (IntersectionID*)arg;
    int vehicle_count = 0;
    
    char start_msg[100];
    sprintf(start_msg, "Vehicle Generator started for Intersection %d", *id);
    log_event(start_msg);

    while (1) {
        Vehicle* v = (Vehicle*)malloc(sizeof(Vehicle));
        v->id = ++vehicle_count;
        v->type = (VehicleType)(rand() % 6);
        v->entry = (Direction)(rand() % 4);
        do {
            v->dest = (Direction)(rand() % 4);
        } while (v->dest == v->entry); // Destination must be different from entry
        v->current_intersection = *id;

        pthread_t tid;
        if (pthread_create(&tid, NULL, vehicle_routine, v) != 0) {
            log_event("Failed to create vehicle thread");
            free(v);
        } else {
            pthread_detach(tid);
        }

        // Random spawn interval
        usleep((rand() % 1000 + 500) * 1000); // 0.5-1.5 seconds
    }
    return NULL;
}

// Phase 3: Vehicle generator with controller context
void* vehicle_generator_with_context(void* arg) {
    ControllerContext* ctx = (ControllerContext*)arg;
    int vehicle_count = 0;
    
    char start_msg[100];
    sprintf(start_msg, "Vehicle Generator started for Intersection %s", ctx->name);
    log_event(start_msg);

    while (ctx->running) {
        VehicleArgs* vargs = new VehicleArgs();
        vargs->id = ++vehicle_count;
        vargs->type = (VehicleType)(rand() % 6);
        vargs->entry = (Direction)(rand() % 4);
        
        // Generate destination different from entry
        do {
            vargs->dest = (Direction)(rand() % 4);
        } while (vargs->dest == vargs->entry);
        
        vargs->intersection_id = ctx->id;
        vargs->controller_ctx = ctx;

        pthread_t tid;
        if (pthread_create(&tid, NULL, vehicle_routine, vargs) != 0) {
            log_event("Failed to create vehicle thread");
            delete vargs;
        } else {
            pthread_detach(tid);
        }

        // Random spawn interval: 0.5-1.5 seconds
        usleep((rand() % 1000 + 500) * 1000);
    }
    
    sprintf(start_msg, "Vehicle Generator stopped for Intersection %s", ctx->name);
    log_event(start_msg);
    return NULL;
}
