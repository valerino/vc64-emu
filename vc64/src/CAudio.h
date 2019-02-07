//
// Created by valerino on 23/01/19.
//

#ifndef VC64_EMU_CAUDIO_H
#define VC64_EMU_CAUDIO_H

#include "CSID.h"

class CAudio {
public:
    CAudio(CSID* sid);
    ~CAudio();
    int update();
private:
    CSID* _sid;
};


#endif //VC64_EMU_CAUDIO_H
