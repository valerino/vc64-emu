#include <stdio.h>
#include <bitutils.h>
#include <SDL.h>
#include "CCIABase.h"

CCIABase::CCIABase(CMOS65xx *cpu, CPLA *pla, uint16_t baseAddress,
                   int connectedTo) {
    _cpu = cpu;
    _pla = pla;
    _baseAddress = baseAddress;
    _connectedTo = connectedTo;
}

CCIABase::~CCIABase() {}

uint8_t CCIABase::readPRA() {
    uint8_t bt;
    CCIABase::read(_baseAddress, &bt);
    return bt;
}
uint8_t CCIABase::readPRB() {
    uint8_t bt;
    CCIABase::read(_baseAddress + 1, &bt);
    return bt;
}

void CCIABase::writePRA(uint8_t pra) { CCIABase::write(_baseAddress, pra); }
void CCIABase::writePRB(uint8_t prb) { CCIABase::write(_baseAddress + 1, prb); }

void CCIABase::read(uint16_t address, uint8_t *bt) {
    int offset = address - _baseAddress;
    switch (offset) {
        /**
         * @todo 0x02-0x03 (DDRA, DDRB)
         */
    case 0x04:
        // TA LO
        *bt = (_timerA & 0xff);
        break;

    case 0x05:
        // TA HI
        *bt = (_timerA & 0xff00) >> 8;
        break;

    case 0x06:
        // TB LO
        *bt = (_timerB & 0xff);
        break;

    case 0x07:
        // TB HI
        *bt = (_timerB & 0xff00) >> 8;
        break;

        /**
         * @todo 0x08-0x0c (TOD 10THS, TOD SEC, TOD MIN, TOD HR, SDR)
         */

    case 0x0d: {
        // ICR
        // @todo: handle other bits
        uint8_t res = 0;
        if (IS_BIT_SET(_timerAStatus,
                       CIA_TIMER_INTERRUPT_UNDERFLOW_TRIGGERED) ||
            IS_BIT_SET(_timerBStatus,
                       CIA_TIMER_INTERRUPT_UNDERFLOW_TRIGGERED)) {
            // an interrupt occurred, set bit 7
            BIT_SET(res, 7);

            if (IS_BIT_SET(_timerAStatus,
                           CIA_TIMER_INTERRUPT_UNDERFLOW_TRIGGERED)) {
                // source is timer A underflow
                BIT_SET(res, 0);
            }
            if (IS_BIT_SET(_timerBStatus,
                           CIA_TIMER_INTERRUPT_UNDERFLOW_TRIGGERED)) {
                // source is timer B underflow
                BIT_SET(res, 1);
            }

            // reading resets flags
            BIT_CLEAR(_timerAStatus, CIA_TIMER_INTERRUPT_UNDERFLOW_TRIGGERED);
            BIT_CLEAR(_timerBStatus, CIA_TIMER_INTERRUPT_UNDERFLOW_TRIGGERED);
        }
        *bt = res;
        break;
    }

    default:
        // read memory
        _cpu->memory()->readByte(address, bt);
        break;
    }
}

