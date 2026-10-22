#---------------------------------------------------------------------------------
# Orbis8 - PICO-8 Fantasy Console Player for PlayStation 4
# OpenOrbis Toolchain Makefile
#---------------------------------------------------------------------------------

ifndef OPENORBIS
    OPENORBIS := /opt/OpenOrbis/PS4-Toolchain
endif

APP_TITLE       := Orbis8 - PICO-8 Player
APP_TITLE_ID    := ORBS00008
APP_VERSION     := 1.00
APP_AUTHOR      := birth1mark

TARGET          := orbis8
BUILD           := build
OUTPUT          := $(TARGET).elf
EBOOT           := eboot.bin

CC              := clang
CXX             := clang++
LD              := ld.lld

COMMON_DIR      := ../SDL2Common/source
SRC_DIR         := ../../source
PLATFORM_DIR    := source
Z8LUA_DIR       := ../../libs/z8lua
MINIZ_DIR       := ../../libs/miniz
LODEPNG_DIR     := ../../libs/lodepng
SIMPLEINI_DIR   := ../../libs/simpleini

# Include directories
INCLUDES        := -I$(OPENORBIS)/include \
                   -I$(OPENORBIS)/include/SDL2 \
                   -I$(SRC_DIR) \
                   -I$(COMMON_DIR) \
                   -I$(PLATFORM_DIR) \
                   -I$(Z8LUA_DIR) \
                   -I$(MINIZ_DIR) \
                   -I$(LODEPNG_DIR) \
                   -I$(SIMPLEINI_DIR)

# Compiler flags for PS4 (x86_64 FreeBSD ELF)
CFLAGS          := -target x86_64-pc-freebsd-elf -fPIC -funwind-tables \
                   -O2 -Wall -Wno-unused-variable -Wno-unused-function \
                   -D__ORBIS__ -D__PS4__ -DVER_STR=\"$(APP_VERSION)\" \
                   $(INCLUDES)

CXXFLAGS        := $(CFLAGS) -std=gnu++17 -fno-rtti -fno-exceptions

# Linker flags
LDFLAGS         := -m elf_x86_64 -pie --script $(OPENORBIS)/link.x \
                   --eh-frame-hdr -L$(OPENORBIS)/lib

LIBS            := -lkernel -lc -lSceVideoOut -lScePad -lSceAudioOut \
                   -lSceSysmodule -lSceNet -lSceHttp -lSDL2 -lcurl -lz

# Collect all source files
SOURCES_C       := $(wildcard $(Z8LUA_DIR)/*.c) \
                   $(wildcard $(MINIZ_DIR)/*.c) \
                   $(wildcard $(LODEPNG_DIR)/*.c)

SOURCES_CPP     := $(wildcard $(SRC_DIR)/*.cpp) \
                   $(wildcard $(COMMON_DIR)/*.cpp) \
                   $(wildcard $(PLATFORM_DIR)/*.cpp)

OBJS_C          := $(patsubst %.c, $(BUILD)/%.o, $(notdir $(SOURCES_C)))
OBJS_CPP        := $(patsubst %.cpp, $(BUILD)/%.o, $(notdir $(SOURCES_CPP)))
OBJS            := $(OBJS_C) $(OBJS_CPP)

VPATH           := $(SRC_DIR):$(COMMON_DIR):$(PLATFORM_DIR):$(Z8LUA_DIR):$(MINIZ_DIR):$(LODEPNG_DIR)

.PHONY: all clean pkg

all: $(BUILD) $(EBOOT)

$(BUILD):
	@mkdir -p $(BUILD)

$(BUILD)/%.o: %.c
	@echo "  CC    $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.cpp
	@echo "  CXX   $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET).oelf: $(OBJS)
	@echo "  LD    $@"
	@$(LD) $(LDFLAGS) $(OBJS) $(LIBS) -o $@

$(EBOOT): $(TARGET).oelf
	@echo "  CREATE-EBOOT  $@"
	@$(OPENORBIS)/bin/linux/create-eboot -i $(TARGET).oelf --paid 0x3800000000000011 $(EBOOT)
	@cp $(EBOOT) ../../pkg/eboot.bin
	@echo "==> Build finished: $(EBOOT)"

pkg: all
	@echo "==> Packaging Orbis8 Fake PKG..."
	@mkdir -p ../../pkg/sce_sys ../../pkg/carts
	@cp -r ../../starter_carts/* ../../pkg/carts/ 2>/dev/null || true
	@which orbis-pub-cmd >/dev/null 2>&1 && orbis-pub-cmd img_create ../../pkg/project.gp4 ../../$(TARGET).pkg || \
	 echo "Note: Use orbis-pub-cmd or python pkg-builder to seal the final .pkg file."

clean:
	@rm -rf $(BUILD) $(TARGET).oelf $(EBOOT) ../../pkg/eboot.bin
