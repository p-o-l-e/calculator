//
// ss_oled (Small, Simple OLED library)
// Copyright (c) 2017-2019 BitBank Software, Inc.
// Written by Larry Bank (bitbank@pobox.com)
// Project started 1/15/2017
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "grotesque_font.h"
#include "ss_oled.h"


const unsigned char oled64_initbuf[]={0x00,0xae,0xa8,0x3f,0xd3,0x00,0x40,0xa1,0xc8,
      0xda,0x12,0x81,0xff,0xa4,0xa6,0xd5,0x80,0x8d,0x14,
      0xaf,0x20,0x02};



// some globals
static int iCSPin, iDCPin, iResetPin;
#define MAX_CACHE 32
static void oledWriteCommand(SSOLED *pOLED, unsigned char c);
void InvertBytes(uint8_t *pData, uint8_t bLen);


//
// Invert font data
//
void InvertBytes(uint8_t *pData, uint8_t bLen)
{
    uint8_t i;
    for (i=0; i<bLen; i++)
    {
        *pData = ~(*pData);
        pData++;
    }
} /* InvertBytes() */


// wrapper/adapter functions to make the code work on Linux
static uint8_t pgm_read_byte(uint8_t *ptr)
{
    return *ptr;
}

static int16_t pgm_read_word(uint8_t *ptr)
{
    return ptr[0] + (ptr[1]<<8);
}

static void _I2CWrite(SSOLED *pOLED, unsigned char *pData, int iLen)
{
    I2CWrite(&pOLED->bbi2c, pOLED->oled_addr, pData, iLen);
} /* _I2CWrite() */

//
// Initializes the OLED controller into "page mode"
//
int oledInit(SSOLED *pOLED, int iType, int iAddr, int bFlip, int bInvert, int bWire, int sda, int scl, int reset, int32_t iSpeed)
{
    unsigned char uc[4];
    int rc = OLED_NOT_FOUND;

    pOLED->ucScreen = NULL; // reset backbuffer; user must provide one later
    pOLED->oled_type = iType;
    pOLED->oled_flip = bFlip;
    pOLED->oled_wrap = 0; // default - disable text wrap
    #ifdef _LINUX_
    pOLED->bbi2c.iBus = sda; // bus number
    #endif
    pOLED->bbi2c.bWire = bWire;
    pOLED->bbi2c.iSDA = sda;
    pOLED->bbi2c.iSCL = scl;
    iResetPin = reset;
    // Disable SPI mode code
    iCSPin = iDCPin = -1;

    I2CInit(&pOLED->bbi2c, iSpeed); // on Linux, SDA = bus number, SCL = device address
     
    // find the device address if requested
    if (iAddr == -1 || iAddr == 0 || iAddr == 0xff) // find it
    {
        I2CTest(&pOLED->bbi2c, 0x3c);
        if (I2CTest(&pOLED->bbi2c, 0x3c))
        pOLED->oled_addr = 0x3c;
        else if (I2CTest(&pOLED->bbi2c, 0x3d))
        pOLED->oled_addr = 0x3d;
        else
        return rc; // no display found!
    }
    else
    {
        pOLED->oled_addr = iAddr;
        I2CTest(&pOLED->bbi2c, iAddr);
        if (!I2CTest(&pOLED->bbi2c, iAddr))
        return rc; // no display found
    }
    // Detect the display controller (SSD1306, SH1107 or SH1106)
    uint8_t u = 0;
    I2CReadRegister(&pOLED->bbi2c, pOLED->oled_addr, 0x00, &u, 1); // read the status register
    u &= 0x0f; // mask off power on/off bit

    rc = OLED_SH1106_3C;
    pOLED->oled_type = OLED_128x64; // needs to be treated a little differently


    // if (pOLED->oled_addr == 0x3d) rc++; // return the '3D' version of the type


    _I2CWrite(pOLED,(unsigned char *)oled64_initbuf, sizeof(oled64_initbuf));
 
    if (bInvert)
    {
        uc[0] = 0; // command
        uc[1] = 0xa7; // invert command
        _I2CWrite(pOLED,uc, 2);
    }
    if (bFlip) // rotate display 180
    {
        uc[0] = 0; // command
        uc[1] = 0xa0;
        _I2CWrite(pOLED,uc, 2);
        uc[1] = 0xc0;
        _I2CWrite(pOLED,uc, 2);
    }
    pOLED->oled_x = 128; // assume 128x64
    pOLED->oled_y = 64;
    
    return rc;
} /* oledInit() */


