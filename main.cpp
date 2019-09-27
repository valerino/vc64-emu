//
// Created by valerino on 06/01/19.
//

#include <stdio.h>
#include <SDL.h>
#include <CBuffer.h>
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
CMOS65xx *cpu = nullptr;
CMemory *mem = nullptr;
CDisplay *display = nullptr;
CInput *input = nullptr;
CAudio *audio = nullptr;
CVICII *vic = nullptr;
CCIA1 *cia1 = nullptr;
CCIA2 *cia2 = nullptr;
CSID *sid = nullptr;
bool running = true;
bool debugger = false;
bool hotkeyDbgBreak = false;
int64_t totalCycles = 0;
char *path = nullptr;

/**
 * shows banner
 */
void banner() {
    printf("vc64 - a c64 emulator\n"
           "\t(c)opyleft, valerino, y2k19\n");
}

/**
 * a callback for memory writes
 */
void cpuCallbackWrite(uint16_t address, uint8_t val) {
    if (address >= VIC_REGISTERS_START && address <= VIC_REGISTERS_END) {
        // accessing vic registers
        vic->write(address, val);
    } else if (address >= CIA2_REGISTERS_START &&
               address <= CIA2_REGISTERS_END) {
        // accessing cia-2 registers
        cia2->write(address, val);
    } else if (address >= CIA1_REGISTERS_START &&
               address <= CIA1_REGISTERS_END) {
        // accessing cia-1 registers
        cia1->write(address, val);
    } else {
        // default
        cpu->memory()->writeByte(address, val);
    }
}

/**
 * a callback for memory reads
 */
void cpuCallbackRead(uint16_t address, uint8_t *val) {
    if (address >= VIC_REGISTERS_START && address <= VIC_REGISTERS_END) {
        // accessing vic registers
        vic->read(address, val);
    } else if (address >= CIA2_REGISTERS_START &&
               address <= CIA2_REGISTERS_END) {
        // accessing cia-2 registers
        cia2->read(address, val);
    } else if (address >= CIA1_REGISTERS_START &&
               address <= CIA1_REGISTERS_END) {
        // accessing cia-1 registers
        cia1->read(address, val);
    } else {
        // default
        cpu->memory()->readByte(address, val);
    }
}

/**
 * @brief poll for SDL events (input, etc...) and takes the appropriate action
 */
void pollSdlEvents() {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT) {
            // SDL window closed, application must quit asap
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "QUIT requested!");
            running = false;
            break;
        }

        switch (ev.type) {
        case SDL_KEYUP:
        case SDL_KEYDOWN: {
            // process input
            uint32_t hotkeys = 0;
            input->update(&ev, &hotkeys);
            if (hotkeys == HOTKEY_DEBUGGER && debugger) {
                // we must break!
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                            "HOTKEY DEBUGBREAK requested!");
                hotkeyDbgBreak = true;
            } else if (hotkeys == HOTKEY_PASTE_TEXT) {
                // fill the clipboard queue to be processed in the main loop
                input->fillClipboardQueue();
            } else if (hotkeys == HOTKEY_FORCE_EXIT) {
                // force exit
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "HOTKEY FORCE exit!");
                running = false;
            }
            break;
        }
        default:
            break;
        }
    }
}

/**
 * @brief prints usage
 * @param argv
 */
void usage(char **argv) {
    printf("usage: %s -f <file> [-dsh]\n"
           "\t-f: file to be loaded (PRG only is supported as now)\n"
           "\t-t: run cpu test in test/6502_functional_test.bin\n"
           "\t-d: debugger\n"
           "\t-s: fullscreen\n"
           "\t-h: this help\n",
           argv[0]);
}

/**
 * just test the cpu
 */
void testCpuMain(bool debugger) {
    while (running) {
        // step the cpu
        int cycles = cpu->step(debugger, debugger ? hotkeyDbgBreak : false);
        if (cycles == -1) {
            // exit loop
            running = false;
            continue;
        }
        totalCycles += cycles;

        // reset hotkey dbg-break status if any
        hotkeyDbgBreak = false;
    }
}

/**
 * once the cpu has reached enough cycles to have loaded the BASIC interpreter,
 * issue a load of our prg. this trigger only once!
 */
