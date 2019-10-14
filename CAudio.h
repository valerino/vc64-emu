//
// Created by valerino on 23/01/19.
//
#pragma once

#include "CSID.h"

class CAudio {
  public:
    CAudio(CSID *sid);
    ~CAudio();
    int update();

  private:
    CSID *_sid = nullptr;
};
