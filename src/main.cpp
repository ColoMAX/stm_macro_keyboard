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
#include <Keyboard.h>
#include <stm32f1xx_hal_cortex.h>
#include "mbedtls/config.h"
#include "mbedtls/sha1.h"

#include <macros_example.hpp>





Keypad customKeypad =
    Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

CircularBuffer<char, sizeof(psk) - 1> buffer;

volatile unsigned long lastSeen = 0;
volatile bool initialized = false;
volatile bool numlock_litteral = false;

void enable_flash_read_protection() {
  HAL_FLASH_Unlock();
  HAL_FLASH_OB_Unlock();
  FLASH_OBProgramInitTypeDef pOBInit_for_readprot;
  HAL_FLASHEx_OBGetConfig(&pOBInit_for_readprot);
   // no protection
  if (pOBInit_for_readprot.RDPLevel == OB_RDP_LEVEL_0) { 
    // enable protection
    pOBInit_for_readprot.OptionType = OPTIONBYTE_RDP;
    pOBInit_for_readprot.RDPLevel = OB_RDP_LEVEL_1;
    HAL_FLASHEx_OBProgram(&pOBInit_for_readprot);
    HAL_FLASH_OB_Launch();
  }
  HAL_FLASH_OB_Lock();
  HAL_FLASH_Lock();
  
}
enum led_on { none, red, green };

void led_driver(led_on which) {
  switch (which) {
    case red:
      // TODO change to direct port manipulation or
      //    to use the hal (CMSIS)
      BILED(digitalWrite(PIN_ANODE_RED, HIGH));
      BILED(digitalWrite(PIN_ANODE_GREEN, LOW));
      digitalWrite(PIN_LED_BUILTIN, LOW);
      break;
    case green:
      BILED(digitalWrite(PIN_ANODE_RED, LOW));
      BILED(digitalWrite(PIN_ANODE_GREEN, HIGH));
      digitalWrite(PIN_LED_BUILTIN, HIGH);
      break;
    case none:
      BILED(digitalWrite(PIN_ANODE_RED, LOW));
      BILED(digitalWrite(PIN_ANODE_GREEN, LOW));
      digitalWrite(PIN_LED_BUILTIN, HIGH);
      break;
    default:
      // dont care, should not be possible
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
                Keyboard.print(maps[i][j]);
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
          bool passwordCorrect;
          for (int i = 0; i < buffer.size(); i++) {
            passwordCorrect = buffer[i] == psk[i];
            if (!passwordCorrect) break;
          }
          if (passwordCorrect) {
            // led_driver(green);
            initialized = true;
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
          HAL_NVIC_SystemReset();
          break;
        case '*':  // logout
          initialized = false;
          break;
        default:
          break;
      }
  }
}

void setup() {
  if (READ_PROTECTION)
    enable_flash_read_protection();

  lastSeen = millis();
  Keyboard.begin();

  // TODO use CMSIS
  pinMode(PIN_LED_BUILTIN, OUTPUT);
  BILED(pinMode(PIN_ANODE_RED, OUTPUT));
  BILED(pinMode(PIN_ANODE_GREEN, OUTPUT));

  customKeypad.setDebounceTime(DEBOUCE_TIME);
  customKeypad.addEventListener(keypadEvent);
}

void blinker_uninitialized(bool initialized) {
  // if bi_led is turned off, this part is doing a lot 
  //    of non-relavent computation
  static unsigned long last_seen_minor = millis();
  static enum {
    first_blink,
    first_blink_end,
    short_pause,
    second_blink,
    second_blink_end,
    long_pause
  } state = long_pause;
  if (initialized) {
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
        // led_driver(none);
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
  customKeypad.getKeys();  // needed for eventListener,  ;(
  
  if (initialized && PSKTIMEOUT && (millis() - lastSeen) >= PSKTIMEOUT) {
    initialized = false;
    buffer.clear();
    // led_driver(red);
  }

  blinker_uninitialized(initialized);
}