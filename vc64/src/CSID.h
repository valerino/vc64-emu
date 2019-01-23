//
// Created by valerino on 23/01/19.
//

#ifndef VC64_EMU_CSID_H
#define VC64_EMU_CSID_H

#include <CMOS65xx.h>

/**
 * implements the SID 6581 audio chip
 */
class CSID {
public:
    CSID(CMOS65xx* cpu);
    ~CSID();

private:
    CMOS65xx* _cpu;
};


#endif //VC64_EMU_CSID_H