// Sends a command to turn on or off the OLED display
void oledPower(SSOLED *pOLED, uint8_t bOn)
{
    if (bOn) oledWriteCommand(pOLED, 0xaf); // turn on OLED
    else     oledWriteCommand(pOLED, 0xae); // turn off OLED
} 

// Send a single byte command to the OLED controller
static void oledWriteCommand(SSOLED *pOLED, unsigned char c)
{
    unsigned char buf[2];

    buf[0] = 0x00; // command introducer
    buf[1] = c;
    _I2CWrite(pOLED, buf, 2);
}

static void oledWriteCommand2(SSOLED *pOLED, unsigned char c, unsigned char d)
{
    unsigned char buf[3];

    buf[0] = 0x00;
    buf[1] = c;
    buf[2] = d;
    _I2CWrite(pOLED, buf, 3);
}


// Sets the brightness (0=off, 255=brightest)
void oledSetContrast(SSOLED *pOLED, unsigned char ucContrast)
{
    oledWriteCommand2(pOLED, 0x81, ucContrast);
}

// Scroll the internal buffer by 1 scanline (up/down)
// width is in pixels, lines is group of 8 rows
int oledScrollBuffer(SSOLED *pOLED, int iStartCol, int iEndCol, int iStartRow, int iEndRow, int bUp)
{
    uint8_t b, *s;
    int col, row;
    
    if (iStartCol < 0 || iStartCol > 127 || iEndCol < 0 || iEndCol > 127 || iStartCol > iEndCol) // invalid
        return -1;
    if (iStartRow < 0 || iStartRow > 7 || iEndRow < 0 || iEndRow > 7 || iStartRow > iEndRow)
        return -1;
    
    if (bUp)
    {
        for (row=iStartRow; row<=iEndRow; row++)
        {
            s = &pOLED->ucScreen[(row * 128) + iStartCol];
            for (col=iStartCol; col<=iEndCol; col++)
            {
                b = *s;
                b >>= 1; // scroll pixels 'up'
                if (row < iEndRow)
                    b |= (s[128] << 7); // capture pixel of row below, except for last row
                *s++ = b;
            } // for col
        } // for row
    } // up
    else // down
    {
        for (row=iEndRow; row>=iStartRow; row--)
        {
            s = &pOLED->ucScreen[(row * 128)+iStartCol];
            for (col=iStartCol; col<=iEndCol; col++)
            {
                b = *s;
                b <<= 1; // scroll down
                if (row > iStartRow)
                    b |= (s[-128] >> 7); // capture pixel of row above
                *s++ = b;
            } // for col
        } // for row
    }
    return 0;
} 

// Send commands to position the "cursor" (aka memory write address)
// to the given row and column
static void oledSetPosition(SSOLED *pOLED, int x, int y, int bRender)
{
    unsigned char buf[4];

    pOLED->iScreenOffset = (y*128)+x;
    if (!bRender) return; // don't send the commands to the OLED if we're not rendering the graphics now

    x += 2;

    buf[0] = 0x00; // command introducer
    buf[1] = 0xb0 | y; // set page to Y
    buf[2] = x & 0xf; // lower column address
    buf[3] = 0x10 | (x >> 4); // upper column addr
    _I2CWrite(pOLED, buf, 4);
}

// Write a block of pixel data to the OLED
// Length can be anything from 1 to 1024 (whole display)
static void oledWriteDataBlock(SSOLED *pOLED, unsigned char *ucBuf, int iLen, int bRender)
{
    unsigned char ucTemp[129];

    ucTemp[0] = 0x40; // data command
    // Copying the data has the benefit in SPI mode of not letting
    // the original data get overwritten by the SPI.transfer() function
    if (bRender)
    {
        memcpy(&ucTemp[1], ucBuf, iLen);
        _I2CWrite(pOLED, ucTemp, iLen+1);
    }
    // Keep a copy in local buffer
    if (pOLED->ucScreen)
    {
        memcpy(&pOLED->ucScreen[pOLED->iScreenOffset], ucBuf, iLen);
        pOLED->iScreenOffset += iLen;
        pOLED->iScreenOffset &= 1023; // we use a fixed stride of 128 no matter what the display size
    }
}

