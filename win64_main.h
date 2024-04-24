#if !defined(WIN64_MAIN_H)

#include <windows.h>
#include <stdint.h>
#include <Xinput.h>
#include <dsound.h>
#include <math.h>
#include <stdio.h>
#include <malloc.h>
//TODO: Complete macro
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0])) 

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

/*
DEBUG = 1 means in debug mode
DEBUG = 0 means not debug mode
*/


#define DEBUG 1

#if DEBUG
// NOTE: These are not for commercial use as they are blocking and the write does not 
// protect against lost data
struct debug_read_file_result
{
    uint32_t ContentsSize;
    void *Contents;
};
static debug_read_file_result DEBUGPlatformReadEntireFile(char *Filename);
static void DEBUGPlatformFreeFileMemory(void *Memory);
static bool DEBUGPlatformWriteEntireFile(char *Filename, uint64_t MemorySize, void *Memory);
#else
// void *DEBUGPlatformReadEntireFile(char *Filename);
// void *DEBUGPlayformFreeFileMemory(void *BitmapMemory);
#endif


#if DEBUG
#define Assert(Expression) \
    if(!(Expression)){*(int *)0 = 0;} //Crash the code if it is not true
#else
#define Assert(Expression) 
#endif

struct windows_offscreen_buffer
{
BITMAPINFO info;
void *Memory;
int Width;
int Height;
int Pitch;
};

struct window_dimension
{
    int Width;
    int Height;
};

struct SoundOutput
{
      int SamplePerSecond = 48000;
      uint32_t RunningSampleIndex = 0;
      int BytesPerSample = sizeof(int16_t)*2;
      int SecondaryBufferSize = SamplePerSecond*BytesPerSample;
      bool IsSoundPlaying = false;
      int LatencySampleCount = SamplePerSecond/60;
}; 
#define WIN64_MAIN_H
#endif