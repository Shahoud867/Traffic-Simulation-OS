#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "vehicle.h"
#include "utils.h"

const char* VehicleTypeNames[] = {
    "Ambulance", "Firetruck", "Bus", "Car", "Bike", "Tractor"
};

const char* DirectionNames[] = {
    "North", "South", "East", "West"
};

void* vehicle_routine(void* arg) {
    Vehicle* v = (Vehicle*)arg;
    char log_msg[256];

    sprintf(log_msg, "Vehicle %d (%s) Arrived at %s", 
            v->id, VehicleTypeNames[v->type], DirectionNames[v->entry]);
    log_event(log_msg);

    // Simulate travel time
    usleep((rand() % 2000 + 1000) * 1000); // 1-3 seconds

    sprintf(log_msg, "Vehicle %d (%s) Exited towards %s", 
            v->id, VehicleTypeNames[v->type], DirectionNames[v->dest]);
    log_event(log_msg);

    free(v);
    return NULL;
}

void* vehicle_generator(void* arg) {
    IntersectionID* id = (IntersectionID*)arg;
    int vehicle_count = 0;
    
    char start_msg[100];
    sprintf(start_msg, "Vehicle Generator started for Intersection %d", *id);
    log_event(start_msg);

    while (1) {
        Vehicle* v = malloc(sizeof(Vehicle));
        v->id = ++vehicle_count;
        v->type = rand() % 6;
        v->entry = rand() % 4;
        do {
            v->dest = rand() % 4;
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
