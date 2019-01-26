//
// Created by valerino on 19/01/19.
//

#include "CVICII.h"
#include <CLog.h>
#include "debugflags.h"
CVICII::~CVICII() {

}

CVICII::CVICII(CMOS65xx *cpu) {
    _cpu = cpu;
    _rasterIrqLine = 0;
}

void CVICII::doIo(int type, uint16_t address, uint8_t bt) {
    switch (address) {
        case VIC_REG_RASTER_COUNT:
            if (type == CPU_MEM_WRITE) {
                // raster line is $d012 + bit7 of the control register 1
                _rasterIrqLine = bt;
#ifdef DEBUG_VIC
                CLog::printRaw("rastercount=%d\n", _rasterIrqLine);
#endif
            }
            break;

        case VIC_REG_CONTROL1:
            if (type == CPU_MEM_WRITE) {
                // raster line is $d012 + bit7 of the control register 1
                _rasterIrqLine |= (bt & 0x80) << 1;
#ifdef DEBUG_VIC
                CLog::printRaw("rastercount (+cr1)=%d\n", _rasterIrqLine);
#endif
            }
            break;
    }
}

CMOS65xx *CVICII::cpu() {
    return _cpu;
}

int CVICII::run(int cycleCount) {
    if (cycleCount % VIC_PAL_LINES == 0) {
        _rasterIrqLine++;
        if (_rasterIrqLine > VIC_PAL_LINES) {
            _rasterIrqLine = 0;
            _cpu->memory()->writeByte(VIC_REG_RASTER_COUNT,0);
            _cpu->irq();
            return 40;
        }
    }
    return 0;
}
