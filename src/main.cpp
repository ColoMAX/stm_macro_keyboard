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

  The macros will be saved in PLAIN TEXT in the memory of the STM, but the
  device can be put in write-only mode. Unfortuanntly there are known
  vulnerabilitys.

  You'll have to put in a pincode after connecting the device to a computer, and
  after 10 minuies of inactivity as a safety precaution.

  The code can be optimized further, aswell as be made more secure. Security can
  be enhanced by using an encryption method for the macros and pincode. Another
  mothod is that the pincode IS the key to the encryption of the macros.
  Optimization can be applyied on how the keys are being registerd.

  Major improvement would be to create a global FSM.
  Also mutiple pages/tabs could be incorporated of macros by
   for example holding down a macro key, you go to a new page/mode
  with different macros mappings. Or by utilizing the four
  rightsize buttons as page select buttons.
  Thus creating 256 or 36 macros buttons instead of 16.

*/

#include <Arduino.h>
#include <CircularBuffer.h>
#include <Keyboard.h>
#include <Keypad.h>
#include <stm32f1xx_hal_cortex.h>

// not used yet
//#include "mbedtls/config.h"
//#include <mbedtls/blowfish.h>

#include <macros_example.hpp>

#define expr 0

// experimental eeprom.
// does show weird behaviour, so disabled.
#if expr
#include "eewl.h"
#else
#endif

Keypad keypadObj = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

CircularBuffer<char, sizeof(psk) - 1> buffer;

volatile unsigned long lastSeen = 0;
volatile bool initialized = false;
volatile bool numlock_litteral = false;

volatile int psk_false_count = 0;
volatile int psk_false_count_last = 0;
volatile bool disable_timeout = false;

#if expr
struct data_saved {
  int count;
  int count_last;
} cs = {0, 0};
EEWL pC(cs, FLASH_PAGE_SIZE / sizeof(cs), 0););
#endif

enum led_on { repeat, none, red, green, orange };
enum green_blink_state { green_start_blink, green_end_blink, green_stable };

void enable_flash_read_protection();
void system_wipe();
void led_driver(led_on which = repeat);
void print_macro(KeypadEvent key);
void psk_handle(KeypadEvent key);
void keypadEvent(KeypadEvent key);
void logout_warning(unsigned long* last_seen_minor, led_on color);
void blinker_handler(bool initialized);

void setup() {
  if (READ_PROTECTION) enable_flash_read_protection();
#if expr
  EEPROM.begin();
#endif

  lastSeen = millis();
  Keyboard.begin();

  pinMode(PIN_LED_BUILTIN, OUTPUT);
  BILED(pinMode(PIN_ANODE_RED, OUTPUT));
  BILED(pinMode(PIN_ANODE_GREEN, OUTPUT));

  keypadObj.setDebounceTime(DEBOUCE_TIME);
  keypadObj.addEventListener(keypadEvent);
}

void loop() {
  keypadObj.getKeys();  // needed for eventListener,  ;(
  if (disable_timeout) lastSeen = millis();

  if (!disable_timeout && initialized && PSKTIMEOUT &&
      (millis() - lastSeen) >= PSKTIMEOUT) {
    initialized = false;
    buffer.clear();
    // led_driver(red);
  }

  blinker_handler(initialized);
  // led_driver();
}

/**
 * @brief Enable the flash read protection.
 * Takes reset-by-power to take effect.
 *
 */
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

__RAM_FUNC HAL_StatusTypeDef
HAL_FLASHEx_Erase(FLASH_EraseInitTypeDef* pEraseInit, uint32_t* PageError);
__RAM_FUNC HAL_StatusTypeDef HAL_FLASH_Lock();
__RAM_FUNC void HAL_NVIC_SystemReset();

__RAM_FUNC void system_wipe() {
  HAL_FLASH_Unlock();

  // wipe all except bootloader
  uint32_t PageError;
  FLASH_EraseInitTypeDef S_erase{
      .TypeErase = FLASH_TYPEERASE_PAGES,
      .Banks = FLASH_BANK_1,
      .PageAddress = USER_PROGRAM_START,
      // static_assert(IS_FLASH_NB_PAGES())
      .NbPages = (FLASH_START_LAST_PAGE - USER_PROGRAM_START) /
                 FLASH_PAGE_SIZE  // FIXME may be wrong
  };
  HAL_FLASHEx_Erase(&S_erase, &PageError);
  HAL_FLASH_Lock();
  HAL_NVIC_SystemReset();
}

/**
 * @brief low level led_driver, for external and internal.
 * Arduino style.
 * @param which enum led_on. for orange needs frequent calls.
 */
void led_driver(led_on which) {
  static led_on lastState = which;
  static led_on lastColor = none;
  if (which == repeat) {
    which = lastState;
  }

  switch (which) {
  red_driver:
  case red:
    BILED(digitalWrite(PIN_ANODE_RED, HIGH));
    BILED(digitalWrite(PIN_ANODE_GREEN, LOW));
    digitalWrite(PIN_LED_BUILTIN, LOW);
    lastColor = red;
    break;
  green_driver:
  case green:
    BILED(digitalWrite(PIN_ANODE_RED, LOW));
    BILED(digitalWrite(PIN_ANODE_GREEN, HIGH));
    digitalWrite(PIN_LED_BUILTIN, HIGH);
    lastColor = green;
    break;
  none_driver:
  case none:
    BILED(digitalWrite(PIN_ANODE_RED, LOW));
    BILED(digitalWrite(PIN_ANODE_GREEN, LOW));
    digitalWrite(PIN_LED_BUILTIN, HIGH);
    lastColor = none;
    break;
  orange_driver:
  case orange:
    if (lastColor == green) {
      goto red_driver;
    } else {
      goto green_driver;
    }
    break;
    default:
      // dont care, should not be possible
      break;
  }
}

