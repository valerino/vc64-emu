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
bool running = true;
bool mustBreak = false;

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
    else if (address >= CIA1_REGISTERS_START && address <= CIA1_REGISTERS_END) {
        // accessing cia-1 registers
        cia1->write(address, val);
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
    else if (address >= CIA1_REGISTERS_START && address <= CIA1_REGISTERS_END) {
        // accessing cia-1 registers
        cia1->read(address, val);
    }
    else {
        // default
        cpu->memory()->readByte(address, val);
    }
}

/**
 * callback for sdl events
 * @param event
 */
void sdlEventCallback(SDL_Event* event) {
    if (event->type == SDL_QUIT) {
        // SDL window closed, application must quit asap
        CLog::print("QUIT requested!");
        running = false;
        return;
    }

    switch (event->type) {
        case SDL_KEYUP:
        case SDL_KEYDOWN: {
            // process input
            uint32_t hotkeys = 0;
            input->update(event, &hotkeys);
            if (hotkeys & HOTKEY_DEBUGGER) {
                // we must break!
                CLog::print("DEBUGBREAK requested (works only in debugger mode!)");
                mustBreak = true;
            }
        }
            break;
        default:
            break;
    }
}

/**
 * prints usage
 * @param argv
 */
void usage(char** argv) {
    CLog::error("usage: %s -f <file> [-dsh]\n" \
    "\t-f: file to be update (TAP/PRG/CRT)\n" \
    "\t-d: debugger\n" \
    "\t-s: fullscreen\n" \
    "\t-h: this help\n",
    argv[0]);
}

int main (int argc, char** argv) {
    CLog::enableLog(true);

    // prints title
    banner();
    bool debugger = false;
    bool fullScreen = false;

    // get commandline
    char* path = nullptr;
    while (1) {
        int option = getopt(argc, argv, "dshf:");
        if (option == -1) {
            break;
        }
        switch (option) {
        case '?':
        case 'h':
            usage(argv);
            return 1;
        case 'f':
            path = optarg;
            break;
        case 'd':
            debugger = true;
            break;
        case 's':
            fullScreen = true;
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
        displayOpts.windowFlags = fullScreen ? SDL_WINDOW_FULLSCREEN : 0;
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
        while (running) {
            // step the cpu
            int cycles = cpu->step(debugger, debugger ? mustBreak : false);
            if (cycles == -1) {
                // exit loop
                running = false;
                continue;
            }
            cycleCount -= cycles;

            // reset break status if any
            mustBreak = false;

            // after each cycle, update internal chips state
            if (!cpu->isTestMode()) {
                vic->update(cycleCount);
                cia1->update(cycleCount);
                cia2->update(cycleCount);
                sid->update(cycleCount);
            }

            // after cycles x seconds elapsed, update the display, process input and play sound
            if (cycleCount <= 0) {
                if (!cpu->isTestMode()) {
                    // update display
                    cycleCount -= display->update();
                }

                // process input
                CSDLUtils::pollEvents(sdlEventCallback);

                if (!cpu->isTestMode()) {
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
