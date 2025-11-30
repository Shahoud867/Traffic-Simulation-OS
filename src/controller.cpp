#include <cstdio>
#include <cstdlib>
#include <pthread.h>
#include <unistd.h>
#include "controller.h"
#include "vehicle.h"
#include "utils.h"

void run_controller(IntersectionID id) {
    char log_msg[100];
    sprintf(log_msg, "Controller for Intersection %d started", id);
    log_event(log_msg);

    pthread_t generator_thread;
    IntersectionID arg = id;
    
    if (pthread_create(&generator_thread, NULL, vehicle_generator, &arg) != 0) {
        log_event("Failed to create vehicle generator thread");
        exit(1);
    }

    // Join generator (it runs forever, so this blocks)
    pthread_join(generator_thread, NULL);
}
