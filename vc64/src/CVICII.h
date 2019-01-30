//
// Created by valerino on 19/01/19.
//

#ifndef VC64_EMU_CVICII_H
#define VC64_EMU_CVICII_H

#include <CMOS65xx.h>

#define VIC_PAL_HZ  50.124

#define VIC_PAL_SCREEN_W 403
#define VIC_PAL_SCREEN_H 284
#define VIC_PAL_COLUMNS 504
#define VIC_PAL_LINES 312

#define LINE_LAST_VISIBLE 298
#define LINE_FIRST_VISIBLE 14

#define CYCLES_PER_LINE 19656 / 312
#define CYCLES_BADLINE 23

#define VIC_REG_CONTROL1 0x11
#define VIC_REG_RASTER_COUNT 0x12
#define VIC_REG_INT_MASK 0x1a

// registers: https://www.c64-wiki.com/wiki/Page_208-211
#define VIC_REGISTERS_START 0xd000
#define VIC_REGISTERS_END   0xd3ff

#define RASTER_COUNT ((rawVideoMem[VIC_REG_CONTROL1] & 0x80) << 1) | rawVideoMem[VIC_REG_RASTER_COUNT]

/**
 * emulates the vic-ii 6569 chip
 */
class CVICII {
public:
    /**
     * constructor
     * @param cpu the cpu
     */
    CVICII(CMOS65xx* cpu);

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
     * read from vic memory
     * @param address
     * @param bt
     */
    void read(uint16_t address, uint8_t* bt);

    /**
     * write to vic memory
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
    uint8_t _cr1;
    void setRasterCounter(uint16_t cnt);
    uint16_t check_shadow_address(uint16_t address, bool* hit);
    bool check_unused_address(int type, uint16_t address, uint8_t *bt);
};


#endif //VC64_EMU_CVICII_H
