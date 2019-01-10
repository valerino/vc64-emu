//
// Created by valerino on 06/01/19.
//

#ifndef CMOS65XX_H
#define CMOS65XX_H

/**
 * implement MOS 6502/6510 CPU emulator
 */
#include <IMemory.h>

class CMOS65xx {
public:

    /**
     * services an irq stored in the IRQ vector
     */
    void irq();

    /**
     * services a nonmaskable interrupt stored in the NMI vector
     */
    void nmi();

    /**
     * run the cpu for the specified number of cycles
     * @param cycles number of cycles
     */
    void run(int cycles);

    /**
     * resets the cpu to the initial state
     * @return
     */
    void reset();

    /**
     * constructor
     * @param mem IMemory implementation (emulated memory)
     */
    CMOS65xx(IMemory* mem);
};


#endif //CMOS65XX_H
