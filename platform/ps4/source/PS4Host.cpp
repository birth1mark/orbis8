#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <fstream>
#include <iostream>
#include <vector>
#include <string>

using namespace std;

#include "../../SDL2Common/source/sdl2basehost.h"
#include "../../../source/host.h"
#include "../../../source/hostVmShared.h"
#include "../../../source/nibblehelpers.h"
#include "../../../source/logger.h"
#include "../../../source/filehelpers.h"

// SDL2 includes
#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_gamecontroller.h>

#define WINDOW_SIZE_X 1920
#define WINDOW_SIZE_Y 1080

#define WINDOW_FLAGS SDL_WINDOW_FULLSCREEN_DESKTOP
#define RENDERER_FLAGS (SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC)
#define PIXEL_FORMAT SDL_PIXELFORMAT_ARGB8888

// DualShock 4 button mappings for SDL_CONTROLLER_BUTTON
#define DS4_BTN_CROSS       SDL_CONTROLLER_BUTTON_A
#define DS4_BTN_CIRCLE      SDL_CONTROLLER_BUTTON_B
#define DS4_BTN_SQUARE      SDL_CONTROLLER_BUTTON_X
#define DS4_BTN_TRIANGLE    SDL_CONTROLLER_BUTTON_Y
#define DS4_BTN_OPTIONS     SDL_CONTROLLER_BUTTON_START
#define DS4_BTN_SHARE       SDL_CONTROLLER_BUTTON_BACK
#define DS4_BTN_L1          SDL_CONTROLLER_BUTTON_LEFTSHOULDER
#define DS4_BTN_R1          SDL_CONTROLLER_BUTTON_RIGHTSHOULDER
#define DS4_BTN_DPAD_UP     SDL_CONTROLLER_BUTTON_DPAD_UP
#define DS4_BTN_DPAD_DOWN   SDL_CONTROLLER_BUTTON_DPAD_DOWN
#define DS4_BTN_DPAD_LEFT   SDL_CONTROLLER_BUTTON_DPAD_LEFT
#define DS4_BTN_DPAD_RIGHT  SDL_CONTROLLER_BUTTON_DPAD_RIGHT

// Analog joystick dead zone
const int JOYSTICK_DEAD_ZONE = 8000;
static int jxDir = 0;
static int jyDir = 0;

static SDL_Event event;
static SDL_Point touchLocation = { 128 / 2, 128 / 2 };
static SDL_GameController* controller = NULL;

static string _ps4SettingsDir = "/data/orbis8";
static string _ps4SettingsPrefix = "/data/orbis8/";
static string _ps4CartDir = "/app0/carts";
static string _ps4CustomBiosLua = 
    "cartpath = \"/app0/carts/\"\n"
    "selectbtn = \"x\"\n"
    "pausebtn = \"options\"";

static void ensure_dir(const char* path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        mkdir(path, 0777);
    }
}

Host::Host(int windowWidth, int windowHeight) {
    // Ensure Orbis8 internal storage paths exist on PS4 /data partition
    ensure_dir("/data");
    ensure_dir("/data/orbis8");
    ensure_dir("/data/orbis8/cdata");
    ensure_dir("/data/orbis8/carts");

    // Check if USB storage with pico8 carts is plugged in
    struct stat st;
    if (stat("/mnt/usb0/pico8", &st) == 0 && S_ISDIR(st.st_mode)) {
        _ps4CartDir = "/mnt/usb0/pico8";
    } else if (stat("/mnt/usb1/pico8", &st) == 0 && S_ISDIR(st.st_mode)) {
        _ps4CartDir = "/mnt/usb1/pico8";
    }

    // Initialize GameController subsystem
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) == 0) {
        for (int i = 0; i < SDL_NumJoysticks(); ++i) {
            if (SDL_IsGameController(i)) {
                controller = SDL_GameControllerOpen(i);
                if (controller) {
                    Logger_Write("Connected GameController: %s\n", SDL_GameControllerName(controller));
                    break;
                }
            }
        }
    }

    setPlatformParams(
        WINDOW_SIZE_X,
        WINDOW_SIZE_Y,
        WINDOW_FLAGS,
        RENDERER_FLAGS,
        PIXEL_FORMAT,
        _ps4SettingsPrefix,
        _ps4CustomBiosLua,
        _ps4CartDir
    );
}

