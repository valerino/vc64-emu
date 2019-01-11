//
// Created by valerino on 06/01/19.
//

#include <stdio.h>
#include <CMOS65xx.h>
#include <SDL.h>
#include <getopt.h>
#include "CDisplay.h"
#include "CMemory.h"
#include <CSDLUtils.h>
#include <CLog.h>
#include <time.h>

/**
 * shows banner
 */
void banner() {
    printf("vc64 - a c64 emulator\n");
    printf("(c)opyleft, valerino, y2k19\n\n");
}

/**
 * prints usage
 * @param argv
 */
void usage(char** argv) {
    printf("usage: %s -f [file to play]\n", argv[0]);
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

    // initialize sdl
    int res = SDL_Init(SDL_INIT_EVERYTHING);
    if (res != 0) {
        CLog::error("%s", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    CLog::print("SDL initialized OK!");

    // TODO: check bios

    // create display
    CDisplay* display = CDisplay::instance();
    bool initialized = display->init();
    if (!initialized) {
        CLog::error("%s", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    CLog::print("Display initialized OK!");

    // create memory
    CMemory* mem = new CMemory();
    CLog::print("Memory initialized OK!");

    if (path != nullptr) {
        // TODO: load PRG/CRT into memory
        CLog::print("loading file: %s", path);
    }

    // create cpu
    CMOS65xx* cpu = new CMOS65xx(mem);
    if (cpu->reset() != 0) {
        // failed to load bios
        CLog::error("Failed to load bios files!");
        delete mem;
        SDL_Quit();
        return 1;
    }
    CLog::print("CPU initialized OK!");

    // emulate
    // TODO: values for a PAL c64, taken from https://www.ktverkko.fi/~msmakela/8bit/cbm.html
    int cpu_hz = 985248;

    // these many cycles before a screen refresh
    int cycles_to_run = cpu_hz / DISPLAY_PAL_HZ;

    while (1) {
        uint32_t start_ms = SDL_GetTicks();
        // TODO: run cpu cycles

        // TODO: screen update

        // TODO: play audio

        // read input
        bool mustExit;
        uint8_t* keys;
        CSDLUtils::pollEvents(&keys, &mustExit);
        if (mustExit) {
            break;
        }
        // TODO: process input

        // sleep
        uint32_t end_ms = SDL_GetTicks();
        uint32_t elapsed = end_ms - start_ms;
        if (elapsed < 1000) {
            SDL_Delay(1000 - elapsed);
        }
    }

    // done
    delete mem;
    delete cpu;
    SDL_Quit();
    return res;
}