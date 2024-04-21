#include "rendering.h"

static void OutputSound(sound_output_buffer *SoundBuffer)
{
  static double tSine;
  int16_t ToneVolume = 2000/2;
  int ToneHz = 256;
  int WavePeriod = SoundBuffer->SamplePerSecond/ToneHz;

  int16_t *SampleOut = SoundBuffer->Samples;
  for(int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
  {
    double SineValue = sinf(tSine);
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
      uint8_t Blue = (X + XOffset + YOffset);
      uint8_t Green = (Y + YOffset + XOffset);
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
    ApplicationState->ToneHz = 256;
    Memory->IsInitialized = true;
  }

  controller_input *Input0 = &Input->Controllers[0];
  if(Input0->IsAnalog)
  {
    //NOTE: Use analog movement
  }
  else
  {
    //NOTE: Use digital movement
  }

  //Input.SpaceBarEndedDown;
  //Input.SpaceBarHalfTransitionCount;
  if(Input0->Down.EndedDown)
  {
    ApplicationState->GreenOffset += 1;
  }

  // Input.StartX;
  // Input.MinX;
  // Input.MaxX;
  // Input.EndX;

  OutputSound(SoundBuffer);
  Render(Buffer, ApplicationState->BlueOffset, ApplicationState->GreenOffset);
}