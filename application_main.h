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
    bool IsConnected;
    bool IsAnalog;
    float StickAverageX;
    float StickAverageY;
    
    union
    {   
        button_state Buttons[8];
        struct 
        {
            button_state MoveUp;
            button_state MoveLeft;
            button_state MoveDown;
            button_state MoveRight;        

            button_state ActionUp;
            button_state ActionLeft;
            button_state ActionDown;
            button_state ActionRight;
        };
    };
};


struct application_input
{
    // TODO: Insert clock value here.
    controller_input Controllers[5];
};
inline controller_input *GetController(application_input *Input, int ControllerIndex)
{
    Assert(ControllerIndex < ArrayCount(Input->Controllers));

    controller_input *Result = &Input->Controllers[ControllerIndex];
    return Result;
}

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
