#pragma once

#ifdef RGBLIGHT_ENABLE
#    define RGBLED_NUM 54 // Number of LEDs
#    define RGBLED_SPLIT \
        { 27, 27 }
#    define RGBLIGHT_SPLIT
#endif

#ifdef RGB_MATRIX_ENABLE
#    define RGBLED_NUM       54 // Number of LEDs
#    define RGB_MATRIX_LED_COUNT RGBLED_NUM
#    define RGB_MATRIX_SPLIT \
        { 27, 27 }
#    define SPLIT_TRANSPORT_MIRROR
#endif


#ifdef OLED_ENABLE
    #define OLED_UPDATE_INTERVAL 80
#endif
