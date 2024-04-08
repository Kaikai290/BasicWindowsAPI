#if !defined(RENDERING_H)

struct game_offscreen_buffer
{
void *Memory;
int Width;
int Height;
int Pitch;
};

void GameUpdateAndRendering(game_offscreen_buffer *Buffer);

#define RENDERING_H
#endif