//
// Write a block of flash memory to the display
//
void oledWriteFlashBlock(SSOLED *pOLED, uint8_t *s, int iLen)
{
    int j;
    int iWidthMask = pOLED->oled_x -1;
    int iSizeMask = ((pOLED->oled_x * pOLED->oled_y)/8) - 1;
    int iWidthShift = (pOLED->oled_x == 128) ? 7:6; // assume 128 or 64 wide
    uint8_t ucTemp[128];

    while (((pOLED->iScreenOffset & iWidthMask) + iLen) >= pOLED->oled_x) // if it will hit the page end
    {
        j = pOLED->oled_x - (pOLED->iScreenOffset & iWidthMask); // amount we can write in one shot
        memcpy(ucTemp, s, j);
        oledWriteDataBlock(pOLED, ucTemp, j, 1);
        s += j;
        iLen -= j;
        pOLED->iScreenOffset = (pOLED->iScreenOffset + j) & iSizeMask;
        oledSetPosition(pOLED, pOLED->iScreenOffset & iWidthMask, (pOLED->iScreenOffset >> iWidthShift), 1);
    } // while it needs some help
    memcpy(ucTemp, s, iLen);
    oledWriteDataBlock(pOLED, ucTemp, iLen, 1);
    pOLED->iScreenOffset = (pOLED->iScreenOffset + iLen) & iSizeMask;
} /* oledWriteFlashBlock() */

//
// Write a repeating byte to the display
//
void oledRepeatByte(SSOLED *pOLED, uint8_t b, int iLen)
{
    int j;
    int iWidthMask = pOLED->oled_x -1;
    int iWidthShift = (pOLED->oled_x == 128) ? 7:6; // assume 128 or 64 pixels wide
    int iSizeMask = ((pOLED->oled_x * pOLED->oled_y)/8) -1;
    uint8_t ucTemp[128];

    memset(ucTemp, b, (iLen > 128) ? 128:iLen);
    while (((pOLED->iScreenOffset & iWidthMask) + iLen) >= pOLED->oled_x) // if it will hit the page end
    {
        j = pOLED->oled_x - (pOLED->iScreenOffset & iWidthMask); // amount we can write in one shot
        oledWriteDataBlock(pOLED, ucTemp, j, 1);
        iLen -= j;
        pOLED->iScreenOffset = (pOLED->iScreenOffset + j) & iSizeMask;
        oledSetPosition(pOLED, pOLED->iScreenOffset & iWidthMask, (pOLED->iScreenOffset >> iWidthShift), 1);
    } // while it needs some help
    oledWriteDataBlock(pOLED, ucTemp, iLen, 1);
    pOLED->iScreenOffset += iLen;
}



// Set (or clear) an individual pixel
// The local copy of the frame buffer is used to avoid
// reading data from the display controller
int oledSetPixel(SSOLED *pOLED, int x, int y, unsigned char ucColor, int bRender)
{
    int i;
    unsigned char uc, ucOld;

    i = ((y >> 3) * 128) + x;
    if (i < 0 || i > 1023) return -1; // off the screen
    oledSetPosition(pOLED, x, y>>3, bRender);

        uint8_t ucTemp[3];
        ucTemp[0] = 0x80; // one command
        ucTemp[1] = 0xE0; // read_modify_write
        ucTemp[2] = 0xC0; // one data
        _I2CWrite(pOLED, ucTemp, 3);

        // read a dummy byte followed by the data byte we want
        I2CRead(&pOLED->bbi2c, pOLED->oled_addr, ucTemp, 2);
        uc = ucOld = ucTemp[1]; // first byte is garbage 


    uc &= ~(0x1 << (y & 7));
    if (ucColor)
    {
        uc |= (0x1 << (y & 7));
    }
    if (uc != ucOld) // pixel changed
    {
        uint8_t ucTemp[4];
        ucTemp[0] = 0xc0; // one data
        ucTemp[1] = uc;   // actual data
        ucTemp[2] = 0x80; // one command
        ucTemp[3] = 0xEE; // end read_modify_write operation
        _I2CWrite(pOLED, ucTemp, 4);
    }
    return 0;
}


int oledPSET(SSOLED *pOLED, int x, int y, unsigned char ucColor)
{
    int i;
    unsigned char uc;

    i = ((y >> 3) * 128) + x;
    if (i < 0 || i > 1023) return -1; // off the screen
    oledSetPosition(pOLED, x, y>>3, 1);

    uint8_t ucTemp[4];
    ucTemp[0] = 0x80; // one command
    ucTemp[1] = 0xE0; // read_modify_write
    ucTemp[2] = 0xC0; // one data
    _I2CWrite(pOLED, ucTemp, 3);

    // read a dummy byte followed by the data byte we want
    I2CRead(&pOLED->bbi2c, pOLED->oled_addr, ucTemp, 2);
    uc = ucTemp[1]; // first byte is garbage 

    uc &= ~(0x1 << (y & 7));
    uc |= (0x1 << (y & 7));

    ucTemp[0] = 0xc0; // one data
    ucTemp[1] = uc;   // actual data
    ucTemp[2] = 0x80; // one command
    ucTemp[3] = 0xEE; // end read_modify_write operation
    _I2CWrite(pOLED, ucTemp, 4);

    return 0;
}



