#pragma once

#include <stdint.h>

/**
 * @brief different mapping types depending on the PLA latch configuration
 * @see https://www.c64-wiki.com/wiki/Bank_Switching
 */
#define PLA_MAP_RAM 0
#define PLA_MAP_BASIC_ROM 1
#define PLA_MAP_KERNAL_ROM 2
#define PLA_MAP_CHARSET_ROM 3
#define PLA_MAP_CART_ROM_LO 4
#define PLA_MAP_CART_ROM_HI 5
#define PLA_MAP_IO_DEVICES 6
#define PLA_MAP_UNDEFINED 7

/**
 * @brief implements the PLA (programmable logic array), which handles the
 * memorymap/bankswitching
 * @see https://www.c64-wiki.com/wiki/PLA_(C64_chip)
 * @see https://www.c64-wiki.com/wiki/Bank_Switching
 */
class CPLA {
  public:
    CPLA();
    virtual ~CPLA();

    /**
     * @brief returns whether LORAM (bit 0 in the control port at $01) is set
     * @return bool
     */
    bool isLoram();

    /**
     * @brief returns whether HIRAM (bit 1 in the control port at $01) is set
     * @return bool
     */
    bool isHiram();

    /**
     * @brief returns whether CHAREN (bit 3 in the control port at $01) is set
     * @return bool
     */
    bool isCharen();

    /**
     * @brief returns whether the GAME bit is set (a game cartridge is inserted)
     * @see https://www.c64-wiki.com/wiki/Expansion_Port
     * @return bool
     */
    bool isGame();

    /**
     * @brief returns whether the EXROM bit is set (a game cartridge is
     * inserted)
     * @see https://www.c64-wiki.com/wiki/Expansion_Port
     * @return bool
     */
    bool isExrom();

    /**
     * @brief set the GAME bit in the latch (bit 3)
     * @see https://www.c64-wiki.com/wiki/Expansion_Port
     * @param set set/unset
     */
    void setGame(bool set);

    /**
     * @brief set the EXROM bit in the latch (bit 4)
     * @see https://www.c64-wiki.com/wiki/Expansion_Port
     * @param set set/unset
     */
    void setExrom(bool set);

    /**
     * @brief setup the memory mapping bits according to the control port
     * (zeropage $1) value
     * @param controlPort value from $01
     */
    void setupMemoryMapping(uint8_t controlPort);

    /**
     * @brief returns one of the PLA_MAP types, indicating what to map where
     * @see https://www.c64-wiki.com/wiki/Bank_Switching
     * @return
     */
    int mapAddressToType(uint16_t address);

    int latch() { return _latch; }

  private:
    uint8_t _latch = 0;
};