/**
 * @brief write out macro via keyboard.
 *
 * @param key Registered key on numpad. (char)
 */
void print_macro(KeypadEvent key) {
  // brute force search.
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLS; j++) {
      if (hexaKeys[i][j] == key) {
        Keyboard.print(maps[i][j]);
        return;
      }
    }
  }
}

/**
 * @brief Check updated pincode, set initialized if correct, and light up led.
 * Pincode is a circulair buffer.
 * @param key Registered key on numpad. (char)
 */
void psk_handle(KeypadEvent key) {
  buffer.push(key);
  // only once "*" is pressed, the pincode is checked
  if (buffer.isFull() && buffer.last() == psk_end_char) {
    bool passwordCorrect;
    for (int i = 0; i < buffer.size(); i++) {
      passwordCorrect = buffer[i] == psk[i];
      if (!passwordCorrect) break;
    }
    if (passwordCorrect) {
      led_driver(green);
      initialized = true;
      psk_false_count_last = psk_false_count = 0;
#if expr
      cs.count = psk_false_count;
      pC.put(cs);
#endif
    } else {
#if expr
      pC.get(cs);
      psk_false_count = cs.count;
#endif

      psk_false_count++;
#if expr
      cs.count = psk_false_count;
      pC.put(cs);
#endif
      if (psk_false_count > 0 && psk_false_count % psk_lockout_max == 0) {
        if (psk_false_count % psk_wipe_count == 0) {
          system_wipe();
        } else {
          led_driver(red);
          unsigned long last_time = millis();
          // block for some time
          while ((millis() - last_time < psk_lockout_time)) {
            yield();
          }
        }
      }
    }
  }
}

/**
 * @brief handles keypress events.
 *
 * @param key Registered key on numpad. (char)
 */
void keypadEvent(KeypadEvent key) {
  // TODO clean by use of fsm
  static bool switched_mode = false;

  switch (keypadObj.getState()) {
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
          print_macro(key);
        } else {
          // numpad mode.
          Keyboard.write(key);
        }
      } else {
        // pincode mode
        psk_handle(key);
      }
      break;

    case HOLD:
      switch (key) {
        case '#':  // switch between numpad and macro mode
          if (initialized) {
            numlock_litteral = !numlock_litteral;
            switched_mode = true;
          }
          break;
        case 'D':
          if (!initialized | !numlock_litteral) {
            // reset and enter bootloader.
            HAL_NVIC_SystemReset();
          } else {
            switched_mode = true;
            disable_timeout = !disable_timeout;
            led_on color = disable_timeout ? green : red;
            unsigned long now = millis();
            // block
            while ((millis() - now) < 500) {
              yield();
              led_driver(color);
            }
          }
          break;
        case '*':  // logout
          initialized = false;
          break;
        default:
          break;
      }
  }
}

/**
 * @brief Handle initilized -macro mode green led functions.
 *
 * @param last_seen_minor last led change time (ms)
 */
void logout_warning(unsigned long* last_seen_minor, led_on color) {
  static green_blink_state gstate = green_start_blink;
  // almost out of time?
  if (PSKTIMEOUT && (millis() - lastSeen) < PSKTIMEOUT_WARNING ||
      disable_timeout) {
    // nah
    led_driver(color);
  } else {
    // ALMOST OUT OF TIME!
    switch (gstate) {
      case green_start_blink:
        led_driver(none);
        if ((millis() - *last_seen_minor) >= 100) {
          gstate = green_end_blink;
          *last_seen_minor = millis();
        }
        break;
      case green_end_blink:  // make sure to wait for one period.
        led_driver(color);
        gstate = green_stable;
        break;
      case green_stable:
        led_driver(color);
        if ((millis() - *last_seen_minor) >= 900) {
          gstate = green_start_blink;
          *last_seen_minor = millis();
        }
        break;
      default:
        break;
    }
  }
}

bool false_handle() {
  static enum { first, start_orange, timeout } state = timeout;
  bool active = true;
  unsigned long current_time = millis();
  static unsigned long last_seen_minor = current_time;

  // led_driver(orange);
  switch (state) {
    case first:
      last_seen_minor = current_time;
      state = start_orange;
      // led_driver(orange);
      // active = true;
      break;
    case start_orange:
      if ((current_time - last_seen_minor > 200)) {
        last_seen_minor = current_time;
        state = timeout;
        psk_false_count_last = psk_false_count;
      }
      led_driver(orange);
      // active = true;
      break;
    case timeout:
      if (psk_false_count_last < psk_false_count) state = first;
      active = false;
      break;
    default:
      state = timeout;
      break;
  }
  return active;
}

/**
 * @brief Overall led controll.
 *
 * @param initialized logged in flag
 */
void blinker_handler(bool initialized) {
  // if bi_led option is turned off, this part is doing a lot
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
    logout_warning(&last_seen_minor, numlock_litteral ? orange : green);
    state = first_blink;

  } else {
    if (!false_handle()) {
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
  }
}