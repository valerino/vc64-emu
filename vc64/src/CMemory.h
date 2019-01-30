//
// Created by valerino on 10/01/19.
//

#ifndef VC64_EMU_CMEMORY_H
#define VC64_EMU_CMEMORY_H

#include <cstdint>
#include <IMemory.h>

// the whole 64k size
#define MEMORY_SIZE 0x10000

// memory map
#define MEMORY_BASIC_ADDRESS    0xa000
#define MEMORY_BASIC_SIZE       0x2000
#define MEMORY_CHARSET_ADDRESS  0xd000
#define MEMORY_CHARSET_SIZE     0x1000
#define MEMORY_KERNAL_ADDRESS   0xe000
#define MEMORY_KERNAL_SIZE      0x2000
#define MEMORY_VIDEO_ADDRESS     0x400

/**
 * implements the emulated memory
 * reference: https://www.c64-wiki.com/wiki/Memory_Map
 */

/*
        RAM Table

        Hex Address	Dec Address	Page	        Contents
        ------------------------------------------------
        $0000-$00FF	0-255	    Page 0	        Zeropage addressing
        $0100-$01FF	256-511	    Page 1	        Enhanced Zeropage contains the stack
        $0200-$02FF	512-767	    Page 2	        Operating System and BASIC pointers
        $0300-$03FF	768-1023	Page 3	        Operating System and BASIC pointers
        $0400-$07FF	1024-2047	Page 4-7	    Screen Memory
        $0800-$9FFF	2048-40959	Page 8-159	    Free BASIC program storage area (38911 bytes)
        $A000-$BFFF	40960-49151	Page 160-191	Free machine language program storage area (when switched-out with ROM)
        $C000-$CFFF	49152-53247	Page 192-207	Free machine language program storage area
        $D000-$D3FF	53248-54271	Page 208-211
        $D400-$D7FF	54272-54527	Page 212-215
        $D800-$DBFF	55296-56319	Page 216-219
        $DC00-$DCFF	56320-56575	Page 220
        $DD00-$DDFF	56576-56831	Page 221
        $DE00-$DFFF	56832-57343	Page 222-223	Reserved for interface extensions
        $E000-$FFFF	57344-65535	Page 224-255	Free machine language program storage area (when switched-out with ROM)

        ROM Table
        Cartridge ROM only becomes resident if attached to the expansion port on power-up. It is included for completeness as a record of the addresses it occupies as a ROM bank.

        Hex Address	Dec Address	Page	        Contents
        ------------------------------------------------
        $8000-$9FFF	32768-40959	Page 128-159	Cartridge ROM (low)
        $A000-$BFFF	40960-49151	Page 160-191	BASIC interpretor ROM or cartridge ROM (high)
        $D000-$DFFF	53248-57343	Page 208-223	Character generator ROM
        $E000-$FFFF	57344-65535	Page 224-255	KERNAL ROM or cartridge ROM (high)

        I/O Table
        Hex Address	Dec Address	Page	        Contents
        ------------------------------------------------
        $0000-$0001	0-1	-	                    CPU I/O port - see Zeropage
        $D000-$D3FF	53248-54271	Page 208-211	VIC-II registers
        $D400-$D7FF	54272-55295	Page 212-215	SID registers
        $D800-$DBFF	55296-56319	Page 216-219	Color Memory
        $DC00-$DCFF	56320-56575	Page 220	    CIA 1
        $DD00-$DDFF	56576-56831	Page 221	    CIA 2
        $DE00-$DEFF	56832-57087	Page 222	    I/O 1
        $DF00-$DFFF	57088-57343	Page 223	    I/O 2

 */
class CMemory: public IMemory {
private:
    // global/writable memory
    uint8_t* _mem;

    // for commodity, we keep the ROMS aliased there in separate buffers (which shadows the respective address in the global memory)
    uint8_t* _charRom;
    uint8_t* _kernalRom;
    uint8_t* _basicRom;

    /**
     * load bios files
     * @return 0 on success, or errno
     */
    int loadBios();

public:
    uint8_t readByte(uint32_t address, uint8_t *b) override;

    uint16_t readWord(uint32_t address, uint16_t *w) override;

    int writeByte(uint32_t address, uint8_t b) override;

    int writeWord(uint32_t address, uint16_t w) override;

    int writeBytes(uint32_t address, uint8_t *b, uint32_t size) override;

    int readBytes(uint32_t address, uint8_t *b, uint32_t bufferSize, uint32_t readSize) override;

    int init() override;

    /**
     * get the raw memory pointer
     * @param size if not null, returns the memory size here
     * @return the memory pointer
     */
    uint8_t* ram(uint32_t *size);

    CMemory();
    ~CMemory();
};


#endif //VC64_EMU_CMEMORY_H
