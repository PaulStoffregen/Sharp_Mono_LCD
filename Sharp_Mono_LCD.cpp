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

 You may need to adjust the SPI Clock Divisor to get the fastest reliable
 operation.
 Although the Display is rated for 1Mhz, I was able to use a divisor of 8 
 on the Due (12 MHz) and a divisor of 6 on the Zero (8 MHz).

 You can change the SHARPMEM_LCDWIDTH and SHARPMEM_LCDHEIGHT in 
 Sharp_Mono_LCD.h for the size of display you are using.
 
*********************************************************************/

#include "Sharp_Mono_LCD.h"
#include <SPI.h>

#ifndef _swap_int16_t
#define _swap_int16_t(a, b) { int16_t t = a; a = b; b = t; }
#endif
#ifndef _swap_uint16_t
#define _swap_uint16_t(a, b) { uint16_t t = a; a = b; b = t; }
#endif

/**************************************************************************
    Adafruit Sharp Memory Display Connector
    -----------------------------------------------------------------------
    Pin   Function        Notes
    ===   ==============  ===============================
      1   VIN             3.3-5.0V (into LDO supply)
      2   3V3             3.3V out
      3   GND
      4   SCLK            SPI Hardware Serial Clock Pin
      5   MOSI            SPI Hardware Serial Data Input Pin
      6   CS              Serial Chip Select (can be any pin)
      9   EXTMODE         COM Inversion Select (Low = SW clock/serial)
      7   EXTCOMIN        External COM Inversion Signal
      8   DISP            Display On(High)/Off(Low)

 **************************************************************************/

#define SHARPMEM_BIT_WRITECMD   (0x01)
#define SHARPMEM_BIT_VCOM       (0x02)
#define SHARPMEM_BIT_CLEAR      (0x04)
#define TOGGLE_VCOM             do { _sharpmem_vcom = _sharpmem_vcom ? 0x00 : SHARPMEM_BIT_VCOM; } while (0);

byte sharpmem_buffer[(SHARPMEM_LCDWIDTH * SHARPMEM_LCDHEIGHT) / 8];

/* ************* */
/* CONSTRUCTORS  */
/* ************* */
Sharp_Mono_LCD::Sharp_Mono_LCD(uint8_t ss) :
  Adafruit_GFX(SHARPMEM_LCDWIDTH, SHARPMEM_LCDHEIGHT) {

  _ss = ss;
  digitalWrite(_ss, HIGH);
  pinMode(_ss, OUTPUT);

  // Set the vcom bit to a defined state
  _sharpmem_vcom = SHARPMEM_BIT_VCOM;
}

void Sharp_Mono_LCD::begin() {
  setRotation(2);

#if defined(__SAM3X8E__)
  SPI.begin(_ss);
  SPI.setClockDivider(8); // may have to adjust this clock divisor for best operation
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(LSBFIRST);
#else
  SPI.begin();
  SPI.setClockDivider(6);  //may have to adjust this clock divisor for best operation
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(LSBFIRST);
#endif
  clearDisplay();

}

// 1<<n is a costly operation on AVR -- table usu. smaller & faster
static const uint8_t PROGMEM
set[] = {  1,  2,  4,  8,  16,  32,  64,  128 },
        clr[] = { (uint8_t)(~1), (uint8_t)(~2), (uint8_t)(~4), (uint8_t)(~8),
                  (uint8_t)(~16), (uint8_t)(~32), (uint8_t)(~64), (uint8_t)(~128) };

/**************************************************************************/
/*!
    @brief Draws a single pixel in image buffer

    @param[in]  x
                The x position (0 based)
    @param[in]  y
                The y position (0 based)
*/
/**************************************************************************/
void Sharp_Mono_LCD::drawPixel(int16_t x, int16_t y, uint16_t color)
{
  if ((x < 0) || (x >= _width) || (y < 0) || (y >= _height)) return;

  switch (rotation) {
    case 1:
      _swap_int16_t(x, y);
      x = WIDTH  - 1 - x;
      break;
    case 2:
      x = WIDTH  - 1 - x;
      y = HEIGHT - 1 - y;
      break;
    case 3:
      _swap_int16_t(x, y);
      y = HEIGHT - 1 - y;
      break;
  }

  if (color) {
    sharpmem_buffer[(y * SHARPMEM_LCDWIDTH + x) / 8] |=
      pgm_read_byte(&set[x & 7]);
  } else {
    sharpmem_buffer[(y * SHARPMEM_LCDWIDTH + x) / 8] &=
      pgm_read_byte(&clr[x & 7]);
  }
}

/**************************************************************************/
/*!
    @brief Clears the screen
*/
/**************************************************************************/
void Sharp_Mono_LCD::clearDisplay()
{
  memset(sharpmem_buffer, 0xff, (SHARPMEM_LCDWIDTH * SHARPMEM_LCDHEIGHT) / 8);
  // Send the clear screen command rather than doing a HW refresh (quicker)
  digitalWrite(_ss, HIGH);
  SPI.transfer(_sharpmem_vcom | SHARPMEM_BIT_CLEAR); //change to LSB
  SPI.transfer(0x00);
  TOGGLE_VCOM;
  digitalWrite(_ss, LOW);
}

/**************************************************************************/
/*!
    @brief Clears the buffer
*/
/**************************************************************************/
void Sharp_Mono_LCD::clearBuffer()
{
  memset(sharpmem_buffer, 0xff, (SHARPMEM_LCDWIDTH * SHARPMEM_LCDHEIGHT) / 8);
}

/**************************************************************************/
/*!
    @brief Renders the contents of the pixel buffer on the LCD
*/
/**************************************************************************/
void Sharp_Mono_LCD::refresh(void)
{
  uint16_t i, totalbytes, currentline, oldline;
  totalbytes = (SHARPMEM_LCDWIDTH * SHARPMEM_LCDHEIGHT) / 8;

  // Send the write command
  digitalWrite(_ss, HIGH);
  SPI.transfer(SHARPMEM_BIT_WRITECMD | _sharpmem_vcom);
  TOGGLE_VCOM;

  // Send the address for line 1
  oldline = currentline = 1;
  SPI.transfer(currentline);

  // Send image buffer
  for (i = 0; i < totalbytes; i++)
  {
    SPI.transfer(sharpmem_buffer[i]);
    currentline = ((i + 1) / (SHARPMEM_LCDWIDTH / 8)) + 1;
    if (currentline != oldline)
    {
      // Send end of line and address bytes
      SPI.transfer(0x00);
      if (currentline <= SHARPMEM_LCDHEIGHT)
      {
        SPI.transfer(currentline);
      }
      oldline = currentline;
    }
  }

  // Send another trailing 8 bits for the last line
  SPI.transfer(0x00);
  digitalWrite(_ss, LOW);
}
