To build needs platformio. Rest can be read in src/main.cpp

Casing just plain PLA and 4 M2 bolts.
Glued some large nuts on the inside to weigth down 
and some hotglue on the bolts to make it less slippery.

--building--

The macro definitions can be given during the build-process via a cli. Or it can be done via the header file in source.
For the latter the last line of the platformio.ini files needs to be commented out. 

--operation--

settings can be found in `macros_example.hpp`and I recommend changing the timings to your liking.
Device needs stmDuino bootloader.

After the device is flashed and setup it does:
    It first starts the bootloader, after which the red led wil flash (high frequency).
    You may enter a pincode to unlock the device, applying with "*" key.
    If the pincode is correct the green led should lightup.
    If wrong, the green and red led should light-up momentarly (orange if led in the same package).
    The pincode can then be retried. After a number of wrong attempts, the device will lockout for some time.
    Then it may be retried. NOTE this lockout can be circumvented once power is lost TODO fix this).
    If then again the attemp limit is reached, the device will wipe its own memory, except of the bootloader.

    If the pincode was correct, the green led will light up.
    Now macros can be used by short pressing any of the buttons, or the device can be switched to numpad mode by a long-press "#".
    One may also whish to go into bootloader mode, then long-hold D. Then de device will restart.
    After some time of inactivity, you will be logged out and the pincode will need to be re-applied.
    This is indicated by the red led starting to flash. You'll get a warning of this timeout by the green led starting to flash.
    You can also manually logout by a long-pres on the "*" button.

    The login timeout can be disabled by going into numpad mode (long-press #) and long-pressing D. The led will momentarly turn green when the timeout is disabled, otherwise red for re-enabled.

TODO:  
    Encryption of macros
    use HAL/NVIC
    rewrite pin detection
    write better readme