InputState_t Host::scanInput() {
    currKDown = 0;
    uint8_t kUp = 0;
    int prevJxDir = jxDir;
    int prevJyDir = jyDir;
    stretchKeyPressed = false;
    uint8_t mouseBtnState = 0;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_CONTROLLERBUTTONDOWN:
                switch (event.cbutton.button) {
                    case DS4_BTN_OPTIONS:    currKDown |= P8_KEY_PAUSE; break;
                    case DS4_BTN_DPAD_LEFT:  currKDown |= P8_KEY_LEFT;  break;
                    case DS4_BTN_DPAD_RIGHT: currKDown |= P8_KEY_RIGHT; break;
                    case DS4_BTN_DPAD_UP:    currKDown |= P8_KEY_UP;    break;
                    case DS4_BTN_DPAD_DOWN:  currKDown |= P8_KEY_DOWN;  break;
                    case DS4_BTN_CROSS:      currKDown |= P8_KEY_X;     break; // 🅾 button
                    case DS4_BTN_CIRCLE:     currKDown |= P8_KEY_O;     break; // ❎ button
                    case DS4_BTN_SQUARE:     currKDown |= P8_KEY_O;     break; // Alternate action
                    case DS4_BTN_TRIANGLE:   stretchKeyPressed = true;  break; // Toggle screen scaling
                    case DS4_BTN_L1:         lDown = true;              break;
                    case DS4_BTN_R1:         rDown = true;              break;
                }
                break;

            case SDL_CONTROLLERBUTTONUP:
                switch (event.cbutton.button) {
                    case DS4_BTN_OPTIONS:    kUp |= P8_KEY_PAUSE; break;
                    case DS4_BTN_DPAD_LEFT:  kUp |= P8_KEY_LEFT;  break;
                    case DS4_BTN_DPAD_RIGHT: kUp |= P8_KEY_RIGHT; break;
                    case DS4_BTN_DPAD_UP:    kUp |= P8_KEY_UP;    break;
                    case DS4_BTN_DPAD_DOWN:  kUp |= P8_KEY_DOWN;  break;
                    case DS4_BTN_CROSS:      kUp |= P8_KEY_X;     break;
                    case DS4_BTN_CIRCLE:     kUp |= P8_KEY_O;     break;
                    case DS4_BTN_SQUARE:     kUp |= P8_KEY_O;     break;
                    case DS4_BTN_L1:         lDown = false;       break;
                    case DS4_BTN_R1:         rDown = false;       break;
                }
                break;

            case SDL_CONTROLLERAXISMOTION:
                if (event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX) {
                    if (event.caxis.value < -JOYSTICK_DEAD_ZONE) {
                        jxDir = -1;
                    } else if (event.caxis.value > JOYSTICK_DEAD_ZONE) {
                        jxDir = 1;
                    } else {
                        jxDir = 0;
                    }
                } else if (event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
                    if (event.caxis.value < -JOYSTICK_DEAD_ZONE) {
                        jyDir = -1;
                    } else if (event.caxis.value > JOYSTICK_DEAD_ZONE) {
                        jyDir = 1;
                    } else {
                        jyDir = 0;
                    }
                }
                break;

            case SDL_QUIT:
                quit = 1;
                break;
        }
    }

    // L1 + R1 + Share / Options combo to exit back to PS4 menu
    if (lDown && rDown && (currKDown & P8_KEY_PAUSE)) {
        quit = 1;
    }

    currKHeld |= currKDown;
    currKHeld ^= kUp;

    // Analog stick mapped to D-pad inputs
    if (jxDir > 0) {
        currKHeld |= P8_KEY_RIGHT;
        currKHeld &= ~(P8_KEY_LEFT);
        if (prevJxDir != jxDir) currKDown |= P8_KEY_RIGHT;
    } else if (jxDir < 0) {
        currKHeld |= P8_KEY_LEFT;
        currKHeld &= ~(P8_KEY_RIGHT);
        if (prevJxDir != jxDir) currKDown |= P8_KEY_LEFT;
    } else if (prevJxDir != 0) {
        currKHeld &= ~(P8_KEY_RIGHT | P8_KEY_LEFT);
        currKDown &= ~(P8_KEY_RIGHT | P8_KEY_LEFT);
    }

    if (jyDir > 0) {
        currKHeld |= P8_KEY_DOWN;
        currKHeld &= ~(P8_KEY_UP);
        if (prevJyDir != jyDir) currKDown |= P8_KEY_DOWN;
    } else if (jyDir < 0) {
        currKHeld |= P8_KEY_UP;
        currKHeld &= ~(P8_KEY_DOWN);
        if (prevJyDir != jyDir) currKDown |= P8_KEY_UP;
    } else if (prevJyDir != 0) {
        currKHeld &= ~(P8_KEY_UP | P8_KEY_DOWN);
        currKDown &= ~(P8_KEY_UP | P8_KEY_DOWN);
    }

    return InputState_t {
        currKDown,
        currKHeld,
        (int16_t)touchLocation.x,
        (int16_t)touchLocation.y,
        mouseBtnState
    };
}

vector<string> Host::listcarts() {
    vector<string> carts;
    DIR *dir;
    struct dirent *ent;

    // Scan primary cart directory (/app0/carts or USB)
    if ((dir = opendir(_cartDirectory.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_name[0] == '.') continue;
            if (isCartFile(ent->d_name)) {
                carts.push_back(_cartDirectory + "/" + ent->d_name);
            }
        }
        closedir(dir);
    }

    // Also scan USB /mnt/usb0/pico8 if not already scanned
    if (_cartDirectory != "/mnt/usb0/pico8" && (dir = opendir("/mnt/usb0/pico8")) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_name[0] == '.') continue;
            if (isCartFile(ent->d_name)) {
                carts.push_back(string("/mnt/usb0/pico8/") + ent->d_name);
            }
        }
        closedir(dir);
    }

    return carts;
}

std::vector<std::string> Host::listdirs() {
    std::vector<std::string> dirs;
    DIR *dir;
    struct dirent *ent;

    if ((dir = opendir(_cartDirectory.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_name[0] == '.') continue;
            if (ent->d_type == DT_DIR) {
                dirs.push_back(ent->d_name);
            }
        }
        closedir(dir);
    }

    return dirs;
}
