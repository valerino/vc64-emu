//
// Created by valerino on 19/01/19.
//

#ifndef VC64_EMU_CCIA1_H
#define VC64_EMU_CCIA1_H

#include "CCIA.h"

/**
 * implements the 1st CIA 6526, which controls keyboard, joystick, paddles, datassette and IRQ
 */
class CCIA1: public CCIA {
public:
    CCIA1(CMOS65xx* cpu);
    virtual ~CCIA1();
private:

};


#endif //VC64_EMU_CCIA1_H