// Set the current cursor position
// The column represents the pixel column (0-127)
// The row represents the text row (0-7)
void oledSetCursor(SSOLED *pOLED, int x, int y)
{
    pOLED->iCursorX = x;
    pOLED->iCursorY = y;
}



// Dump a screen's worth of data directly to the display
// Try to speed it up by comparing the new bytes with the existing buffer
void oledDumpBuffer(SSOLED *pOLED, uint8_t *pBuffer)
{
    int x, y;
    int iLines, iCols;
    uint8_t bNeedPos;
    uint8_t *pSrc = pOLED->ucScreen;
        
    if (pBuffer == NULL) // dump the internal buffer if none is given
        pBuffer = pOLED->ucScreen;
    if (pBuffer == NULL) return; // no backbuffer and no provided buffer
  
    iLines = pOLED->oled_y >> 3;
    iCols = pOLED->oled_x >> 4;
    for (y=0; y<iLines; y++)
    {
        bNeedPos = 1; // start of a new line means we need to set the position too
        for (x=0; x<iCols; x++) // wiring library has a 32-byte buffer, so send 16 bytes so that the data prefix (0x40) can fit
        {
            if (pOLED->ucScreen == NULL || pBuffer == pSrc || memcmp(pSrc, pBuffer, 16) != 0) // doesn't match, need to send it
            {
                if (bNeedPos) // need to reposition output cursor?
                {
                    bNeedPos = 0;
                    oledSetPosition(pOLED, x*16, y, 1);
                }
                oledWriteDataBlock(pOLED, pBuffer, 16, 1);
            }
            else
            {
                bNeedPos = 1; // we're skipping a block, so next time will need to set the new position
            }
            pSrc += 16;
            pBuffer += 16;
        } // for x
        pSrc += (128 - pOLED->oled_x); // for narrow displays, skip to the next line
        pBuffer += (128 - pOLED->oled_x);
    } // for y
}


// Fill the frame buffer with a byte pattern
// e.g. all off (0x00) or all on (0xff)
void oledFill(SSOLED *pOLED, unsigned char ucData, int bRender)
{
    uint8_t x, y;
    uint8_t iLines, iCols;
    unsigned char temp[16];

    iLines = pOLED->oled_y >> 3;
    iCols = pOLED->oled_x >> 4;
    memset(temp, ucData, 16);
    pOLED->iCursorX = pOLED->iCursorY = 0;
 
    for (y=0; y<iLines; y++)
    {
        oledSetPosition(pOLED, 0,y, bRender); // set to (0,Y)
        for (x=0; x<iCols; x++) // wiring library has a 32-byte buffer, so send 16 bytes so that the data prefix (0x40) can fit
        {
            oledWriteDataBlock(pOLED, temp, 16, bRender);
        } // for x
    } // for y
    if (pOLED->ucScreen)  memset(pOLED->ucScreen, ucData, (pOLED->oled_x * pOLED->oled_y)/8);
} /* oledFill() */

//
// Provide or revoke a back buffer for your OLED graphics
// This allows you to manage the RAM used by ss_oled on tiny
// embedded platforms like the ATmega series
// Pass NULL to revoke the buffer. Make sure you provide a buffer
// large enough for your display (e.g. 128x64 needs 1K - 1024 bytes)
//
void oledSetBackBuffer(SSOLED *pOLED, uint8_t *pBuffer)
{
    pOLED->ucScreen = pBuffer;
} /* oledSetBackBuffer() */



