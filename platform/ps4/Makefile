#---------------------------------------------------------------------------------
# Orbis8 - PICO-8 Fantasy Console Player for PlayStation 4
# OpenOrbis Toolchain Makefile
#---------------------------------------------------------------------------------

ifndef OPENORBIS
    ifdef OO_PS4_TOOLCHAIN
        OPENORBIS := $(OO_PS4_TOOLCHAIN)
    else ifneq ("$(wildcard $(HOME)/toolchains/OpenOrbis/PS4Toolchain)","")
        OPENORBIS := $(HOME)/toolchains/OpenOrbis/PS4Toolchain
    else
        OPENORBIS := /opt/OpenOrbis/PS4-Toolchain
    endif
endif

APP_TITLE       := Orbis8 - PICO-8 Player
APP_TITLE_ID    := ORBS00008
APP_VERSION     := 1.00
APP_AUTHOR      := birth1mark

TARGET          := orbis8
BUILD           := build
OUTPUT          := $(TARGET).elf
EBOOT           := eboot.bin

ifneq ("$(wildcard $(HOME)/toolchains/bin/clang)","")
    CC          := $(HOME)/toolchains/bin/clang
    CXX         := $(HOME)/toolchains/bin/clang++
    LD          := $(HOME)/toolchains/bin/ld.lld
else
    CC          := clang
    CXX         := clang++
    LD          := ld.lld
endif

COMMON_DIR      := ../SDL2Common/source
SRC_DIR         := ../../source
PLATFORM_DIR    := source
Z8LUA_DIR       := ../../libs/z8lua
MINIZ_DIR       := ../../libs/miniz
LODEPNG_DIR     := ../../libs/lodepng
SIMPLEINI_DIR   := ../../libs/simpleini

# Include directories & sysroot
SYSROOT_FLAGS   := -isysroot $(OPENORBIS) \
                   -isystem $(OPENORBIS)/include/c++/v1 \
                   -isystem $(OPENORBIS)/include \
                   -include stdlib.h

INCLUDES        := -I$(OPENORBIS)/include \
                   -I$(OPENORBIS)/include/SDL2 \
                   -I$(SRC_DIR) \
                   -I$(COMMON_DIR) \
                   -I$(PLATFORM_DIR) \
                   -I$(Z8LUA_DIR) \
                   -I$(MINIZ_DIR) \
                   -I$(LODEPNG_DIR) \
                   -I$(SIMPLEINI_DIR)

# Compiler flags for PS4 (x86_64 FreeBSD 12 ELF)
CFLAGS          := --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables \
                   -O2 -Wall -Wno-unused-variable -Wno-unused-function \
                   -Wno-deprecated \
                   -D__ORBIS__ -D__PS4__ -DVER_STR=\"$(APP_VERSION)\" \
                   $(SYSROOT_FLAGS) $(INCLUDES)

CXXFLAGS        := $(CFLAGS) -std=gnu++17 -fno-rtti -fexceptions

# Linker flags
LDFLAGS         := -m elf_x86_64 -pie --script $(OPENORBIS)/link.x \
                   --eh-frame-hdr -L$(OPENORBIS)/lib

LIBS            := $(OPENORBIS)/lib/crt1.o \
                   -lc -lkernel -lc++ \
                   -lSceUserService \
                   -lSceVideoOut -lScePad -lSceAudioOut -lSceSysmodule \
                   -lSceNet -lSceSsl -lSceHttp -lSDL2

# Collect all source files
SOURCES_C       := $(wildcard $(Z8LUA_DIR)/*.c) \
                   $(wildcard $(MINIZ_DIR)/*.c)

SOURCES_CPP     := $(wildcard $(SRC_DIR)/*.cpp) \
                   $(wildcard $(COMMON_DIR)/*.cpp) \
                   $(wildcard $(PLATFORM_DIR)/*.cpp) \
                   $(wildcard $(LODEPNG_DIR)/*.cpp)

OBJS_C          := $(patsubst %.c, $(BUILD)/%.o, $(notdir $(SOURCES_C)))
OBJS_CPP        := $(patsubst %.cpp, $(BUILD)/%.o, $(notdir $(SOURCES_CPP)))
OBJS            := $(OBJS_C) $(OBJS_CPP)

VPATH           := $(SRC_DIR):$(COMMON_DIR):$(PLATFORM_DIR):$(Z8LUA_DIR):$(MINIZ_DIR):$(LODEPNG_DIR)

.PHONY: all clean pkg

all: $(BUILD) $(EBOOT)

$(BUILD):
	@mkdir -p $(BUILD)

# Z8Lua and libs require C++ features (fix32.h, cstdint)
$(BUILD)/%.o: %.c
	@echo "  CXX   $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/%.o: %.cpp
	@echo "  CXX   $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET).elf: $(OBJS)
	@echo "  LD    $@"
	@$(LD) $(LDFLAGS) $(OBJS) $(LIBS) -o $@

$(EBOOT): $(TARGET).elf
	@echo "  CREATE-EBOOT  $@"
	@OO_PS4_TOOLCHAIN=$(OPENORBIS) $(OPENORBIS)/bin/linux/create-fself -in=$(TARGET).elf -out=$(TARGET).oelf --eboot $(EBOOT) --paid 0x3800000000000011
	@mkdir -p ../../pkg
	@cp $(EBOOT) ../../pkg/eboot.bin
	@echo "==> PS4 Build finished: $(EBOOT)"

pkg: all
	@echo "==> Packaging Orbis8 Fake PKG..."
	@mkdir -p ../../pkg/sce_sys ../../pkg/carts
	@cp -r ../../starter_carts/* ../../pkg/carts/ 2>/dev/null || true
	@test -f ../../pkg/sce_sys/icon0.png || (which magick >/dev/null 2>&1 && magick ../vita/sce_sys/icon0.png -resize 512x512 ../../pkg/sce_sys/icon0.png) || cp ../vita/sce_sys/icon0.png ../../pkg/sce_sys/icon0.png
	@cd ../.. && python3 tools/make_gp4.py pkg
	@cd ../../pkg && DOTNET_SYSTEM_GLOBALIZATION_INVARIANT=1 LD_LIBRARY_PATH=$(HOME)/toolchains/lib:$$LD_LIBRARY_PATH \
		$(OPENORBIS)/bin/linux/PkgTool.Core pkg_build project.gp4 . && \
		mv IV0000-ORBS00008_00-0000000000000000.pkg ../$(TARGET).pkg
	@echo "==> Sealed Fake PKG ready: $(TARGET).pkg"

clean:
	@rm -rf $(BUILD) $(TARGET).elf $(TARGET).oelf $(EBOOT) ../../pkg/eboot.bin ../../$(TARGET).pkg
