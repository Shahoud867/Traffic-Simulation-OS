#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include "utils.h"

#define LOG_FILE "logs/traffic_sim.log"

void log_event(const char* msg) {
    FILE* fp = fopen(LOG_FILE, "a");
    if (fp == NULL) {
        perror("Failed to open log file");
        return;
    }

    time_t now;
    time(&now);
    char* date = ctime(&now);
    date[strlen(date) - 1] = '\0'; // Remove newline

    fprintf(fp, "[%s] %s\n", date, msg);
    printf("[%s] %s\n", date, msg); // Also print to console

    fclose(fp);
}

double get_current_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}
