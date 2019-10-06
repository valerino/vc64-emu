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
#define VIC_RESOLUTION_X 320
#define VIC_RESOLUTION_Y 200
#define VIC_PAL_SCANLINES 312
#define VIC_PAL_HZ 985248
#define VIC_PAL_CYCLES_PER_LINE 63
#define VIC_PAL_CYCLES_PER_BADLINE 23
#define VIC_PAL_FIRST_VISIBLE_LINE 15 // 14
#define VIC_PAL_LAST_VISIBLE_LINE 298 // 300 // 298

/**
 * registers
 * https://www.c64-wiki.com/wiki/Page_208-211
 */
#define VIC_REGISTERS_START 0xd000
#define VIC_REGISTERS_END 0xd3ff

/**
 * @brief defines a color in the c64 palettte
 */
typedef struct _rgbStruct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} RgbStruct;

/**
 * @brief defines a rectangle
 */
typedef struct _rect {
    int firstX;
    int firstY;
    int lastX;
    int lastY;
    int vBorderSize;
    int hBorderSize;
} Rect;

/**
 * @brief callback to set the pixel at the specific position
 */
typedef void (*BlitCallback)(void *thisPtr, RgbStruct *rgb, int pos);

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
    bool _CSEL = false;
    bool _RSEL = false;
    bool _DEN = false;
    uint8_t _regM[8 * 2] = {0}; // 8 hardware sprites coordinates registers X/Y
                                // (M0X,M0Y ... M7X,M7Y)
    uint8_t _regMSBX = 0;
    uint8_t _regCR1 = 0;
    uint16_t _regRASTER = 0;
    uint8_t _regLP[2] = {0};
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
    uint8_t _regBC[4] = {0}; // 4 background colors (registers B0C ... B3C)
    uint8_t _regMM[2] = {0}; // sprite multicolor (registers MM0-MM1)
    uint8_t
        regMC[8]; // sprite colors for hw sprites 0..7 (registers M0C ... M7C)
    BlitCallback _cb = nullptr;
    CCIA2 *_cia2 = nullptr;
    RgbStruct _palette[16] = {0};

    uint16_t checkShadowAddress(uint16_t address);

    bool isSpriteEnabled(int idx);
    bool isSpriteMulticolor(int idx);
    bool isSpriteYExpanded(int idx);
    bool isSpriteXExpanded(int idx);

    void setSpriteCoordinate(int idx, uint8_t val);
    uint8_t getSpriteCoordinate(int idx);

    void setSpriteColor(int idx, uint8_t val);
    uint8_t getSpriteColor(int idx);

    void setSpriteMulticolor(int idx, uint8_t val);
    uint8_t getSpriteMulticolor(int idx);

    uint8_t getBackgoundColor(int idx);
    void setBackgoundColor(int idx, uint8_t val);

    bool checkUnusedAddress(int type, uint16_t address, uint8_t *bt);
    void getScreenAddress(uint16_t *screenAddress,
                          uint16_t *charsetAddress = nullptr,
                          int *bank = nullptr);
    void getBitmapModeScreenAddress(uint16_t *colorInfoAddress,
                                    uint16_t *bitmapAddress);

    uint16_t getSpriteXCoordinate(int idx);
    uint8_t getSpriteYCoordinate(int idx);

    void drawBorder(int rasterLine);
    void drawCharacterMode(int rasterLine);
    void drawSprites(int rasterLine);
    void blit(int x, int y, RgbStruct *rgb);
    void initPalette();
    int getScreenMode();
    void drawSprite(int rasterLine, int idx, int x, int row);
    void drawSpriteMulticolor(int rasterLine, int idx, int x, int row);
    bool isSpriteDrawingOnBorder(int x, int y);
    uint16_t getSpriteDataAddress(int idx);
    void getDisplayWindowLimits(Rect *limits);
    void getScreenLimits(Rect *limits);
    void drawStandardCharacterMode();
    void drawMulticolorCharacterMode();
    void drawBorder();
    int getCurrentRasterLine();
    void setCurrentRasterLine(int line);
};

#endif // VC64_EMU_CVICII_H
