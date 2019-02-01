//
// Created by valerino on 23/01/19.
//

#ifndef VC64_EMU_CINPUT_H
#define VC64_EMU_CINPUT_H

#include "CCIA1.h"

class CInput {
public:
    CInput(CCIA1* cia1);
    ~CInput();

    void update(uint8_t* keys);
private:
    CCIA1* _cia1;
};


#endif //VC64_EMU_CINPUT_H
