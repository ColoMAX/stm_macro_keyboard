#include <Arduino.h>

/* HOLD "D" to start bootlaoder (reset device), hold "#" to switch between macro and just keyvalue.
When power applied, first type in passcode ending with a "*" to activate device, after the timeout, the passswork needs to be put in again. LED will light up when device is unlocked.   */

// TODO add encription to the macros

const byte ROWS = 4;
const byte COLS = 4;

//#define USE_HASH_HMAC 

const char* maps[ROWS][COLS] = {{"1", "2\n", "3", "4"},
                                {"cmake3 ..\n", "5", "6", "B"},
                                {"7", "8", "9", "C"},
                                {"*", "0", "#", "D"}};

const char psk[] = "1234*";

const unsigned long PSKTIMEOUT = 10*1000*60; // ms:  10 min

const unsigned long BLINK_PERIOD = 500;    // ms: 500 ms