#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

// Global volatile flag for safe signal handling
volatile sig_atomic_t stop_flag = 0;

/**
 * Clean up function - restores fan control to auto mode and exits
 */
void clean_up() {
    printf("End work of program\n");
    system("echo level auto | sudo tee /proc/acpi/ibm/fan");
    exit(0);
}

/**
 * Signal handler for graceful shutdown
 * Sets stop_flag when SIGINT, SIGTERM or SIGTSTP is received
 */
void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM || sig == SIGTSTP) {
        stop_flag = 1;
    }
}

/**
 * Monitors thermal sensors and adjusts fan speed accordingly
 * Uses temperature thresholds to set appropriate fan levels
 */
void monitor_thermal() {
    char buffer[256];
    char text[256];
    int mod = 10;  // Temperature modifier for threshold adjustment
    int temp[8], level = 0, curr_level = 0;
    
    while (stop_flag == 0) {
        // Read thermal data from IBM ACPI interface
        FILE *filed = fopen("/proc/acpi/ibm/thermal", "r");
        if (filed == NULL) {
            perror("Failed to open thermal file");
            sleep(20);
            continue;
        }
        
        fgets(buffer, sizeof(buffer), filed);
        sscanf(buffer, "%s %d %d %d %d %d %d %d %d", 
               text, &temp[0], &temp[1], &temp[2], &temp[3], 
               &temp[4], &temp[5], &temp[6], &temp[7]);

        // Determine fan level based on highest temperature reading
        // Levels 2-6 correspond to increasing fan speeds
        if (temp[0] > 60 - mod || temp[1] > 60 - mod || temp[2] > 60 - mod || 
            temp[3] > 60 - mod || temp[4] > 60 - mod || temp[5] > 60 - mod || 
            temp[6] > 60 - mod || temp[7] > 60 - mod) {
            level = 6;  // Maximum fan speed for critical temperatures
        } else if (temp[0] > 55 - mod || temp[1] > 55 - mod || temp[2] > 55 - mod || 
                  temp[3] > 55 - mod || temp[4] > 55 - mod || temp[5] > 55 - mod || 
                  temp[6] > 55 - mod || temp[7] > 55 - mod) {
            level = 5;  // High fan speed
        } else if (temp[0] > 50 - mod || temp[1] > 50 - mod || temp[2] > 50 - mod || 
                  temp[3] > 50 - mod || temp[4] > 50 - mod || temp[5] > 50 - mod || 
                  temp[6] > 50 - mod || temp[7] > 50 - mod) {
            level = 4;  // Medium-high fan speed
        } else if (temp[0] > 45 - mod || temp[1] > 45 || temp[2] > 45 || 
                  temp[3] > 45 || temp[4] > 45 || temp[5] > 45 || 
                  temp[6] > 45 || temp[7] > 45) {
            level = 3;  // Medium fan speed
        } else if (temp[0] > 40 || temp[1] > 40 - mod || temp[2] > 40 - mod || 
                  temp[3] > 40 - mod || temp[4] > 40 - mod || temp[5] > 40 - mod || 
                  temp[6] > 40 - mod || temp[7] > 40 - mod) {
            level = 2;  // Low fan speed
        } else {
            // Temperatures are normal - use automatic fan control
            system("echo level auto | sudo tee /proc/acpi/ibm/fan");
            printf("level auto\n");
            curr_level = 0;  // Reset current level when returning to auto
        }

        // Only change fan level if it's different from current level
        if (curr_level != level && level != 0) {
            curr_level = level;
            char command[256];
            
            // Construct and execute fan control command
            snprintf(command, sizeof(command), "echo level %d | sudo tee /proc/acpi/ibm/fan", level);
            system(command);
        }
        
        fclose(filed);
        sleep(20);  // Wait 20 seconds between checks
    }
    
    // Clean up when stop_flag is set
    clean_up();
}

/**
 * Main function - sets up signal handlers and starts thermal monitoring
 */
int main(int argc, char *argv[]) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;

    // Register signal handlers for graceful shutdown
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("MONITOR: sigaction(SIGINT) failed");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("MONITOR: sigaction(SIGTERM) failed");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGTSTP, &sa, NULL) == -1) {
        perror("MONITOR: sigaction(SIGTSTP) failed");
        exit(EXIT_FAILURE);
    }

    // Start the thermal monitoring loop
    monitor_thermal();

    return 0;
}
