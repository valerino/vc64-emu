//
// Created by valerino on 23/01/19.
//
#pragma once
#include <CMOS65xx.h>

/**
 * implements the SID 6581 audio chip
 */
class CSID {
  public:
    CSID(CMOS65xx *cpu);
    ~CSID();

    /**
     * update the internal state
     * @param current cycle count
     * @return additional cycles used
     */
    int update(int cycleCount);

  private:
    CMOS65xx *_cpu;
};
