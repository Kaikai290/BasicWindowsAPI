#include "rendering.h"



static void Render(offscreen_buffer *Buffer, int XOffset, int YOffset)
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

static void OutputSound(sound_output_buffer SoundBuffer)
{
  uint32_t RunningSampleIndex = 0;
  int ToneHz = 256;
  int WavePeriod = SoundBuffer.SamplePerSecond/ToneHz;
  int16_t ToneVolume = 2000/2;

  int16_t *SampleOut = SoundBuffer.Samples;

  for(int SampleIndex = 0; SampleIndex < SoundBuffer.SampleCount; ++SampleIndex)
  {
    float t = (2.0f * 3.14f)*(float)RunningSampleIndex / (float)WavePeriod;
    float SineValue = sinf(t);
    *SampleOut++ = (int16_t)(SineValue * ToneVolume);
    *SampleOut++ = (int16_t)(SineValue * ToneVolume); 
  }
}

void UpdateAndRendering(offscreen_buffer *Buffer, sound_output_buffer *SoundBuffer)
{
    // TODO(Kai): deal with the case when the sound is not directly after the preious sound
    //Allow sample offsets
    OutputSound(*SoundBuffer);
    int BlueOffset = 0;
    int GreenOffset = 0;
    Render(Buffer, BlueOffset, GreenOffset);
}