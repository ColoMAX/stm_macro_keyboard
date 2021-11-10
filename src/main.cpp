/* colomax at revelation space Den Haag 2021
  A STM32F103C8T6 "bluepill" based macro keyboard with a
  membrane matrix keypad.

  The device needs an usb bootloader for optimal function, which can be
  aquired from https://github.com/rogerclarkmelbourne/STM32duino-bootloader
  ["generic_boot20_pc13.bin"]. This can be uploaded by the STM32CUBEProgrammer
  and a stlinkV2.

  It used the STM32Duino core, which does have a few limitations (at least at
  the time of building this). The Maple core does better in some regard, but has
  other hurdles.
    - On option to use Serial (over USB) is not supported by this core in
  combination with HID. This could be added manually.
    - Option to tranfer data by stats-led's of numlock and capslock is not
    supported by the ST-keyboard library. This could be added manually.

  Change the macros in the header, hold D to reset the device to go into
  bootloader mode, which will be exited after 1 second. One will need to dfu
  drivers (maple) to flash via usb.

  The macros will be saved in PLAIN TEXT in the memory of the STM, thus no
  protection from firmware reads!

  You'll have to put in a pincode after connecting the device to a computer, and
  after 10 minuies of inactivity as a safety precaution.

  The code can be optimized further, aswell as be made more secure. Security can
  be enhanced by using an encryption method for the macros and pincode. Another
  mothod is that the pincode IS the key to the encryption of the macros.
  Optimization can be applyied on how the keys are being registerd.

  ! THE MACROS FOR SHA & MAPLE CORE DO NOT WORK (YET)!
*/

#include <Arduino.h>
#include <CircularBuffer.h>
#include <Keypad.h>
#include <stm32f1xx_hal_cortex.h>

#include <macros_example.hpp>

//#include <hmac.h>
//#include "sha256.h"

#ifdef USE_HASH_HMAC
uint8_t hash[sizeof(psk)];
// char hash_r[256];
#endif

#define MAPLE 0

#if MAPLE
#include <USBComposite.h>
USBHID HID;
HIDKeyboard Keyboard(HID);
#define SerialUSB CompositeSerial
USBCompositeSerial CompositeSerial;
#else
#include <Keyboard.h>
#include <USBSerial.h>
#define SerialUSB Serial
#endif

const char hexaKeys[ROWS][COLS] = {{'1', '2', '3', 'A'},
                                   {'4', '5', '6', 'B'},
                                   {'7', '8', '9', 'C'},
                                   {'*', '0', '#', 'D'}};

byte rowPins[ROWS] = {PA15, PB3, PB4, PB5};
byte colPins[COLS] = {PB6, PB7, PB8, PB9};

Keypad customKeypad =
    Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

const byte PIN_LED_BUILTIN = PC13;
const byte PIN_ANODE_GREEN = PB11;
const byte PIN_ANODE_RED = PB10;

CircularBuffer<char, sizeof(psk) - 1> buffer;

volatile unsigned long lastSeen = 0;
volatile bool initialized = false;
volatile bool numlock_litteral = false;

enum led_on { none, red, green };

void led_driver(led_on which) {
  switch (which) {
    case red:
      // TODO change to direct port manipulation or
      //    to use the hal (CMSIS)
      digitalWrite(PIN_ANODE_RED, HIGH);
      digitalWrite(PIN_ANODE_GREEN, LOW);
      digitalWrite(PIN_LED_BUILTIN, LOW);
      break;
    case green:
      digitalWrite(PIN_ANODE_RED, LOW);
      digitalWrite(PIN_ANODE_GREEN, HIGH);
      digitalWrite(PIN_LED_BUILTIN, HIGH);
      break;
    case none:
      digitalWrite(PIN_ANODE_RED, LOW);
      digitalWrite(PIN_ANODE_GREEN, LOW);
      digitalWrite(PIN_LED_BUILTIN, HIGH);
      break;
    default:
      // dont care
      break;
  }
}

