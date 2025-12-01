#include <cstdio>
#include <cstring>
#include "parking.hpp"
#include "utils.hpp"

// Initialize parking lot with semaphores
void init_parking_lot(ParkingLot* parking, int spots, int queue_capacity) {
    parking->total_spots = spots;
    parking->queue_capacity = queue_capacity;
    parking->parked_count = 0;
    parking->waiting_count = 0;
    
    // Initialize semaphores
    sem_init(&parking->sem_parking_spots, 0, spots);
    sem_init(&parking->sem_waiting_queue, 0, queue_capacity);
    
    pthread_mutex_init(&parking->stats_lock, NULL);
    
    char log_msg[256];
    sprintf(log_msg, "Parking lot initialized: %d spots, %d queue capacity", 
            spots, queue_capacity);
    log_event(log_msg);
}

// Cleanup parking lot resources
void cleanup_parking_lot(ParkingLot* parking) {
    sem_destroy(&parking->sem_parking_spots);
    sem_destroy(&parking->sem_waiting_queue);
    pthread_mutex_destroy(&parking->stats_lock);
    
    log_event("Parking lot cleaned up");
}

// Check if vehicle type can park (Emergency vehicles don't park)
bool can_vehicle_park(VehicleType type) {
    return (type == CAR || type == BIKE || type == TRACTOR || type == BUS);
}

// Try to acquire parking (returns true if successful)
bool try_acquire_parking(ParkingLot* parking, int vehicle_id) {
    char log_msg[256];
    
    // Try to enter waiting queue (non-blocking check)
    int queue_val;
    sem_getvalue(&parking->sem_waiting_queue, &queue_val);
    
    if (queue_val <= 0) {
        sprintf(log_msg, "Vehicle %d: Waiting queue full, rerouting", vehicle_id);
        log_event(log_msg);
        return false;
    }
    
    // Acquire waiting queue slot
    sprintf(log_msg, "Vehicle %d: Entering parking waiting queue", vehicle_id);
    log_event(log_msg);
    
    sem_wait(&parking->sem_waiting_queue);
    
    // Update waiting count
    pthread_mutex_lock(&parking->stats_lock);
    parking->waiting_count++;
    pthread_mutex_unlock(&parking->stats_lock);
    
    sprintf(log_msg, "Vehicle %d: Waiting for parking spot...", vehicle_id);
    log_event(log_msg);
    
    // Wait for actual parking spot
    sem_wait(&parking->sem_parking_spots);
    
    // Got parking spot, release queue slot
    sem_post(&parking->sem_waiting_queue);
    
    // Update counts
    pthread_mutex_lock(&parking->stats_lock);
    parking->waiting_count--;
    parking->parked_count++;
    pthread_mutex_unlock(&parking->stats_lock);
    
    sprintf(log_msg, "Vehicle %d: ACQUIRED parking spot (Total parked: %d/10)", 
            vehicle_id, parking->parked_count);
    log_event(log_msg);
    
    return true;
}

// Release parking spot
void release_parking(ParkingLot* parking, int vehicle_id) {
    pthread_mutex_lock(&parking->stats_lock);
    parking->parked_count--;
    int current_parked = parking->parked_count;
    pthread_mutex_unlock(&parking->stats_lock);
    
    sem_post(&parking->sem_parking_spots);
    
    char log_msg[256];
    sprintf(log_msg, "Vehicle %d: RELEASED parking spot (Total parked: %d/10)", 
            vehicle_id, current_parked);
    log_event(log_msg);
}

// Get parking statistics
void get_parking_stats(ParkingLot* parking, int* parked, int* waiting) {
    pthread_mutex_lock(&parking->stats_lock);
    *parked = parking->parked_count;
    *waiting = parking->waiting_count;
    pthread_mutex_unlock(&parking->stats_lock);
}
