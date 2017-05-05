/*********************************************************************
  This is a modified Adafruit library for the monochrome SHARP Memory Displays.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

    Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/products/1393

  Written by Limor Fried/Ladyada for Adafruit Industries.
  BSD license, check license.txt for more information

 *********************************************************************

  Modified by John Schnurr, Sept 2016. Modified to use the Hardware SPI Pins.
  Faster frames can be achieved using the SPI pins.
  Verified to work on a Mega, Due, and Zero.

  The Hardware pins required are:
                     MOSI              SCK                SS
  Arduino Mega:     51 (or ICSP 4)    52 (or ICSP 3)     Any Pin
  Arduino Due:      ICSP 4            ICSP 3             Any Pin
  Arduino Mega:     ICSP 4            ICSP 3             Any Pin

  You may need to adjust the SPI Clock Divisor to get the fastet operation.
  Although the Display is rated for 1Mhz, I was able to use a divisor of 8
  on the Due (12 MHz) and a divisor of 6 on the Zero (8 MHz).

  You can change the SHARPMEM_LCDWIDTH and SHARPMEM_LCDHEIGHT in
  Sharp_Mono_LCD.h for the size of display you are using.

*********************************************************************/

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <Adafruit_GFX.h>
#ifdef __AVR
#include <avr/pgmspace.h>
#elif defined(ESP8266)
#include <pgmspace.h>
#endif

// LCD Dimensions
#define SHARPMEM_LCDWIDTH       (128)
#define SHARPMEM_LCDHEIGHT      (128)

class Sharp_Mono_LCD : public Adafruit_GFX {
  public:
    Sharp_Mono_LCD(uint8_t ss);
    void begin(void);
    void drawPixel(int16_t x, int16_t y, uint16_t color);
    void clearDisplay();
    void clearBuffer();
    void refresh(void);

  private:
    uint8_t _ss;
#ifdef __AVR__
    uint8_t _sharpmem_vcom,;
#endif
#if defined(__SAM3X8E__) || defined(ARDUINO_ARCH_SAMD) || defined(__arm__)
    uint32_t _sharpmem_vcom;
#endif

};
