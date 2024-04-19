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

struct button_state
{
    int HalfTransitionCount;
    bool EndedDown;
};

struct controller_input
{
    bool IsAnalog;

    float StartX;
    float StartY;

    float MinX;
    float MinY;
    
    float MaxX;
    float MaxY;

    float EndX;
    float EndY;
    
    union
    {   struct
        {
            button_state Up;
            button_state Left;
            button_state Down;
            button_state Right;
        };
    };
};

struct keyboard_input
{
    bool IsAnalog = false;
    button_state SpaceBar;
    button_state W;
    button_state A;
    button_state S;
    button_state D;
};

struct app_input
{
    controller_input Controllers[4];
    keyboard_input Keyboard[4];
};

static void Render(graphics_offscreen_buffer *Buffer, int XOffset, int YOffset);

static void OutputSound(sound_output_buffer *SoundBuffer);

void UpdateAndRendering(graphics_offscreen_buffer *Buffer, sound_output_buffer *SoundBuffer, app_input *Input);

#define RENDERING_H
#endif
