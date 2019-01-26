//
// Created by valerino on 19/01/19.
//

#ifndef VC64_EMU_CVICII_H
#define VC64_EMU_CVICII_H

#include <CMOS65xx.h>

/**
 * vic address range
 */
#define VIC_ADDRESS_LO 0xd000
#define VIC_ADDRESS_HI 0xd3ff
#define VIC_ADDRESS(_x_) (_x_ >= VIC_ADDRESS_LO && _x_ <= VIC_ADDRESS_HI)

#define VIC_PAL_SCREEN_W 403
#define VIC_PAL_SCREEN_H 284

/**
 * registers addresses
 */
#define VIC_REG_CONTROL1 0xd011
#define VIC_REG_RASTER_COUNT 0xd012
#define VIC_REG_INT_MASK 0xd01a

#define VIC_PAL_LINES 312

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

    void doIo(int type, uint16_t address, uint8_t bt);
private:
    CMOS65xx* _cpu;
    int _rasterIrqLine;
};


#endif //VC64_EMU_CVICII_H
