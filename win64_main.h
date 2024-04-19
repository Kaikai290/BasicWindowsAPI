#if !defined(WIN64_MAIN_H)

#include <windows.h>
#include <stdint.h>

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

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