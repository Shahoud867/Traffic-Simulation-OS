#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <sys/wait.h>
#include <csignal>
#include "common.h"
#include "controller.h"
#include "utils.h"

pid_t f10_pid;

void handle_sigint(int sig) {
    (void)sig;
    printf("\nCaught SIGINT, shutting down...\n");
    if (f10_pid > 0) {
        kill(f10_pid, SIGTERM);
        waitpid(f10_pid, NULL, 0);
    }
    log_event("Simulation terminated.");
    exit(0);
}

int main() {
    signal(SIGINT, handle_sigint);
    
    log_event("Traffic Simulation Started");

    f10_pid = fork();
    if (f10_pid < 0) {
        perror("Fork failed");
        exit(1);
    } else if (f10_pid == 0) {
        // Child Process: Controller F10
        srand(time(NULL) ^ getpid()); // Seed RNG
        run_controller(INTERSECTION_F10);
        exit(0);
    }

    // Parent Process
    // Wait for child or signal
    wait(NULL);

    return 0;
}