// Draw a string of normal (8x8), small (6x8) or large (16x32) characters
// At the given col+row
int oledWriteString(SSOLED *pOLED, int iScroll, int x, int y, char *szMsg, int iSize, int bInvert, int bRender)
{
    int i, iFontOff, iLen, iFontSkip;
    unsigned char c, *s, ucTemp[40];

    if (x == -1 || y == -1) // use the cursor position
    {
        x = pOLED->iCursorX; y = pOLED->iCursorY;
    }
    else
    {
        pOLED->iCursorX = x; pOLED->iCursorY = y; // set the new cursor position
    }
    if (pOLED->iCursorX >= pOLED->oled_x || pOLED->iCursorY >= pOLED->oled_y / 8)
       return -1; // can't draw off the display

    oledSetPosition(pOLED, pOLED->iCursorX, pOLED->iCursorY, bRender);
    if (iSize == FONT_8x8) // 8x8 font
    {
        i = 0;
        iFontSkip = iScroll & 7; // number of columns to initially skip
        while (pOLED->iCursorX < pOLED->oled_x && szMsg[i] != 0 && pOLED->iCursorY < pOLED->oled_y / 8)
        {
            if (iScroll < 8) // only display visible characters
            {
                c = (unsigned char)szMsg[i];
                iFontOff = (int)(c-32) * 7;
                // we can't directly use the pointer to FLASH memory, so copy to a local buffer
                ucTemp[0] = 0;
                memcpy(&ucTemp[1], &gtFont[iFontOff], 7);
                if (bInvert) InvertBytes(ucTemp, 8);
                // oledCachedWrite(ucTemp, 8);
                iLen = 8 - iFontSkip;
                if (pOLED->iCursorX + iLen > pOLED->oled_x) // clip right edge
                    iLen = pOLED->oled_x - pOLED->iCursorX;
                oledWriteDataBlock(pOLED, &ucTemp[iFontSkip], iLen, bRender); // write character pattern
                pOLED->iCursorX += iLen;
                if (pOLED->iCursorX >= pOLED->oled_x-7 && pOLED->oled_wrap) // word wrap enabled?
                {
                    pOLED->iCursorX = 0; // start at the beginning of the next line
                    pOLED->iCursorY++;
                    oledSetPosition(pOLED, pOLED->iCursorX, pOLED->iCursorY, bRender);
                }
                iFontSkip = 0;
            }
            iScroll -= 8;
            i++;
        } // while
        return 0;
    }
    return -1; // invalid size
}



int oledWriteStringV(SSOLED *pOLED, int iScroll, int x, int y, char *szMsg, int iSize, int bInvert, int bRender)
{
    int i, iFontOff, iLen, iFontSkip;
    unsigned char c, *s, ucTemp[40];

    if (x == -1 || y == -1) // use the cursor position
    {
        x = pOLED->iCursorX; y = pOLED->iCursorY;
    }
    else
    {
        pOLED->iCursorX = x; pOLED->iCursorY = y; // set the new cursor position
    }
    if (pOLED->iCursorX >= pOLED->oled_x || pOLED->iCursorY >= pOLED->oled_y / 8)
       return -1; // can't draw off the display

    oledSetPosition(pOLED, pOLED->iCursorX, pOLED->iCursorY, bRender);
    if (iSize == FONT_8x8) // 8x8 font
    {
        i = 0;
        iFontSkip = iScroll & 7; // number of columns to initially skip
        while (pOLED->iCursorX < pOLED->oled_x && szMsg[i] != 0 && pOLED->iCursorY < pOLED->oled_y / 8)
        {
            if (iScroll < 8) // only display visible characters
            {
                c = (unsigned char)szMsg[i];
                iFontOff = (int)(c-32) * 7;
                // we can't directly use the pointer to FLASH memory, so copy to a local buffer
                ucTemp[0] = 0;
                memcpy(&ucTemp[1], &gtFont[iFontOff], 7);
                if (bInvert) InvertBytes(ucTemp, 8);
                // oledCachedWrite(ucTemp, 8);
                iLen = 8 - iFontSkip;
                if (pOLED->iCursorX + iLen > pOLED->oled_x) // clip right edge
                    iLen = pOLED->oled_x - pOLED->iCursorX;
                oledWriteDataBlock(pOLED, &ucTemp[iFontSkip], iLen, bRender); // write character pattern
                pOLED->iCursorY ++;
                oledSetPosition(pOLED, pOLED->iCursorX, pOLED->iCursorY, bRender);
                iFontSkip = 0;
            }
            iScroll -= 8;
            i++;
        } // while
        return 0;
    }
    return -1; // invalid size
}


int oledWriteStringF(SSOLED *pOLED, int iScroll, int x, int y, uint8_t symbol, char* font, int bInvert)
{
    pOLED->iCursorX = x; pOLED->iCursorY = y; // set the new cursor position
    oledSetPosition(pOLED, pOLED->iCursorX, pOLED->iCursorY, 1);
    // we can't directly use the pointer to FLASH memory, so copy to a local buffer
    unsigned char ucTemp = 0;
    memcpy(&ucTemp, &font[symbol], 7);

    oledWriteDataBlock(pOLED, &ucTemp, 7, 1); // write character pattern
    return 0;
}