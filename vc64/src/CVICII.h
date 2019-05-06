//
// Created by valerino on 19/01/19.
//

#ifndef VC64_EMU_CVICII_H
#define VC64_EMU_CVICII_H

#include <CMOS65xx.h>
#include "CCIA2.h"
#include <CSDLUtils.h>

#define VIC_RES_W 320
#define VIC_RES_H 200

// refresh rate
#define VIC_PAL_HZ  50.124

// sizes comprensive of vblank and hblank
// from http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt
#define VIC_PAL_SCREEN_W 403
#define VIC_PAL_SCREEN_H 284
#define VIC_PAL_SCANLINES 312
#define VIC_PAL_VISIBLE_LINES 284
#define VIC_PAL_CYCLES_PER_LINE 63
#define VIC_PAL_FIRST_VISIBLE_LINE 14
#define VIC_PAL_LAST_VISIBLE_LINE 298

#define VIC_CHAR_MODE_COLUMNS 40

// display window is intended the 'blue' window surrounded by the border, when c64 boots :)
#define VIC_PAL_FIRST_DISPLAYWINDOW_COLUMN 42
#define VIC_PAL_FIRST_DISPLAYWINDOW_LINE 56
#define VIC_PAL_LAST_DISPLAYWINDOW_LINE 256

// 23 cycles per badline
#define VIC_PAL_CYCLES_PER_BADLINE 23

/**
 * registers
 * https://www.c64-wiki.com/wiki/Page_208-211
 */
#define VIC_REGISTERS_START 0xd000
#define VIC_REGISTERS_END   0xd3ff

/**
 * emulates the vic-ii 6569 chip
 */
class CVICII {
    /**
     * can access private members
     */
    friend class CDisplay;

public:
    /**
     * constructor
     * @param cpu the cpu
     * @param cia2 the CIA-2 chip
     */
    CVICII(CMOS65xx* cpu, CCIA2* cia2);

    ~CVICII();

    /**
     * update the internal state
     * @param current cycle count
     * @return additional cycles used
     */
    int update(int cycleCount);

    /**
     * read from chip memory
     * @param address
     * @param bt
     */
    void read(uint16_t address, uint8_t* bt);

    /**
     * write to chip memory
     * @param address
     * @param bt
     */
     void write(uint16_t address, uint8_t bt);

private:
    CMOS65xx* _cpu;
    uint16_t _rasterCounter;
    uint16_t _rasterIrqLine;
    int _firstY;
    int _lastY;
    int _firstX;
    int _lastX;
    int _scrollX;
    int _scrollY;
    uint8_t _regBackgroundColors[4];
    uint8_t _regBorderColor;
    uint8_t _regCr1;
    uint8_t _regCr2;
    uint8_t _regInterruptLatch;
    uint8_t _regInterruptEnabled;
    void setSdlCtx(SDLDisplayCtx* ctx, uint32_t* frameBuffer);

    /**
     * for the c64 palettte
     */
    typedef struct _rgbStruct {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    } rgbStruct;
    rgbStruct _palette[16];
    uint32_t* _fb;
    SDLDisplayCtx* _sdlCtx;
    CCIA2* _cia2;
    void updateRasterCounter();
    uint16_t checkShadowAddress(uint16_t address);
    bool checkUnusedAddress(int type, uint16_t address, uint8_t *bt);
    void getTextModeScreenAddress(uint16_t* screenCharacterRamAddress, uint16_t* charsetAddress, int* bank);
    void getBitmapModeScreenAddress(uint16_t* colorInfoAddress, uint16_t* bitmapAddress);
    void drawBorder(int rasterLine);
    void drawCharacterMode(int rasterLine);
    bool isCharacterMode();
    void blit(int x, int y, CVICII::rgbStruct *rgb);
};


#endif //VC64_EMU_CVICII_H
