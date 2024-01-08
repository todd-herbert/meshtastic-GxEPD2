// Display Library for SPI e-paper panels from Dalian Good Display and boards from Waveshare.
// Requires HW SPI and Adafruit_GFX. Caution: the e-paper panels require 3.3V supply AND data lines!
//
// based on Demo Example from Good Display, available here: http://www.e-paper-display.com/download_detail/downloadsId=806.html
// Panel: ICMEN2R13EFC1 : Heltec Wireless Paper v1.1
// Controller : SSD1680 : https://www.good-display.com/companyfile/101.html
//
// Author: Jean-Marc Zingg
//
// Version: see library.properties
//
// Library: https://github.com/ZinggJM/GxEPD2

#include "GxEPD2_213_FC1.h"

GxEPD2_213_FC1::GxEPD2_213_FC1(int16_t cs, int16_t dc, int16_t rst, int16_t busy, SPIClass &spi) :
  GxEPD2_EPD(cs, dc, rst, busy, LOW, 6000000, WIDTH, HEIGHT, panel, hasColor, hasPartialUpdate, hasFastPartialUpdate, spi)
{
  
}

void GxEPD2_213_FC1::clearScreen(uint8_t value)
{
  writeScreenBuffer(value);
  refresh(true);
}

void GxEPD2_213_FC1::writeScreenBuffer(uint8_t value)
{
    _initial_write = false; // initial full screen buffer clean done
  if (!_using_partial_mode) _Init_Part();
  _writeCommand(0x13); // set current
  for (uint32_t i = 0; i < uint32_t(WIDTH) * uint32_t(HEIGHT) / 8; i++)
  {
    _writeData(value);
  }
  _writeCommand(0x11);
  _writeData(0x00);

    _writeCommand(0x10); // preset previous
    for (uint32_t i = 0; i < uint32_t(WIDTH) * uint32_t(HEIGHT) / 8; i++)
    {
      _writeData(0xFF); // 0xFF is white
    }
  
}

