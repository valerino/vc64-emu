//
// Created by valerino on 19/01/19.
//

#ifndef VC64_EMU_CVICII_H
#define VC64_EMU_CVICII_H

#include <CMOS65xx.h>
#include "CCIA2.h"

#ifndef NDEBUG
// debug-only flag
//#define DEBUG_VIC
#endif

/**
 * screen modes
 * https://www.c64-wiki.com/wiki/Standard_Bitmap_Mode
 */
#define VIC_SCREEN_MODE_CHARACTER_STANDARD 0
#define VIC_SCREEN_MODE_CHARACTER_MULTICOLOR 1
#define VIC_SCREEN_MODE_BITMAP_STANDARD 2
#define VIC_SCREEN_MODE_BITMAP_MULTICOLOR 3
#define VIC_SCREEN_MODE_EXTENDED_BACKGROUND_COLOR 4

// sizes comprensive of vblank and hblank
// from http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt
#define VIC_PAL_SCREEN_W 403
#define VIC_PAL_SCREEN_H 284
#define VIC_PAL_SCANLINES 312
#define VIC_PAL_HZ 985248
#define VIC_PAL_CYCLES_PER_LINE 63
#define VIC_PAL_CYCLES_PER_BADLINE 23
#define VIC_PAL_FIRST_VISIBLE_LINE 14
#define VIC_PAL_LAST_VISIBLE_LINE 298

#define VIC_CHAR_MODE_COLUMNS 40

// display window is intended the 'blue' window surrounded by the border, when
// c64 boots :)
#define VIC_PAL_FIRST_DISPLAYWINDOW_COLUMN 42
#define VIC_PAL_FIRST_DISPLAYWINDOW_LINE 56
#define VIC_PAL_LAST_DISPLAYWINDOW_LINE 256

/**
 * registers
 * https://www.c64-wiki.com/wiki/Page_208-211
 */
#define VIC_REGISTERS_START 0xd000
#define VIC_REGISTERS_END 0xd3ff

#define VIC_REG_CR1 0xd011
#define VIC_REG_RASTERCOUNTER 0xd012
#define VIC_REG_CR2 0xd016

// https://www.c64-wiki.com/wiki/53272
#define VIC_REG_BASE_ADDR 0xd018

#define VIC_REG_INTERRUPT_LATCH 0xd019
#define VIC_REG_IRQ_ENABLED 0xd01a
#define VIC_REG_BORDER_COLOR 0xd020
#define VIC_REG_BG_COLOR_0 0xd021
#define VIC_REG_BG_COLOR_1 0xd022
#define VIC_REG_BG_COLOR_2 0xd023
#define VIC_REG_BG_COLOR_3 0xd024

/**
 * @brief defines a color in the c64 palettte
 */
typedef struct _rgbStruct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgbStruct;

/**
 * @brief callback to set the pixel at the specific position
 */
typedef void (*BlitCallback)(void *thisPtr, rgbStruct *rgb, int pos);

/**
 * emulates the vic-ii 6569 chip
 */
class CVICII {
    friend class CDisplay;

  public:
    /**
     * constructor
     * @param cpu the cpu
     * @param cia2 the CIA-2 chip
     */
    CVICII(CMOS65xx *cpu, CCIA2 *cia2);

    /**
     * update the internal state
     * @param current cycle count
     * @return additional cycles used
     */
    int update(long cycleCount);

    /**
     * read from chip memory
     * @param address
     * @param bt
     */
    void read(uint16_t address, uint8_t *bt);

    /**
     * write to chip memory
     * @param address
     * @param bt
     */
    void write(uint16_t address, uint8_t bt);

  protected:
    /**
     * set blitting callback
     * @param display opaque pointer to the display handler
     * @param cb callback to be called when blitting a pixel
     */
    void setBlitCallback(void *display, BlitCallback cb);

  private:
    CMOS65xx *_cpu = nullptr;
    void *_displayObj = nullptr;
    int _prevCycles = 0;
    uint16_t _rasterIrqLine = 0;
    int _scrollX = 0;
    int _scrollY = 0;
    uint8_t _regM0X = 0;
    uint8_t _regM0Y = 0;
    uint8_t _regM1X = 0;
    uint8_t _regM1Y = 0;
    uint8_t _regM2X = 0;
    uint8_t _regM2Y = 0;
    uint8_t _regM3X = 0;
    uint8_t _regM3Y = 0;
    uint8_t _regM4X = 0;
    uint8_t _regM4Y = 0;
    uint8_t _regM5X = 0;
    uint8_t _regM5Y = 0;
    uint8_t _regM6X = 0;
    uint8_t _regM6Y = 0;
    uint8_t _regM7X = 0;
    uint8_t _regM7Y = 0;
    uint8_t _regCR1 = 0;
    uint16_t _regRASTER = 0;
    uint8_t _regLPX = 0;
    uint8_t _regLPY = 0;
    uint8_t _regSpriteEnabled = 0;
    uint8_t _regCR2 = 0;
    uint8_t _regSpriteYExpansion = 0;
    uint8_t _regMemoryPointers = 0;
    uint8_t _regInterrupt = 0;
    uint8_t _regInterruptEnabled = 0;
    uint8_t _regSpriteDataPriority = 0;
    uint8_t _regSpriteMultiColor = 0;
    uint8_t _regSpriteXExpansion = 0;
    uint8_t _regSpriteSpriteCollision = 0;
    uint8_t _regSpriteDataCollision = 0;
    uint8_t _regBorderColor = 0;
    uint8_t _regBackgroundColor0 = 0;
    uint8_t _regBackgroundColor1 = 0;
    uint8_t _regBackgroundColor2 = 0;
    uint8_t _regBackgroundColor3 = 0;
    // uint8_t _regBackgroundColors[4] = {0};
    uint8_t _regSpriteMulticolor0 = 0;
    uint8_t _regSpriteMulticolor1 = 0;
    uint8_t _regSpriteColor0 = 0;
    uint8_t _regSpriteColor1 = 0;
    uint8_t _regSpriteColor2 = 0;
    uint8_t _regSpriteColor3 = 0;
    uint8_t _regSpriteColor4 = 0;
    uint8_t _regSpriteColor5 = 0;
    uint8_t _regSpriteColor6 = 0;
    uint8_t _regSpriteColor7 = 0;

    BlitCallback _cb = nullptr;
    CCIA2 *_cia2 = nullptr;
    rgbStruct _palette[16] = {0};

    uint16_t checkShadowAddress(uint16_t address);
    bool checkUnusedAddress(int type, uint16_t address, uint8_t *bt);
    void getCharacterModeScreenAddress(uint16_t *screenCharacterRamAddress,
                                       uint16_t *charsetAddress, int *bank);
    void getBitmapModeScreenAddress(uint16_t *colorInfoAddress,
                                    uint16_t *bitmapAddress);
    void drawBorder(int rasterLine);
    void drawCharacterMode(int rasterLine);
    bool isCharacterMode();
    void blit(int x, int y, rgbStruct *rgb);
    void initPalette();

    /**
     * @brief get the screen mode and return one of the VIC_SCREEN_MODE_* values
     * @return int
     */
    int getScreenMode();
};

#endif // VC64_EMU_CVICII_H
