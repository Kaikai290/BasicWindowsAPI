#include "rendering.h"

static void Render(game_offscreen_buffer *Buffer, int XOffset, int YOffset)
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

void GameUpdateAndRendering(game_offscreen_buffer *Buffer)
{
    int BlueOffset = 0;
    int GreenOffset = 0;
    Render(Buffer, BlueOffset, GreenOffset);
}