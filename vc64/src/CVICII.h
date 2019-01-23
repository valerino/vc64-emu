//
// Created by valerino on 19/01/19.
//

#ifndef VC64_EMU_CVICII_H
#define VC64_EMU_CVICII_H

#include <CMOS65xx.h>

/**
 * emulates the vic-ii 6569 chip
 */
class CVICII {
public:
    CVICII(CMOS65xx* cpu);
    ~CVICII();
private:
    CMOS65xx* _cpu;
};


#endif //VC64_EMU_CVICII_H
