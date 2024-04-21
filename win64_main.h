#if !defined(WIN64_MAIN_H)

#include <windows.h>
#include <stdint.h>
//TODO: Complete macro
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0])) 

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

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