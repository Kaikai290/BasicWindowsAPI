#if !defined(APPLICATION_MAIN_H)
#include <math.h>
#include <stdint.h>

#include "win64_main.h"

//NOTE: Services that the platform layer provides to the game


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
    {   
        button_state Buttons[4];
        struct 
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

struct application_input
{
    // TODO: Insert clock value here.
    controller_input Controllers[4];
    keyboard_input Keyboard[4];
};

struct application_state
{
    int GreenOffset;
    int BlueOffset;
    int ToneHz;
};

struct application_memory
{
    bool IsInitialized;
    uint64_t PermanentStorageSize;
    void *PermanentStorage; //NOTE: Requitred to be cleared to zero on start-up

    uint64_t TransientStorageSize;
    void *TransientStorage; //NOTE: Requitred to be cleared to zero on start-up
};


static void Render(graphics_offscreen_buffer *Buffer, int XOffset, int YOffset);

static void OutputSound(sound_output_buffer *SoundBuffer);

static void UpdateAndRendering(graphics_offscreen_buffer *Buffer, 
                        sound_output_buffer *SoundBuffer, application_input *Input, 
                        application_memory *Memory);

#define APPLICATION_MAIN_H
#endif
