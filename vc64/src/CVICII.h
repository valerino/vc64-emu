//
// Created by valerino on 19/01/19.
//

#ifndef VC64_EMU_CVICII_H
#define VC64_EMU_CVICII_H

#include <CMOS65xx.h>
#include "CCIA2.h"

#define VIC_PAL_HZ  50.124

#define VIC_PAL_SCREEN_W 403
#define VIC_PAL_SCREEN_H 284
#define VIC_PAL_SCANLINES_PER_VBLANK 312

#define LINE_LAST_VISIBLE 298
#define LINE_FIRST_VISIBLE 14

#define CYCLES_PER_LINE 19656 / 312

/**
 * registers
 * https://www.c64-wiki.com/wiki/Page_208-211
 */
#define VIC_REGISTERS_START 0xd000
#define VIC_REGISTERS_END   0xd3ff

#define VIC_REG_CR1 0xd011
#define VIC_REG_RASTERCOUNTER 0xd012
#define VIC_REG_CR2 0xd016
#define VIC_REG_BASE_ADDR  0xd018       // https://www.c64-wiki.com/wiki/53272
#define VIC_REG_BORDER_COLOR 0xd020
#define VIC_REG_BG_COLOR_0 0xd021
#define VIC_REG_BG_COLOR_1 0xd022
#define VIC_REG_BG_COLOR_2 0xd023
#define VIC_REG_BG_COLOR_3 0xd024

/**
 * emulates the vic-ii 6569 chip
 */
class CVICII {
public:
    /**
     * constructor
     * @param cpu the cpu
     * @param cia2 the CIA-2 chip
     */
    CVICII(CMOS65xx* cpu, CCIA2* cia2);

    ~CVICII();

    /**
     * run the vic
     * @param current cycle count
     * @return number of cycles occupied
     */
    int run(int cycleCount);

    /**
     * get the connected cpu
     * @return the cpu instance
     */
    CMOS65xx* cpu();

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

    /**
     * update the screen
     * @param frameBuffer
     */
    void updateScreen(uint32_t* frameBuffer);

private:
    CMOS65xx* _cpu;
    uint16_t _rasterCounter;
    typedef struct _rgbStruct {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    } rgbStruct;
    rgbStruct _palette[16];
    CCIA2* _cia2;
    void updateRasterCounter(uint16_t cnt);
    uint16_t check_shadow_address(uint16_t address);
    bool check_unused_address(int type, uint16_t address, uint8_t *bt);
    void updateScreenLoRes(uint32_t* frameBuffer);
    void getTextModeScreenAddress(uint16_t* screenCharacterRamAddress, uint16_t* charsetAddress);
    void getBitmapModeScreenAddress(uint16_t* colorInfoAddress, uint16_t* bitmapAddress);
};


#endif //VC64_EMU_CVICII_H
