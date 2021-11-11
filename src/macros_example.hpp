#include <Arduino.h>

/* HOLD "D" to start bootlaoder (reset device), hold "#" to switch between macro and just keyvalue.
When power applied, first type in passcode ending with a "*" to activate device, after the timeout, the passswork needs to be put in again. LED will light up when device is unlocked.   */

// TODO add encription to the macros

// DO NOT ALTER
const byte ROWS = 4;
const byte COLS = 4;

// broken
//#define USE_HASH_HMAC 

// setting this to false after flashed once with true, will not disable
//      the read protection!
constexpr bool READ_PROTECTION = true; // enable flash read protection

// enable the use of external led's
#define enable_bi_led true

// key labels
const char hexaKeys[ROWS][COLS] = {{'1', '2', '3', 'A'},
                                   {'4', '5', '6', 'B'},
                                   {'7', '8', '9', 'C'},
                                   {'*', '0', '#', 'D'}};

// put your macro's in here
const char* maps[ROWS][COLS] = {{"1", "2\n", "3", "4"},
                                {"cmake3 ..\n", "5", "6", "B"},
                                {"7", "8", "9", "C"},
                                {"*", "0", "#", "D"}};


const char psk[] = "1234*"; // pincode

const unsigned long PSKTIMEOUT = 10*1000*60; // ms:  10 min
const unsigned long BLINK_PERIOD = 500;    // ms: 500 ms
const uint DEBOUCE_TIME = 10;               //ms

const byte PIN_LED_BUILTIN = PC13;
const byte PIN_ANODE_GREEN = PB11;
const byte PIN_ANODE_RED = PB10;

byte rowPins[ROWS] = {PA15, PB3, PB4, PB5};
byte colPins[COLS] = {PB6, PB7, PB8, PB9};

#if enable_bi_led
#define BILED(x) x
#else
#define BILED(x)
#endif