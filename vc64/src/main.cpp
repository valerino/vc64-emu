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
 * prints usage
 * @param argv
 */
void usage(char** argv) {
    CLog::error("usage: %s -f [file to play]\n", argv[0]);
}

/**
 * called by the cpu after certain instructions, to transfer control to other chips
 * @param type CPU_MEM_READ, CPU_MEM_WRITE
 * @param address the read/write address
 * @param val the byte read/written
 */
void cpuCallback (int type, uint16_t address, uint8_t val) {
    //CLog::print("callback!");
}

int main (int argc, char** argv) {
    // prints title
    banner();

    // TODO: getopt commandline
    char* path = nullptr;
    int option = getopt(argc, argv, "f:");
    switch (option) {
        case '?':
            usage(argv);
            return 1;
        case 'f':
            path = optarg;
            break;
        default:
            break;
    }

    bool sdlInitialized = false;
    uint64_t totalCycles = 0;
    uint32_t startTime = SDL_GetTicks();
    do {
        // initialize sdl
        int res = SDL_Init(SDL_INIT_EVERYTHING);
        if (res != 0) {
            CLog::error("%s", SDL_GetError());
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
        cpu = new CMOS65xx(mem, cpuCallback);
        if (cpu->reset() != 0) {
            // failed to load bios
            CLog::error("Failed to load bios files!");
            break;
        }
        CLog::print("CPU initialized OK!");

        // create additional chips
        cia1 = new CCIA1(cpu);
        cia2 = new CCIA2(cpu);
        sid = new CSID(cpu);
        vic = new CVICII(cpu);

        // create the subsystems (display, input, audio)
        display = new CDisplay();
        input = new CInput();
        audio = new CAudio();
        SDLDisplayCreateOptions displayOpts = {};
        displayOpts.posX = SDL_WINDOWPOS_CENTERED;
        displayOpts.posY = SDL_WINDOWPOS_CENTERED;
        displayOpts.w = 640;
        displayOpts.h = 480;
        displayOpts.windowName = "vc64-emu";
        displayOpts.rendererFlags = SDL_RENDERER_ACCELERATED;
        char* sdlError;
        res = display->init(&displayOpts, &sdlError);
        if (res != 0) {
            CLog::error("%s", sdlError);
            break;
        }
        CLog::print("Display initialized OK!");

        // pal c64 runs at 0,985mhz
        int cpu_hz = 985248;

        // these many cycles before vsync
        int cyclesToRun = cpu_hz / DISPLAY_PAL_HZ;
        int remainingCycles = 0;
        bool running = true;
        while (running) {
            bool mustExit = false;
            uint32_t start_ms = SDL_GetTicks();

            // run the cpu until vsync
            int cyclesNow = cyclesToRun - remainingCycles;
            remainingCycles = cpu->run(cyclesNow, &mustExit);
            totalCycles += cyclesNow - remainingCycles;

            // TODO: update cia1, cia2, vic

            // screen update
            uint8_t* frameBuffer;
            uint32_t fbSize;
            mem->videoMemory(&frameBuffer, &fbSize);
            display->update(frameBuffer, fbSize);

            // TODO: play audio
            uint8_t* keys;
            CSDLUtils::pollEvents(&keys, &mustExit);
            if (mustExit) {
                // exit loop
                running = false;
                continue;
            }

            // TODO: process input

            // sleep until 1 second reached
            uint32_t end_ms = SDL_GetTicks();
            uint32_t elapsed = end_ms - start_ms;
            if (elapsed < 1000) {
                SDL_Delay(1000 - elapsed);
            }
        }

    } while(0);
    uint32_t endTime = SDL_GetTicks() - startTime;
    CLog::print("done, run for %02d:%02d:%02d, total CPU cycles=%lld", endTime / 1000 / 60 / 60, endTime / 1000 / 60, (endTime / 1000) % 60, totalCycles);

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