void handlePrgLoading() {
    if (totalCycles > 2570000 && path) {
        // enough cycles passed....
        SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
                     "cycle=%lld, loading prg at %s", totalCycles, path);
        // TODO: determine if it's a prg, either fail....
        int res = mem->loadPrg(path);
        if (res == 0) {
            // inject run in the keyboard buffer
            input->injectKeyboardBuffer("RUN\r");
        }

        // we don't need this to trigger anymore
        path = nullptr;
    }
}

int main(int argc, char **argv) {
    // prints title
    banner();
    bool fullScreen = false;
    bool isTestCpu = false;

    // parse commandline
    while (1) {
        int option = getopt(argc, argv, "dshtf:");
        if (option == -1) {
            break;
        }
        switch (option) {
        case '?':
        case 'h':
            usage(argv);
            return 1;
        case 't':
            isTestCpu = true;
            break;
        case 'f':
            path = optarg;
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "file path=%s", path);
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
    uint32_t startTime = SDL_GetTicks();
    int res = 0;
    do {
        // initialize sdl
        res = SDL_Init(SDL_INIT_EVERYTHING);
        if (res != 0) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_Init(): %s",
                         SDL_GetError());
            break;
        }
        sdlInitialized = true;
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "SDL initialized OK!");
        SDL_LogSetAllPriority(SDL_LOG_PRIORITY_DEBUG);

        // create memory
        mem = new CMemory();
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "memory initialized OK!");

        // create cpu
        cpu = new CMOS65xx(mem, cpuCallbackRead, cpuCallbackWrite);
        if (cpu->reset(isTestCpu) != 0) {
            // failed to load bios
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "failed to load bios files!");
            break;
        }
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "CPU initialized OK!");
        if (debugger) {
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
                         "debugging mode ACTIVE!");
        }

        // create additional chips
        cia1 = new CCIA1(cpu);
        cia2 = new CCIA2(cpu);
        sid = new CSID(cpu);
        vic = new CVICII(cpu, cia2);

        // create the subsystems (display, input, audio)
        try {
            display = new CDisplay(vic, "vc64-emu", fullScreen);
        } catch (std::exception ex) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "display->init(): %s",
                         ex.what());
            break;
        }
        input = new CInput(cia1);
        audio = new CAudio(sid);
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "display initialized OK!");

        int timeNow = SDL_GetTicks();
        if (isTestCpu) {
            // running test
            SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
                         "running CPU in test mode!");
            testCpuMain(debugger);
            res = 0;
            break;
        }

        // 312 lines * 63 Cycles = 19656
        int cyclesPerFrame = 19656;
        int msecPerFrame = 20; // (50 : 1 = 1: x) * 1000
        int cycleCounter = cyclesPerFrame;
        while (running) {
            // step the cpu
            int cycles = cpu->step(debugger, debugger ? hotkeyDbgBreak : false);
            if (cycles == -1) {
                // exit loop
                running = false;
                continue;
            }
            totalCycles += cycles;

            // reset hotkey-dbgbreak status if any (for the debugger)
            hotkeyDbgBreak = false;

            // updating vic takes into account the less scanlines for badlines
            // (23 vs 63)
            int c = vic->update(totalCycles);
            cycles += c;
            // totalCycles += c;

            // update other chips
            cia1->update(totalCycles);
            cia2->update(totalCycles);
            sid->update(totalCycles);

            // update cyclecounter
            cycleCounter -= cycles;
            if (cycleCounter <= 0) {
                // draw a frame
                display->update();
                cycleCounter += cyclesPerFrame;

                // poll events
                pollSdlEvents();

                // sleep for the remaining time, if any
                int timeThen = SDL_GetTicks();
                int diff = timeThen - timeNow;
                if (diff < msecPerFrame) {
                    SDL_Delay(msecPerFrame - diff);
                }
                timeNow = timeThen;

                // handle clipboard, if any
                input->checkClipboard(totalCycles, cyclesPerFrame, 5);
            }

            // handle prg loading, if any and if enough cycles has passed, only
            // once
            handlePrgLoading();
        }
    } while (0);

    // calculate some statistics
    uint32_t endTime = SDL_GetTicks() - startTime;
    printf("done, running for %02d:%02d:%02d, total CPU cycles=%lld\n",
           endTime / 1000 / 60 / 60, endTime / 1000 / 60, (endTime / 1000) % 60,
           totalCycles);

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
