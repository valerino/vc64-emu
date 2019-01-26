//
// Created by valerino on 19/01/19.
//

#ifndef VC64_EMU_CCIA2_H
#define VC64_EMU_CCIA2_H

#include "CCIA.h"
/**
 * implements the 2nd CIA 6526, which controls the serial bus, rs232, vic memory and NMI
 */
class CCIA2: public CCIA {
public:
    CCIA2(CMOS65xx* cpu);
    virtual ~CCIA2();
    void run();

private:

};

#endif //VC64_EMU_CCIA2_H