void GxEPD2_213_FC1::writeImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  _writeImage(0x13, bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_213_FC1::_writeImage(uint8_t command, const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (_initial_write) writeScreenBuffer(); // initial full screen buffer clean
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  int16_t wb = (w + 7) / 8; // width bytes, bitmaps are padded
  x -= x % 8; // byte boundary
  w = wb * 8; // byte boundary
  int16_t x1 = x < 0 ? 0 : x; // limit
  int16_t y1 = y < 0 ? 0 : y; // limit
  int16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x; // limit
  int16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y; // limit
  int16_t dx = x1 - x;
  int16_t dy = y1 - y;
  w1 -= dx;
  h1 -= dy;


  if ((w1 <= 0) || (h1 <= 0)) return;
  if (!_using_partial_mode) _Init_Part();
  _writeCommand(0x91); // partial in
  _setPartialRamArea(x1, y1, w1, h1);
  _writeCommand(command);

  //uint8_t extras[h1] = {0};
  
  for (int16_t i = 0; i < h1; i++)
  {
    for (int16_t j = 0; j < wb; j++)
    {
      uint8_t data;
      // use wb, h of bitmap for index!
      int16_t idx = mirror_y ? j + dx / 8 + ((h - 1 - (i + dy))) * wb : j + dx / 8 + (i + dy) * wb;
      if (pgm)
      {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
        data = pgm_read_byte(&bitmap[idx]);
#else
        data = bitmap[idx];
#endif
      }
      else
      {
        data = bitmap[idx];
      }
      if (invert) data = ~data;
      // Here lies an attempt to save those final two lines of the screen, 
      // The first two bits of the byte are what are displayed on each row of the screen
      // I cannot figure out why the buffer is ending like it is, I am guessing the final row is
      // the co
      //if (i < (h1/4)){
      //  if (j == 15){
      //    extras[i*4] = data;
      //    extras[(i*4)+1] = data << 2;
      //    extras[(i*4)+2] = data << 4;
      //    extras[(i*4)+3] = data << 6;
      //    _writeData(extras[i]);
      //  } else {
      //    _writeData(data);
      //  }
      //} else {
        if (j == 15){
          // _writeData(extras[i]);
          _writeData(0xFF);
        } else {
          _writeData(data);
        }
      //}
    }
  }  
  
   _writeCommand(0x92); // partial out
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_213_FC1::writeImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  _writeImagePart(0x13, bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
}


void GxEPD2_213_FC1::_writeImagePart(uint8_t command, const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                     int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (_initial_write) writeScreenBuffer(); // initial full screen buffer clean
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  if ((w_bitmap < 0) || (h_bitmap < 0) || (w < 0) || (h < 0)) return;
  if ((x_part < 0) || (x_part >= w_bitmap)) return;
  if ((y_part < 0) || (y_part >= h_bitmap)) return;
  int16_t wb_bitmap = (w_bitmap + 7) / 8; // width bytes, bitmaps are padded
  x_part -= x_part % 8; // byte boundary
  w = w_bitmap - x_part < w ? w_bitmap - x_part : w; // limit
  h = h_bitmap - y_part < h ? h_bitmap - y_part : h; // limit
  x -= x % 8; // byte boundary
  w = 8 * ((w + 7) / 8); // byte boundary, bitmaps are padded
  int16_t x1 = x < 0 ? 0 : x; // limit
  int16_t y1 = y < 0 ? 0 : y; // limit
  int16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x; // limit
  int16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y; // limit
  int16_t dx = x1 - x;
  int16_t dy = y1 - y;
  w1 -= dx;
  h1 -= dy;
  if ((w1 <= 0) || (h1 <= 0)) return;
  if (!_using_partial_mode) _Init_Part();
  _writeCommand(0x91); // partial in
  _setPartialRamArea(x1, y1, w1, h1);
  _writeCommand(command);
  for (int16_t i = 0; i < h1; i++)
  {
    for (int16_t j = 0; j < w1 / 8; j++)
    {
      uint8_t data;
      // use wb_bitmap, h_bitmap of bitmap for index!
      int16_t idx = mirror_y ? x_part / 8 + j + dx / 8 + ((h_bitmap - 1 - (y_part + i + dy))) * wb_bitmap : x_part / 8 + j + dx / 8 + (y_part + i + dy) * wb_bitmap;
      if (pgm)
      {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
        data = pgm_read_byte(&bitmap[idx]);
#else
        data = bitmap[idx];
#endif
      }
      else
      {
        data = bitmap[idx];
      }
      if (invert) data = ~data;
    }
  }
  _writeData(0xFF);
  _writeCommand(0x92); // partial out
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_213_FC1::writeImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (black)
  {
    writeImage(black, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_213_FC1::writeImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (black)
  {
    writeImagePart(black, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_213_FC1::writeNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (data1)
  {
    writeImage(data1, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_213_FC1::drawImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage(bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_213_FC1::drawImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                   int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImagePart(bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_213_FC1::drawImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (black)
  {
    drawImage(black, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_213_FC1::drawImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                   int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (black)
  {
    drawImagePart(black, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_213_FC1::drawNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (data1)
  {
    drawImage(data1, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_213_FC1::refresh(bool partial_update_mode)
{
  if (partial_update_mode) refresh(0, 0, WIDTH, HEIGHT);
  else
  {
    if (_using_partial_mode) _Init_Full();
    _Update_Full();
    _initial_refresh = false; // initial full update done
  }
}

void GxEPD2_213_FC1::refresh(int16_t x, int16_t y, int16_t w, int16_t h)
{
  if (_initial_refresh) return refresh(false); // initial update needs be full update
  x -= x % 8; // byte boundary
  w -= x % 8; // byte boundary
  int16_t x1 = x < 0 ? 0 : x; // limit
  int16_t y1 = y < 0 ? 0 : y; // limit
  int16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x; // limit
  int16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y; // limit
  w1 -= x1 - x;
  h1 -= y1 - y;
  if (!_using_partial_mode) _Init_Part();
  _writeCommand(0x91); // partial in
  _setPartialRamArea(x1, y1, w1, h1);
  _Update_Part();
  _writeCommand(0x92); // partial out
}

void GxEPD2_213_FC1::powerOff()
{
  _PowerOff();
}

void GxEPD2_213_FC1::hibernate()
{
  _PowerOff();
  if (_rst >= 0)
  {
    _writeCommand(0x07); // deep sleep mode
    _writeData(0xA5);     // enter deep sleep
    _hibernating = true;
  }
}

void GxEPD2_213_FC1::_setPartialRamArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
  uint16_t xe = (x + w - 1) | 0x0007; // byte boundary inclusive (last byte)
  uint16_t ye = y + h - 1;
  x &= 0xFFF8; // byte boundary
  _writeCommand(0x90); // partial window
  _writeData(x % 256);
  _writeData(xe % 256);
  _writeData(y / 256);
  _writeData(y % 256);
  _writeData(ye / 256);
  _writeData(ye % 256);
  //_writeData(0x01); // don't see any difference
  _writeData(0x00); // don't see any differenc
}

void GxEPD2_213_FC1::_PowerOn()
{
  if (!_power_is_on)
  {
    _writeCommand(0x04);
    _waitWhileBusy("_PowerOn", power_on_time);
  }
  _power_is_on = true;
}

void GxEPD2_213_FC1::_PowerOff()
{
  if (_power_is_on)
  {
    _writeCommand(0x02);
    _waitWhileBusy("_PowerOff", power_off_time);
  }
  _power_is_on = false;
  _using_partial_mode = false;
}

void GxEPD2_213_FC1::_InitDisplay()
{
  if (_hibernating) _reset();
  delay(10); // 10ms according to specs
  _writeCommand(0x12);  //SWRESET
  delay(10); // 10ms according to specs

  _writeCommand(0x00);     //panel setting
  _writeData(0x1f);    //LUT from OTP£¬KW-BF   KWR-AF  BWROTP 0f BWOTP 1f
  
  _writeCommand(0x4D); // Dithering?
  _writeData(0x55); // 01010101
  _writeData(0x00); 
  _writeData(0x00);
  _writeCommand(0xA9);
  _writeData(0x25); // 00100101
  _writeData(0x00);
  _writeData(0x00);
  _writeCommand(0xF3);
  _writeData(0x0A); // 00001010
  _writeData(0x00);
  _writeData(0x00);
  
  _writeCommand(0x44); // set Ram-X address start/end position
  _writeData(0x00);
  _writeData(0x0F); // 0x0F-->(15+1)*8=128
  
  _writeCommand(0x45); // set Ram-Y address start/end position
  _writeData(0xF9);	   // 0xF9-->(249+1)=250
  _writeData(0x00);
  _writeData(0x00);
  _writeData(0x00);

  _writeCommand(0x03);
  _writeData(0x00);

  _writeCommand(0x3C); // BorderWavefrom
  _writeData(0x01); // GS/VSS/LUT1
  
  _writeCommand(0x18); // Temperature Sensor Control?
  _writeData(0x80); // Internal Sensor
  
  _writeCommand(0x4E); // set RAM x address count to 0;
  _writeData(0x00);
  _writeCommand(0x4F); // set RAM y address count to 0xF9-->(249+1)=250;
  _writeData(0xF9);
  _writeData(0x00);
}
// experimental partial screen update LUTs, with balanced charge option
// LUTs are filled with zeroes

// this panel doesn't seem to need balanced charge

#define T1  0 // charge balance pre-phase
#define T2  0 // optional extension
#define T3 25 // color change phase (b/w)
#define T4  0 // optional extension for one color
#define T5  0 // white sustain phase
#define T6  0 // black sustain phase

const unsigned char GxEPD2_213_FC1::lut_20_vcomDC_partial[] PROGMEM =
{
  0x00, T1, T2, T3, T4, 1, // 00 00 00 00
};

const unsigned char GxEPD2_213_FC1::lut_21_ww_partial[] PROGMEM =
{ // 10 w
  0x02, T1, T2, T3, T5, 1, // 00 00 00 10
};

const unsigned char GxEPD2_213_FC1::lut_22_bw_partial[] PROGMEM =
{ // 10 w
  0x48, T1, T2, T3, T4, 1, // 01 00 10 00
  //0x5A, T1, T2, T3, T4, 1, // 01 01 10 10 more white
};

const unsigned char GxEPD2_213_FC1::lut_23_wb_partial[] PROGMEM =
{ // 01 b
  0x84, T1, T2, T3, T4, 1, // 10 00 01 00
  //0xA5, T1, T2, T3, T4, 1, // 10 10 01 01 more black
};

const unsigned char GxEPD2_213_FC1::lut_24_bb_partial[] PROGMEM =
{ // 01 b
  0x01, T1, T2, T3, T6, 1, // 00 00 00 01
};

void GxEPD2_213_FC1::_Init_Full()
{
  _InitDisplay();
  _PowerOn();
  _using_partial_mode = false;
}

void GxEPD2_213_FC1::_Init_Part()
{
  _InitDisplay();
  /**/
  if (hasPartialUpdate)
  {
    _writeCommand(0x01); //POWER SETTING
    _writeData (0x03);
    _writeData (0x00);
    _writeData (0x2b);
    _writeData (0x2b);
    _writeData (0x03);

    _writeCommand(0x06); //boost soft start
    _writeData (0x00);   //A
    _writeData (0x00);   //B
    _writeData (0x00);   //C
                         
    // _writeCommand(0x22);
    // _writeData(0xC0);

    _writeCommand(0x30);
    _writeData (0x3C);  // 3A 100HZ   29 150Hz 39 200HZ 31 171HZ
                        
    _writeCommand(0x82); //vcom_DC setting
    _writeData (0x12);

    // _writeCommand(0x50); // Some border setting command could be useful later?
    // _writeData(0x17);

    _writeCommand(0x20);
    _writeDataPGM(lut_20_vcomDC_partial, sizeof(lut_20_vcomDC_partial), 44 - sizeof(lut_20_vcomDC_partial));
    _writeCommand(0x21);
    _writeDataPGM(lut_21_ww_partial, sizeof(lut_21_ww_partial), 42 - sizeof(lut_21_ww_partial));
    _writeCommand(0x22);
    _writeDataPGM(lut_22_bw_partial, sizeof(lut_22_bw_partial), 42 - sizeof(lut_22_bw_partial));
    _writeCommand(0x23);
    _writeDataPGM(lut_23_wb_partial, sizeof(lut_23_wb_partial), 42 - sizeof(lut_23_wb_partial));
    _writeCommand(0x24);
    _writeDataPGM(lut_24_bb_partial, sizeof(lut_24_bb_partial), 42 - sizeof(lut_24_bb_partial));
  }
  _PowerOn();
  _using_partial_mode = true;
}

void GxEPD2_213_FC1::_Update_Full()
{
  _writeCommand(0x12);
  _waitWhileBusy("_Update_Full", full_refresh_time);
  _PowerOff();
}

void GxEPD2_213_FC1::_Update_Part()
{
  _writeCommand(0x12);
  _waitWhileBusy("_Update_Part", partial_refresh_time);
  _PowerOff();
}
