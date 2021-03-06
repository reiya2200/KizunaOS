#pragma once

#include "efi.h"
#include "graphics.h"
#include "stdfunc.h"

extern char g_chara;
extern int check;

typedef struct {
    FrameBuffer frame_buffer;
} BootStruct;

extern "C" void kernel_start(BootStruct* BootStruct);
void init_kernel();
void kernel(FrameBuffer *FrameBuffer);