// Taking care of some special events.
void keypadEvent(KeypadEvent key) {
  // TODO clean by use of fsm
  static bool switched_mode = false;

  switch (customKeypad.getState()) {
    case IDLE:
      break;
    case PRESSED:
      break;
    case RELEASED:
      lastSeen = millis();
      if (initialized) {
        if (switched_mode) {
          switched_mode = false;
          return;
        }
        // macro mode
        if (!numlock_litteral) {
          for (int i = 0; i < ROWS; i++) {
            for (int j = 0; j < COLS; j++) {
              if (hexaKeys[i][j] == key) {
#ifdef USE_HASH_HMAC
                Sha256.print(maps[i][j]);
                Keyboard.print((char*)Sha256.resultHmac());
#else
                Keyboard.print(maps[i][j]);
#endif
                SerialUSB.print("Key pressed\n");
                break;
              }
            }
          }
        } else {
          // numpad mode.
          Keyboard.write(key);
        }
      } else {
        // pincode mode
        buffer.push(key);
        if (buffer.isFull()) {
          bool passwordCorrect = false;
#ifdef USE_HASH_HMAC
          passwordCorrect = buffer[buffer.size() - 1] == '*';
#else
          for (int i = 0; i < buffer.size(); i++) {
            passwordCorrect = buffer[i] == psk[i];
            if (!passwordCorrect) break;
          }
#endif
          if (passwordCorrect) {
            //led_driver(green);
            initialized = true;
#ifdef USE_HASH_HMAC

            Sha256.initHmac(hash,
                            buffer.size());  // key, and length of key in bytes
            // Sha256.print("This is a message to hash");
#else

#endif
          }
        }
      }
      break;

    case HOLD:
      switch (key) {
        case '#':  // switch between numpad and macro mode
          numlock_litteral = !numlock_litteral;
          switched_mode = true;
          break;
        case 'D':  // reset and enter bootloader.
#if MAPLE
          nvic_sys_reset();
#else
          HAL_NVIC_SystemReset();
#endif
          break;
          case '*': // logout
            initialized = false;
            break;
        default:
          break;
      }
  }
}
void setup() {
  lastSeen = millis();
#if MAPLE
  HID.begin(CompositeSerial, HID_KEYBOARD);
  while (!USBComposite)
    ;
  Keyboard.begin();  // needed for LED support
#else
  SerialUSB.begin(115200);
  Keyboard.begin();
#endif
  // TODO use CMSIS
  pinMode(PIN_LED_BUILTIN, OUTPUT);
  pinMode(PIN_ANODE_RED, OUTPUT);
  pinMode(PIN_ANODE_GREEN, OUTPUT);
  //led_driver(red);
  // ledPin_state = digitalRead(
  //    PIN_LED_BUILTIN);  // Store initial LED state. HIGH when LED is on.

  // customKeypad.begin()
  customKeypad.setDebounceTime(10);
  customKeypad.addEventListener(keypadEvent);
}

void blinker_uninitialized(bool initialized) {
  static unsigned long last_seen_minor = millis();
  static enum {
    first_blink,
    first_blink_end,
    short_pause,
    second_blink,
    second_blink_end,
    long_pause
  } state = long_pause;
  if(initialized){
    led_driver(green);
    state = first_blink;
    return;
  }
  switch (state) {
    case first_blink:
      led_driver(red);
      state = first_blink_end;
      last_seen_minor = millis();
      break;
    case first_blink_end:
      if ((millis() - last_seen_minor) >= 50) {
        led_driver(none);
        state = short_pause;
        last_seen_minor = millis();
      }
      break;
    case short_pause:
      if ((millis() - last_seen_minor) >= 100) {
        state = second_blink;
        last_seen_minor = millis();
      }
      break;
    case second_blink:
      led_driver(red);
      state = second_blink_end;
      last_seen_minor = millis();
      break;
    case second_blink_end:
      if ((millis() - last_seen_minor) >= 50) {
        led_driver(none);
        state = long_pause;
        last_seen_minor = millis();
      }
      break;
    case long_pause:
      if ((millis() - last_seen_minor) >= BLINK_PERIOD) {
        //led_driver(none);
        state = first_blink;
        last_seen_minor = millis();
      }
      break;
    default:
      state = long_pause;
      break;
  }
}

void loop() {
  char customKey = customKeypad.getKey();  // needed for eventListener ;(
  if (initialized && PSKTIMEOUT && (millis() - lastSeen) >= PSKTIMEOUT) {
    initialized = false;
    buffer.clear();
    //led_driver(red);
  }
   
  blinker_uninitialized(initialized);
}