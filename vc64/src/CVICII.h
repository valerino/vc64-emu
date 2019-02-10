//
// Created by valerino on 19/01/19.
//

#ifndef VC64_EMU_CVICII_H
#define VC64_EMU_CVICII_H

#include <CMOS65xx.h>
#include "CCIA2.h"
#include <CSDLUtils.h>

#define VIC_PAL_HZ  50.124

#define VIC_PAL_SCREEN_W 403
#define VIC_PAL_SCREEN_H 284
#define VIC_PAL_SCANLINES_PER_VBLANK 312

#define VIC_FIRST_VISIBLE_LINE 14
#define VIC_LAST_VISIBLE_LINE 298

// 63 cycles per line
#define VIC_CYCLES_PER_LINE 19656 / 312

// 23 cycles per badline
#define VIC_CYCLES_PER_BADLINE 23

/**
 * registers
 * https://www.c64-wiki.com/wiki/Page_208-211
 */
#define VIC_REGISTERS_START 0xd000
#define VIC_REGISTERS_END   0xd3ff

#define VIC_REG_CR1 0xd011
#define VIC_REG_RASTERCOUNTER 0xd012
#define VIC_REG_CR2 0xd016

// https://www.c64-wiki.com/wiki/53272
#define VIC_REG_BASE_ADDR  0xd018

#define VIC_REG_INTERRUPT  0xd019
#define VIC_REG_IRQ_ENABLED 0xd01a
#define VIC_REG_BORDER_COLOR 0xd020
#define VIC_REG_BG_COLOR_0 0xd021
#define VIC_REG_BG_COLOR_1 0xd022
#define VIC_REG_BG_COLOR_2 0xd023
#define VIC_REG_BG_COLOR_3 0xd024

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
    int _borderHSize;
    int _borderVSize;
    int _scrollX;
    int _scrollY;
    int _displayH;
    int _displayW;

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
    void updateScreen();
    uint16_t checkShadowAddress(uint16_t address);
    bool checkUnusedAddress(int type, uint16_t address, uint8_t *bt);
    void updateScreenCharacterMode(uint32_t *frameBuffer);
    void getTextModeScreenAddress(uint16_t* screenCharacterRamAddress, uint16_t* charsetAddress);
    void getBitmapModeScreenAddress(uint16_t* colorInfoAddress, uint16_t* bitmapAddress);
    void drawBorderRow(int row);
};


#endif //VC64_EMU_CVICII_H
