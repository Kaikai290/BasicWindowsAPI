#if !defined(RENDERING_H)
#include <math.h>
#include <stdint.h>

struct graphics_offscreen_buffer
{
void *Memory;
int Width;
int Height;
int Pitch;
};

struct sound_output_buffer
{
    int16_t *Samples;
    int SampleCount;
    int SamplePerSecond;
};
static void Render(graphics_offscreen_buffer *Buffer, int XOffset, int YOffset);

static void OutputSound(sound_output_buffer *SoundBuffer);

void UpdateAndRendering(graphics_offscreen_buffer *Buffer, sound_output_buffer *SoundBuffer);

#define RENDERING_H
#endif
