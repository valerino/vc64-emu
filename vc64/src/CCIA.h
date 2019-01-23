//
// Created by valerino on 18/01/19.
//

#ifndef VC64_EMU_CCIA_H
#define VC64_EMU_CCIA_H
#include <CMOS65xx.h>

/**
 * emulates the CIA 6526 (there are 2 inside a c64) which handles most of the I/O. this is the base class for the CIA
 * chips
 */
class CCIA {
public:
    CCIA(CMOS65xx* cpu);
    ~CCIA();

protected:
    CMOS65xx* _cpu;
};


#endif //VC64_EMU_CCIA_H