void CCIABase::write(uint16_t address, uint8_t bt) {
    int offset = address - _baseAddress;
    switch (offset) {
        /**
         * @todo 0x02-0x03 (DDRA, DDRB)
         */
    case 0x04:
        // TA LO
        _timerALatch = (_timerALatch & 0xff00) | bt;
        break;

    case 0x05:
        // TA HI
        _timerALatch = (_timerALatch & 0xff) | ((int)bt << 8);
        if (!IS_BIT_SET(_timerAStatus, CIA_TIMER_RUNNING)) {
            // reload the timer
            _timerA = _timerALatch;
        }
        break;

    case 0x06:
        // TB LO
        _timerBLatch = (_timerBLatch & 0xff00) | bt;
        break;

    case 0x07:
        // TB HI
        _timerBLatch = (_timerBLatch & 0xff) | ((int)bt << 8);
        if (!IS_BIT_SET(_timerBStatus, CIA_TIMER_RUNNING)) {
            // reload the timer
            _timerB = _timerBLatch;
        }
        break;

        /**
         * @todo 0x08-0x0c (TOD 10THS, TOD SEC, TOD MIN, TOD HR, SDR)
         */

    case 0x0d: {
        // ICR
        // clear bit 5,6
        BIT_CLEAR(bt, 5);
        BIT_CLEAR(bt, 6);
        if (IS_BIT_SET(bt, 0)) {
            if (IS_BIT_SET(bt, 7)) {
                BIT_SET(_timerAStatus, CIA_TIMER_INTERRUPT_UNDERFLOW_ENABLED);
            } else {
                BIT_CLEAR(_timerAStatus, CIA_TIMER_INTERRUPT_UNDERFLOW_ENABLED);
            }
        }
        if (IS_BIT_SET(bt, 1)) {
            if (IS_BIT_SET(bt, 7)) {
                BIT_SET(_timerBStatus, CIA_TIMER_INTERRUPT_UNDERFLOW_ENABLED);
            } else {
                BIT_CLEAR(_timerBStatus, CIA_TIMER_INTERRUPT_UNDERFLOW_ENABLED);
            }
        }
        break;
    }
    case 0x0e:
        // CRA
        // @todo handle other bits
        if (IS_BIT_SET(bt, 0)) {
            // start timer
            BIT_SET(_timerAStatus, CIA_TIMER_RUNNING);
        } else {
            // stop timer
            BIT_CLEAR(_timerAStatus, CIA_TIMER_RUNNING);
        }

        if (IS_BIT_SET(bt, 3)) {
            // timer stops
            BIT_CLEAR(_timerAStatus, CIA_TIMER_RUNNING);
        } else {
            // timer restarts, reload latch
            BIT_SET(_timerAStatus, CIA_TIMER_RUNNING);
            _timerA = _timerALatch;
        }

        if (IS_BIT_SET(bt, 4)) {
            // load latch into the timer
            _timerA = _timerALatch;
        }

        // set timer mode
        if (IS_BIT_SET(bt, 5)) {
            _timerAMode = CIA_TIMER_COUNT_POSITIVE_CNT_SLOPES;
        } else {
            _timerAMode = CIA_TIMER_COUNT_CPU_CYCLES;
        }
        break;

    case 0x0f:
        // CRB
        if (IS_BIT_SET(bt, 0)) {
            // start timer
            BIT_SET(_timerBStatus, CIA_TIMER_RUNNING);
        } else {
            // stop timer
            BIT_CLEAR(_timerBStatus, CIA_TIMER_RUNNING);
        }

        if (IS_BIT_SET(bt, 3)) {
            // timer stops
            BIT_CLEAR(_timerBStatus, CIA_TIMER_RUNNING);
        } else {
            // timer restarts, reload latch
            BIT_SET(_timerBStatus, CIA_TIMER_RUNNING);
            _timerB = _timerBLatch;
        }

        if (IS_BIT_SET(bt, 4)) {
            // load latch into the timer
            _timerB = _timerBLatch;
        }

        // set timer mode
        if (!IS_BIT_SET(bt, 5) && !IS_BIT_SET(bt, 6)) {
            // 00
            _timerBMode = CIA_TIMER_COUNT_CPU_CYCLES;
        } else if (!IS_BIT_SET(bt, 5) && IS_BIT_SET(bt, 6)) {
            // 01
            _timerBMode = CIA_TIMER_COUNT_POSITIVE_CNT_SLOPES;
        } else if (IS_BIT_SET(bt, 5) && !IS_BIT_SET(bt, 6)) {
            // 10
            _timerBMode = CIA_TIMER_COUNT_TIMERA_UNDERFLOW;
        } else if (IS_BIT_SET(bt, 5) && IS_BIT_SET(bt, 6)) {
            // 11
            _timerBMode = CIA_TIMER_COUNT_TIMERA_UNDERFLOW_IF_CNT_HI;
        }

        break;

    default:
        break;
    }

    // write the value anyway
    _cpu->memory()->writeByte(address, bt);
}
void CCIABase::updateTimer(int cycleCount, int timerType) {
    // check which timer we're updating
    bool running = false;
    int mode = 0;
    bool interruptUnderflowEnabled = false;
    uint16_t latch = 0;
    int timer = 0;
    if (timerType == CIA_TIMER_A) {
        running = IS_BIT_SET(_timerAStatus, CIA_TIMER_RUNNING);
        mode = _timerAMode;
        interruptUnderflowEnabled =
            IS_BIT_SET(_timerAStatus, CIA_TIMER_INTERRUPT_UNDERFLOW_ENABLED);
        latch = _timerALatch;
        timer = _timerA;
    } else {
        running = IS_BIT_SET(_timerBStatus, CIA_TIMER_RUNNING);
        mode = _timerBMode;
        interruptUnderflowEnabled =
            IS_BIT_SET(_timerBStatus, CIA_TIMER_INTERRUPT_UNDERFLOW_ENABLED);
        latch = _timerBLatch;
        timer = _timerB;
    }

    // check if the timer is running
    if (!running) {
        return;
    }

    // check timerA mode
    if (mode == CIA_TIMER_COUNT_CPU_CYCLES) {
        timer -= (cycleCount - _prevCycleCount);
        if (timer <= 0) {
            // timer underflows!
            if (interruptUnderflowEnabled) {
                // update interrupt status
                if (timerType == CIA_TIMER_A) {
                    BIT_SET(_timerAStatus,
                            CIA_TIMER_INTERRUPT_UNDERFLOW_TRIGGERED);
                } else {
                    BIT_SET(_timerBStatus,
                            CIA_TIMER_INTERRUPT_UNDERFLOW_TRIGGERED);
                }

                // and trigger irq or nmi depending on which cpu line it's
                // connected to
                if (_connectedTo == CIA_TRIGGERS_IRQ) {
                    _cpu->irq();
                } else {
                    _cpu->nmi();
                }
            }

            // reload timer with latch
            timer = latch;
        }

        // update
        if (timerType == CIA_TIMER_A) {
            _timerA = timer;
        } else {
            _timerB = timer;
        }
    } else {
        // TODO: mode is count slopes at CNT pin
    }
}

int CCIABase::update(int cycleCount) {
    // update timers
    updateTimer(cycleCount, CIA_TIMER_A);
    updateTimer(cycleCount, CIA_TIMER_B);

    // update cycle count
    _prevCycleCount = cycleCount;
    return 0;
}
