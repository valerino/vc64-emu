#pragma once

#include <CMOS65xx.h>

/**
 * timer modes
 */
#define CIA_TIMER_COUNT_CPU_CYCLES 0
#define CIA_TIMER_COUNT_POSITIVE_CNT_SLOPES 1
#define CIA_TIMER_COUNT_TIMERA_UNDERFLOW 2
#define CIA_TIMER_COUNT_TIMERA_UNDERFLOW_IF_CNT_HI 3

/**
 * timer status (bits)
 */
#define CIA_TIMER_INTERRUPT_UNDERFLOW_TRIGGERED 0
#define CIA_TIMER_INTERRUPT_UNDERFLOW_ENABLED 1
#define CIA_TIMER_RUNNING 2

/**
 * timer types
 */
#define CIA_TIMER_A 0
#define CIA_TIMER_B 1

/**
 * cia1 is connected to the cpu irq line, cia2 is connected to the cpu nmi line
 */
#define CIA_TRIGGERS_IRQ 0
#define CIA_TRIGGERS_NMI 1

/**
 * @brief implements the base class for the CIA-6526 chip
 * https://www.c64-wiki.com/wiki/CIA
 */
class CCIABase {
  public:
    /**
     * @brief constructor
     * @param cpu the cpu instance
     * @param baseAddress CIA base address
     * @param connectedTo CIA_TRIGGERS_IRQ or CIA_TRIGGERS_NMI
     */
    CCIABase(CMOS65xx *cpu, uint16_t baseAddress, int connectedTo);
    virtual ~CCIABase();

    /**
     * @brief update the internal state
     * @param cycleCount cpu current cycle count
     * @return additional cycles used
     */
    int update(int cycleCount);

    /**
     * read from chip memory
     * @param address
     * @param bt
     */
    virtual void read(uint16_t address, uint8_t *bt);

    /**
     * write to chip memory
     * @param address
     * @param bt
     */
    virtual void write(uint16_t address, uint8_t bt);

  protected:
    CMOS65xx *_cpu = nullptr;
    uint16_t _timerALatch = 0;
    uint16_t _timerBLatch = 0;
    uint16_t _timerA = 0;
    uint16_t _timerB = 0;
    int _timerAStatus = 0;
    int _timerBStatus = 0;
    int _timerAMode = 0;
    int _timerBMode = 0;
    int _prevCycleCount = 0;
    int _connectedTo = 0;
    uint16_t _baseAddress = 0;
    void updateTimer(int cycleCount, int timerType);
};
