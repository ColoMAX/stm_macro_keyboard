#include <Arduino.h>

/* HOLD "D" to start bootlaoder (reset device), hold "#" to switch between macro
and just keyvalue.
When power applied, first type in passcode ending with a "*" to activate device,
after the timeout, the passswork needs to be put in again. LED will light up
when device is unlocked.   */

// DO NOT ALTER ///////////////////
const byte ROWS = 4;
const byte COLS = 4;
#define xstr(s) str(s)
#define str(s) #s
// DO NOT ALTER ///////////////////

// OPTIONS ///////////////////////////////////////////////////////////

// setting this to false after flashed once with true, will not disable
//      the read protection!
constexpr bool READ_PROTECTION = true;  // enable flash read protection

// enable the use of external led's (green and red)
#define enable_bi_led true

// PIN DEFINITIONS
// bi-led
const byte PIN_ANODE_GREEN = PB11;
const byte PIN_ANODE_RED = PB10;

byte rowPins[ROWS] = {PA15, PB3, PB4, PB5};
byte colPins[COLS] = {PB6, PB7, PB8, PB9};

// key labels / numpad mode
const char hexaKeys[ROWS][COLS] = {{'1', '2', '3', 'A'},
                                   {'4', '5', '6', 'B'},
                                   {'7', '8', '9', 'C'},
                                   {'*', '0', '#', 'D'}};

#ifndef USE_SCRIPT_MACROS
// example of macros. For sacurity, the build script includes an interface
// to type in the macro's during buildtime, instead of having it in the source.
const char* maps[ROWS][COLS] = {
    {"1", "2\n", "3", "MyPassworD12"},
    {"cmake3 ..\n", "echo\n", "beep", "B"},
    {"7", "cowsay hi\n", "xclock\n",
     "sudo apt-get update -y && sudo apt-get upgrade -y\n"},
    {"*", "0", "#",
     "ssh -L 3000:login.server.nl:22"
     "usr@login.nl\n"}};
#else
const char* maps[4][4] = {{xstr(MACRO_1_NAME), xstr(MACRO_2_NAME),
                           xstr(MACRO_3_NAME), xstr(MACRO_A_NAME)},
                          {xstr(MACRO_4_NAME), xstr(MACRO_5_NAME),
                           xstr(MACRO_6_NAME), xstr(MACRO_B_NAME)},
                          {xstr(MACRO_7_NAME), xstr(MACRO_8_NAME),
                           xstr(MACRO_9_NAME), xstr(MACRO_C_NAME)},
                          {xstr(MACRO_S_NAME), xstr(MACRO_0_NAME),
                           xstr(MACRO_H_NAME), xstr(MACRO_D_NAME)}};
#endif

// pincode
#define psk_ "1234"
// apply pincode char
#define end_psk *

// number of tries before lockout
const int psk_lockout_max = 5;
// time locked out after psk_lockout_max wrong attempts.
//      Does reset on power reset
const unsigned long psk_lockout_time = 3 * 3600;
// number of lockouts untill flash wipe.
const int psk_wipe_count = 2;

// auto-logout in ms of inactivity:  10 min
const unsigned long PSKTIMEOUT = 10 * 1000 * 60;
// auto-logout warning in ms:  5 min
const unsigned long PSKTIMEOUT_WARNING = PSKTIMEOUT * 0.5;

// DO NOT ALTER ////////////////////////////////////////////////////////
const byte PIN_LED_BUILTIN = PC13;

const unsigned long BLINK_PERIOD = 500;  // ms: 500 ms
const uint DEBOUCE_TIME = 10;            // ms

#define USER_PROGRAM_START 0x08005000
#define FLASH_START_LAST_PAGE 0x0801FC00

#if enable_bi_led
#define BILED(x) x
#else
#define BILED(x)
#endif

static_assert(PSKTIMEOUT > PSKTIMEOUT_WARNING);

const char psk[] = psk_ xstr(end_psk);
const char psk_end_char = xstr(end_psk)[0];
// DO NOT ALTER /////////////////////////////////////////////////////////