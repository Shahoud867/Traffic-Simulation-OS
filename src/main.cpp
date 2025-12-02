#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <sys/wait.h>
#include <csignal>
#include "common.hpp"
#include "controller.hpp"
#include "utils.hpp"

//Global variables for signal handling
pid_t f10_pid = -1;
pid_t f11_pid = -1;
int pipe_f10_to_f11[2] = {-1,-1};
int pipe_f11_to_f10[2] = {-1,-1};

void handle_sigint(int sig) {
    (void)sig;
    //Only print to console if not running in QUIET mode
    if (getenv("QUIET") == NULL) {
        printf("\nCaught SIGINT, shutting down...\n");
    }
    log_event("SIGINT received - initiating shutdown");
    
    //Send shutdown signal through pipes if they exist
    if (pipe_f10_to_f11[1] > 0) {
        IPCMessage shutdown_msg;
        shutdown_msg.type = SHUTDOWN;
        write(pipe_f10_to_f11[1], &shutdown_msg, sizeof(IPCMessage));
    }
    if (pipe_f11_to_f10[1] > 0) {
        IPCMessage shutdown_msg;
        shutdown_msg.type = SHUTDOWN;
        write(pipe_f11_to_f10[1], &shutdown_msg, sizeof(IPCMessage));
    }
    
    //Kill child processes
    if (f10_pid > 0) {
        kill(f10_pid, SIGTERM);
        waitpid(f10_pid, NULL, 0);
    }
    if (f11_pid > 0) {
        kill(f11_pid, SIGTERM);
        waitpid(f11_pid, NULL, 0);
    }
    
    //Close all pipe ends
    if (pipe_f10_to_f11[0] > 0) close(pipe_f10_to_f11[0]);
    if (pipe_f10_to_f11[1] > 0) close(pipe_f10_to_f11[1]);
    if (pipe_f11_to_f10[0] > 0) close(pipe_f11_to_f10[0]);
    if (pipe_f11_to_f10[1] > 0) close(pipe_f11_to_f10[1]);
    
    log_event("Simulation terminated cleanly");
    exit(0);
}

void run_phase2_demo(int max_vehicles) {
    if (getenv("QUIET") == NULL) {
        printf("=== Running Phase 2 Demo: Single Intersection with Quadrant Synchronization ===\n");
    }
    log_event("Phase 2 Demo Started");

    //Run single intersection with quadrant mutexes
    ControllerContext ctx;
    strcpy(ctx.name,"F10");
    ctx.id = INTERSECTION_F10;
    ctx.write_fd = -1;  //No IPC
    ctx.read_fd = -1;
    ctx.running = true;
    ctx.max_vehicles = max_vehicles;

    run_controller_with_ipc(&ctx);
}

void run_phase3_demo(int max_vehicles) {
    if (getenv("QUIET") == NULL) {
        printf("=== Running Phase 3 Demo: Dual Intersections with IPC ===\n");
    }
    log_event("Phase 3 Demo Started");

    //Create bidirectional pipes
    if (pipe(pipe_f10_to_f11) == -1 || pipe(pipe_f11_to_f10) == -1) {
        perror("Pipe creation failed");
        log_event("Failed to create pipes");
        exit(1);
    }
    
    log_event("Pipes created successfully");
    
    // Fork F10 controller
    f10_pid = fork();
    if (f10_pid < 0) {
        perror("Fork failed for F10");
        exit(1);
    } 
    else if (f10_pid == 0) {
        // Child process F10
        srand(time(NULL) ^ getpid());
        
        // Close unused pipe ends
        close(pipe_f10_to_f11[0]);  // Close read end of pipe we write to
        close(pipe_f11_to_f10[1]);  // Close write end of pipe we read from
        
        // Setup controller context
    ControllerContext ctx;
        strcpy(ctx.name, "F10");
        ctx.id = INTERSECTION_F10;
        ctx.write_fd = pipe_f10_to_f11[1];  // Write to F11
        ctx.read_fd = pipe_f11_to_f10[0];   // Read from F11
    ctx.max_vehicles = max_vehicles;
        
        run_controller_with_ipc(&ctx);
        
        close(pipe_f10_to_f11[1]);
        close(pipe_f11_to_f10[0]);
        exit(0);
    }
    
    // Fork F11 controller
    f11_pid = fork();
    if (f11_pid < 0) {
        perror("Fork failed for F11");
        kill(f10_pid, SIGTERM);
        exit(1);
    } 
    else if (f11_pid == 0) {
        // Child process F11
        srand(time(NULL) ^ getpid());
        
        // Close unused pipe ends
        close(pipe_f10_to_f11[1]);  // Close write end of pipe we read from
        close(pipe_f11_to_f10[0]);  // Close read end of pipe we write to
        
        // Setup controller context
    ControllerContext ctx;
        strcpy(ctx.name, "F11");
        ctx.id = INTERSECTION_F11;
        ctx.write_fd = pipe_f11_to_f10[1];  // Write to F10
        ctx.read_fd = pipe_f10_to_f11[0];   // Read from F10
    ctx.max_vehicles = max_vehicles;
        
        run_controller_with_ipc(&ctx);
        
        close(pipe_f11_to_f10[1]);
        close(pipe_f10_to_f11[0]);
        exit(0);
    }
    
    // Parent process - close all pipe ends (children have their own copies)
    close(pipe_f10_to_f11[0]);
    close(pipe_f10_to_f11[1]);
    close(pipe_f11_to_f10[0]);
    close(pipe_f11_to_f10[1]);
    
    // Wait for both children
    int status;
    pid_t finished = waitpid(-1, &status, 0);
    if (getenv("QUIET") == NULL) printf("Child process %d finished\n", finished);

    finished = waitpid(-1, &status, 0);
    if (getenv("QUIET") == NULL) printf("Child process %d finished\n", finished);
    
    log_event("Phase 3 Demo Completed");
}

int main(int argc, char* argv[]) {
    signal(SIGINT, handle_sigint);
    
    if (getenv("QUIET") == NULL) {
        printf("Traffic Simulation - OS Project\n");
        printf("================================\n");
    }
    
    // Check command line arguments
    int phase = 3;  // Default to Phase 3
    int max_vehicles = 0; // <=0 means unlimited
    if (argc > 1) {
        phase = atoi(argv[1]);
    }
    if (argc > 2) {
        max_vehicles = atoi(argv[2]);
    }
    
    log_event("=== Traffic Simulation Started ===");
    
    if (phase == 2) {
        run_phase2_demo(max_vehicles);
    } else if (phase == 3) {
        run_phase3_demo(max_vehicles);
    } else {
        printf("Usage: %s [phase_number]\n", argv[0]);
        printf("  phase_number: 2 (single intersection) or 3 (dual intersections with IPC)\n");
        printf("  Optional: specify max_vehicles as second argument (e.g., '%s 3 100')\n", argv[0]);
        printf("  Default: 3\n");
        return 1;
    }

    return 0;
}
