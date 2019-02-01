//
// Created by valerino on 06/01/19.
//

#include <stdio.h>
#include <time.h>
#include <CBuffer.h>
#include <CSDLUtils.h>
#include <CLog.h>
#include <CMOS65xx.h>
#include <getopt.h>
#include "CDisplay.h"
#include "CInput.h"
#include "CAudio.h"
#include "CMemory.h"
#include "CCIA1.h"
#include "CCIA2.h"
#include "CVICII.h"
#include "CSID.h"
#include "globals.h"

/**
 * globals
 */
CMOS65xx* cpu = nullptr;
CMemory* mem = nullptr;
CDisplay* display = nullptr;
CInput* input = nullptr;
CAudio* audio = nullptr;
CVICII* vic = nullptr;
CCIA1* cia1 = nullptr;
CCIA2* cia2 = nullptr;
CSID* sid = nullptr;

/**
 * shows banner
 */
void banner() {
    CLog::printRaw("vc64 - a c64 emulator\n");
    CLog::printRaw("(c)opyleft, valerino, y2k19\n\n");
}

/**
 * a callback for memory writes
 */
void cpuCallbackWrite(uint16_t address, uint8_t val) {
    if (address >= VIC_REGISTERS_START && address <= VIC_REGISTERS_END) {
        // accessing vic registers
        vic->write(address, val);
    }
    else if (address >= CIA2_REGISTERS_START && address <= CIA2_REGISTERS_END) {
        // accessing cia-2 registers
        cia2->write(address, val);
    }
    else {
        // default
        cpu->memory()->writeByte(address, val);
    }
}

/**
 * a callback for memory reads
 */
void cpuCallbackRead(uint16_t address, uint8_t* val) {
    if (address >= VIC_REGISTERS_START && address <= VIC_REGISTERS_END) {
        // accessing vic registers
        vic->read(address, val);
    }
    else if (address >= CIA2_REGISTERS_START && address <= CIA2_REGISTERS_END) {
        // accessing cia-2 registers
        cia2->read(address, val);
    }
    else {
        // default
        cpu->memory()->readByte(address, val);
    }
}

/**
 * prints usage
 * @param argv
 */
void usage(char** argv) {
    CLog::error("usage: %s -f <file> [-d]\n\t-f: file to be run\n\t-d: debugger", argv[0]);
}

int main (int argc, char** argv) {
    // prints title
    banner();
    bool debugger = false;

    // get commandline
    char* path = nullptr;
    while (1) {
        int option = getopt(argc, argv, "df:");
        if (option == -1) {
            break;
        }
        switch (option) {
        case '?':
            usage(argv);
            return 1;
        case 'f':
            path = optarg;
            break;
        case 'd':
            debugger = true;
            break;
        default:
            break;
        }
    }

    bool sdlInitialized = false;
    int64_t totalCycles = 0;
    uint32_t startTime = SDL_GetTicks();
    do {
        // initialize sdl
        int res = SDL_Init(SDL_INIT_EVERYTHING);
        if (res != 0) {
            CLog::error("SDL_Init(): %s", SDL_GetError());
            break;
        }
        sdlInitialized = true;
        CLog::print("SDL initialized OK!");

        // create memory
        mem = new CMemory();
        CLog::print("Memory initialized OK!");

        if (path != nullptr) {
            CLog::print("Loading file: %s", path);
            // TODO: load PRG/CRT into memory
        }

        // create cpu
        cpu = new CMOS65xx(mem, cpuCallbackRead, cpuCallbackWrite);
        if (cpu->reset() != 0) {
            // failed to load bios
            CLog::error("Failed to load bios files!");
            break;
        }
        CLog::print("CPU initialized OK!");
        if (debugger) {
            CLog::print("debugging mode ACTIVE!");
        }

        // create additional chips
        cia1 = new CCIA1(cpu);
        cia2 = new CCIA2(cpu);
        sid = new CSID(cpu);
        vic = new CVICII(cpu,cia2);

        // create the subsystems (display, input, audio)
        display = new CDisplay(vic);
        input = new CInput(cia1);
        audio = new CAudio(sid);
        SDLDisplayCreateOptions displayOpts = {};
        displayOpts.posX = SDL_WINDOWPOS_CENTERED;
        displayOpts.posY = SDL_WINDOWPOS_CENTERED;
        displayOpts.scaleFactor = 2;
        displayOpts.w = VIC_PAL_SCREEN_W;
        displayOpts.h = VIC_PAL_SCREEN_H;
        displayOpts.windowName = "vc64-emu";
        displayOpts.rendererFlags = SDL_RENDERER_ACCELERATED;
        char* sdlError;
        res = display->init(&displayOpts, &sdlError);
        if (res != 0) {
            CLog::error("display->init(): %s", sdlError);
            break;
        }
        CLog::print("Display initialized OK!");

        // pal c64 runs at 0,985mhz, 50hz, 19656 cycles per second
        int cpu_hz = CPU_HZ;
        int cyclesPerSecond = abs (cpu_hz / VIC_PAL_HZ);
        int cycleCount = cyclesPerSecond;
        bool running = true;
        bool mustBreak = false;
        while (running) {
            bool mustExit = false;
            // step the cpu
            int cycles = cpu->step(debugger, mustBreak);
            if (cycles == -1) {
                // exit loop
                running = false;
                continue;
            }
            cycleCount -= cycles;

            // reset break status if any
            mustBreak = false;

            if (!cpu->isTestMode()) {
                // update chips
                vic->run(cycleCount);
                cia1->run(cycleCount);
                cia2->run(cycleCount);
                //sid->run();
            }

            if (cycleCount <= 0) {
                if (!cpu->isTestMode()) {
                    // screen update, returns raster time
                    display->update();
                }

                // process input
                uint8_t* keys;
                CSDLUtils::pollEvents(&keys, &mustExit);

                // check for break into debugger
                if (debugger) {
                    // ode to softice :)
                    if (keys[SDL_SCANCODE_LCTRL] && keys[SDL_SCANCODE_D]) {
                        // break requested!
                        mustBreak = true;
                    }
                }
                if (mustExit) {
                    // exit loop
                    running = false;
                    continue;
                }

                if (!cpu->isTestMode()) {
                    input->update(keys);

                    // play audio
                    audio->update();
                }

                // next iteration
                cycleCount += cyclesPerSecond;
                totalCycles += cycleCount;
            }
        }
    } while(0);
    uint32_t endTime = SDL_GetTicks() - startTime;
    CLog::print("done, step for %02d:%02d:%02d, total CPU cycles=%" PRId64, endTime / 1000 / 60 / 60, endTime / 1000 / 60, (endTime / 1000) % 60, totalCycles);

    // done
    SAFE_DELETE(cia1)
    SAFE_DELETE(cia2)
    SAFE_DELETE(vic)
    SAFE_DELETE(sid)
    SAFE_DELETE(display)
    SAFE_DELETE(input)
    SAFE_DELETE(audio)
    SAFE_DELETE(mem)
    SAFE_DELETE(cpu)
    if (sdlInitialized) {
        SDL_Quit();
    }
    return 0;
}
