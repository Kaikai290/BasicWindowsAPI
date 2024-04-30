#include "application_main.h"

static void OutputSound(sound_output_buffer *SoundBuffer, int ToneHz)
{
  static double tSine;
  int16_t ToneVolume = 2000/2;
  int WavePeriod = SoundBuffer->SamplePerSecond/ToneHz;

  int16_t *SampleOut = SoundBuffer->Samples;
  for(int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
  {
    double SineValue = sinf((float)tSine);
    int16_t SampleValue = (int16_t)(SineValue * ToneVolume);
    *SampleOut++ = SampleValue;
    *SampleOut++ = SampleValue;

    tSine += 2.0f*3.14f*1.0f/(double)WavePeriod;
  }
}

static void Render(graphics_offscreen_buffer *Buffer, int XOffset, int YOffset)
{
  // TODO(Kai): Let's see what the optimizer does
  uint8_t *Row = (uint8_t *)Buffer->Memory;
  for(int Y = 0; Y < Buffer->Height; ++Y)
  {
    uint32_t *Pixel = (uint32_t *)Row;
    for (int X = 0; X < Buffer->Width; ++X)
    {
      uint8_t Blue = (uint8_t)(X + XOffset);
      uint8_t Green = (uint8_t)(Y + YOffset);
      *Pixel++ = ((Green << 8) | Blue);
    }
    Row += Buffer->Pitch;
  }
}

static void UpdateAndRendering(graphics_offscreen_buffer *Buffer, sound_output_buffer *SoundBuffer, application_input *Input, application_memory *Memory)
{
  Assert(sizeof(application_state) <= Memory->PermanentStorageSize);
  // TODO(Kai): deal with the case when the sound is not directly after the preious sound
  //Allow sample offsets
  application_state *ApplicationState = (application_state *)Memory->PermanentStorage;
  if(!Memory->IsInitialized)
  {
    char *Filename = __FILE__;

    debug_read_file_result File = DEBUGPlatformReadEntireFile(Filename);
    if(File.Contents)
    {
      DEBUGPlatformWriteEntireFile("test.out", File.ContentsSize, File.Contents);
      DEBUGPlatformFreeFileMemory(File.Contents);
    }
    ApplicationState->ToneHz = 256;
    Memory->IsInitialized = true;
  }

  for(int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ControllerIndex++) 
  {
    controller_input *Controller = GetController(Input, ControllerIndex);
    if(Controller->IsAnalog)
    {
      //NOTE: Use analog movement
    }
    else
    {
      if(Controller->MoveLeft.EndedDown)
      {
        ApplicationState->BlueOffset -= 1;
      }
      if(Controller->MoveRight.EndedDown)
      {
        ApplicationState->BlueOffset += 1;
      }
    }

    //Input.SpaceBarEndedDown;
    //Input.SpaceBarHalfTransitionCount;
    // if(Controller->MoveDown.EndedDown)
    // {
    //   ApplicationState->GreenOffset += 1;
    // }
    // if(Controller->MoveRight.EndedDown)
    // {
    //   ApplicationState->BlueOffset -= 1;
    // }

    // Input.StartX;
    // Input.MinX;
    // Input.MaxX;
    // Input.EndX;
  }
  OutputSound(SoundBuffer, ApplicationState->ToneHz);
  Render(Buffer, ApplicationState->BlueOffset, ApplicationState->GreenOffset);
}