#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

// Global flags for signal handling
volatile sig_atomic_t stop_flag = 0;   // Set to 1 when program should terminate
volatile sig_atomic_t view_file = 0;   // Set to 1 when config file should be reloaded

/**
 * @brief Cleanup function called on program termination.
 *        Restores fan control to automatic mode.
 */
void clean_up(void) {
    // Restore automatic fan control
    if (system("echo level auto | sudo tee /proc/acpi/ibm/fan > /dev/null 2>&1") == -1) {
        fprintf(stderr, "Warning: Failed to restore automatic fan control.\n");
    }
    printf("End work of program\n");
    exit(EXIT_SUCCESS);
}

/**
 * @brief Signal handler for various termination and reload signals.
 * @param sig The signal number received.
 */
void signal_handler(int sig) {
    switch (sig) {
        case SIGINT:
        case SIGTERM:
        case SIGTSTP:
            stop_flag = 1;
            break;
        case SIGHUP:
            view_file = 1;
            printf("Received SIGHUP: reloading configuration...\n");
            break;
        default:
            // Should not happen, but handle gracefully
            break;
    }
}

/**
 * @brief Reads and parses the configuration file '/etc/fanreg.conf'.
 *        Expected format: "param=<value>", where <value> is "ext", "med", or "max".
 * @return Adjusted temperature threshold modifier:
 *         - 10 for "ext"
 *         - 5  for "med"
 *         - 20 for "max"
 *         - -1 on error or unknown value
 */
int check_file(void) {
    printf("Reloading configuration file...\n");
    view_file = 0;

    FILE *fd = fopen("/etc/fanreg.conf", "r");
    if (!fd) {
        perror("Failed to open /etc/fanreg.conf");
        return -1;
    }

    char buffer[256];
    if (!fgets(buffer, sizeof(buffer), fd)) {
        fprintf(stderr, "Error reading from /etc/fanreg.conf\n");
        fclose(fd);
        return -1;
    }
    fclose(fd);

    char param[256];
    if (sscanf(buffer, "param=%255s", param) != 1) {
        fprintf(stderr, "Invalid config format in /etc/fanreg.conf\n");
        return -1;
    }

    if (strcmp("ext", param) == 0) {
        return 10;
    } else if (strcmp("med", param) == 0) {
        return 5;
    } else if (strcmp("max", param) == 0) {
        return 20;
    } else {
        fprintf(stderr, "Unknown parameter value: %s\n", param);
        return -1;
    }
}

/**
 * @brief Writes the current process ID to '/var/run/fanreg.pid'.
 * @param pid Process ID to write.
 */
void write_pid(pid_t pid) {
    FILE *fd = fopen("/var/run/fanreg.pid", "w");
    if (!fd) {
        perror("Failed to write PID file");
        return; // Non-fatal, continue
    }
    fprintf(fd, "%d\n", (int)pid);
    fclose(fd);
}

/**
 * @brief Main thermal monitoring loop.
 *        Reads CPU temperatures, determines appropriate fan level,
 *        and applies it via ACPI interface.
 */
void monitor_thermal(void) {
    char buffer[256];
    char text[256];
    int mod = 10;                 // Default temperature threshold modifier
    int temp[8];                  // Temperature readings from 8 sensors
    int curr_level = 0;           // Current fan level (0 = auto)

    while (stop_flag == 0) {
        FILE *filed = fopen("/proc/acpi/ibm/thermal", "r");
        if (!filed) {
            perror("Failed to open /proc/acpi/ibm/thermal");
            sleep(20);
            continue;
        }

        if (!fgets(buffer, sizeof(buffer), filed)) {
            fprintf(stderr, "Failed to read thermal data\n");
            fclose(filed);
            sleep(20);
            continue;
        }
        fclose(filed);

        // Parse the thermal line: "temperatures: T1 T2 ... T8"
        int parsed = sscanf(buffer, "%255s %d %d %d %d %d %d %d %d",
                            text, &temp[0], &temp[1], &temp[2], &temp[3],
                            &temp[4], &temp[5], &temp[6], &temp[7]);

        if (parsed < 9) {
            fprintf(stderr, "Unexpected thermal data format\n");
            sleep(20);
            continue;
        }

        // Find maximum temperature among sensors
        int max_temp = temp[0];
        for (int i = 1; i < 8; i++) {
            if (temp[i] > max_temp) {
                max_temp = temp[i];
            }
        }

        // Determine fan level based on max temperature and modifier
        int level = 0;
        if (max_temp > 60 - mod) {
            level = 6;
        } else if (max_temp > 55 - mod) {
            level = 5;
        } else if (max_temp > 50 - mod) {
            level = 4;
        } else if (max_temp > 45 - mod) {
            level = 3;
        } else if (max_temp > 40 - mod) {
            level = 2;
        } else {
            // Below threshold: use automatic mode
            if (curr_level != 0) {
                if (system("echo level auto | sudo tee /proc/acpi/ibm/fan > /dev/null 2>&1") == -1) {
                    fprintf(stderr, "Warning: Failed to set fan to auto\n");
                }
                printf("Fan set to auto (max temp: %d°C)\n", max_temp);
                curr_level = 0;
            }
            // Skip fan level setting below
            goto sleep_and_continue;
        }

        // Apply new fan level only if changed
        if (curr_level != level) {
            char cmd[64];
            snprintf(cmd, sizeof(cmd), "echo level %d | sudo tee /proc/acpi/ibm/fan > /dev/null 2>&1", level);
            if (system(cmd) == -1) {
                fprintf(stderr, "Warning: Failed to set fan level %d\n", level);
            }
            printf("Fan set to level %d (max temp: %d°C)\n", level, max_temp);
            curr_level = level;
        }

    sleep_and_continue:
        // Check if config reload is requested
        if (view_file) {
            int new_mod = check_file();
            if (new_mod != -1) {
                mod = new_mod;
                printf("Modifier updated to %d\n", mod);
            }
            // Reload immediately without waiting
            continue;
        }

        sleep(20);
    }

    clean_up(); // Ensure cleanup on normal exit
}

/**
 * @brief Main function: sets up signal handlers and starts monitoring.
 */
int main(int argc, char *argv[]) {
    (void)argc; // Unused parameter
    (void)argv;

    pid_t pid = getpid();
    printf("Fan regulator started. PID: %d\n", (int)pid);
    write_pid(pid);

    // Setup signal handlers
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;

    if (sigaction(SIGINT,  &sa, NULL) == -1 ||
        sigaction(SIGTERM, &sa, NULL) == -1 ||
        sigaction(SIGTSTP, &sa, NULL) == -1 ||
        sigaction(SIGHUP,  &sa, NULL) == -1) {
        perror("Failed to set up signal handlers");
        exit(EXIT_FAILURE);
    }

    monitor_thermal();
    return 0;
}
