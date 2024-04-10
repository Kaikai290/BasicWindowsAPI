#if !defined(RENDERING_H)

struct offscreen_buffer
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

void UpdateAndRendering(offscreen_buffer *Buffer, sound_output_buffer *SoundBuffer);

#define RENDERING_H
#endif
