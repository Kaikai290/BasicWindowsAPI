#include <windows.h>
#include <stdint.h>
#include <Xinput.h>
#include <dsound.h>
#include <math.h>
#include <stdio.h>

#include "rendering.cpp"


#define local_persist static
#define internal static
// Day 001 in misc\shell.bat there is no path
// TODO(Kai): This is a global for now

/*
TODO(Kai):
- Saved game location
- Getting a handle to our own executable file
- Asset loading path
- Threading
- Raw Input
- Sleep/timeBeginPeriod
- ClipCursor()
- Fullscreen support
- WM_SUPPORT CURSOR
- QueryCancelAutoplay
- WM_ACTIVATEAPP(when we are not the main application)
- Blit speed improvements (BitBlt)
- Hardware acceleration
- Get keyboard layout
- .....
*/

struct windows_offscreen_buffer
{
BITMAPINFO info;
void *Memory;
int Width;
int Height;
int Pitch;
};

static bool GlobalRunning = true;
static windows_offscreen_buffer GlobalBackBuffer;
static LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;


struct window_dimension
{
    int Width;
    int Height;
};
// NOTE(Kai): This is our support for XInputSet/GetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex,  XINPUT_STATE* pState)
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD  dwUserIndex,  XINPUT_VIBRATION* pVibration)
typedef X_INPUT_GET_STATE(x_input_get_state);
typedef X_INPUT_SET_STATE(x_input_set_state);

X_INPUT_GET_STATE(XInputGetStateStub)
{
  return ERROR_DEVICE_NOT_CONNECTED;
}

X_INPUT_SET_STATE(XInputSetStateStub)
{
  return ERROR_DEVICE_NOT_CONNECTED;
}
static x_input_get_state *XInputGetState_ = XInputGetStateStub;
static x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);



static void LoadXInput()
{
  HMODULE XInputLibary = LoadLibraryA("xinput1_4.dll");
  if(!XInputLibary)
  {
    HMODULE XInputLibary = LoadLibraryA("xinput1_3.dll");
  }
  if(XInputLibary)
  {
    XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibary, "XInputGetState");
    XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibary, "XInputSetState");
  }
}

struct SoundOutput
{
      int SamplePerSecond = 48000;
      uint32_t RunningSampleIndex = 0;
      int ToneHz = 256;
      int WavePeriod = SamplePerSecond/ToneHz;
      int BytesPerSample = sizeof(int16_t)*2;
      int SecondaryBufferSize = SamplePerSecond*BytesPerSample;
      int16_t ToneVolume = 2000/2;
      bool IsSoundPlaying = false;
      int LatencySampleCount = SamplePerSecond/60;
}; 

static void InitDSound(HWND Window, int32_t SamplespPerSecond, int32_t BufferSize)
{
  // NOTE(Kai): Load the Library
  HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

  if(DSoundLibrary)
  {
  // NOTE(Kai): Get a DirectSound object
    direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

    LPDIRECTSOUND DirectSound;
    if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
    {
      WAVEFORMATEX WaveFormat = {};
      WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
      WaveFormat.nChannels = 2;
      WaveFormat.nSamplesPerSec = SamplespPerSecond;
      WaveFormat.wBitsPerSample = 16;
      WaveFormat.nBlockAlign = (WaveFormat.nChannels*WaveFormat.wBitsPerSample) / 8; 
      WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
      WaveFormat.cbSize = 0;
      if(SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
      {
        DSBUFFERDESC BufferDescription = {};
        BufferDescription.dwSize = sizeof(BufferDescription);
        BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

        LPDIRECTSOUNDBUFFER PrimaryBuffer;
        if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
        {
          
          if(SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))
          {
            OutputDebugStringA("Primary Buffer Set\n");
          }
          else
          {
            //NOTE(Kai): Logging
          }
        }
        else
        {
          //NOTE(Kai): Logging
        }
      }
      else
      {
        // NOTE(Kai): Logging
      }
        // NOTE(Kai): "Create" a secondary buffer
        DSBUFFERDESC BufferDescription = {};
        BufferDescription.dwSize = sizeof(BufferDescription);
        BufferDescription.dwFlags = 0;
        BufferDescription.dwBufferBytes = BufferSize;
        BufferDescription.lpwfxFormat = &WaveFormat;

        if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0)))
        {
          OutputDebugStringA("Secondary Buffer Set\n");
        }
        else
        {

        }


        // NOTE(Kai): Play Sounds
    }
    else
    {

    }
    } 
  else
  {
      //TODO(Kai): Logging
  }

  }

static void ClearSoundBuffer(SoundOutput *SoundOutput)
{
  VOID *Region1;
  DWORD Region1Size;
  VOID *Region2;
  DWORD Region2Size;
  if(SUCCEEDED(GlobalSecondaryBuffer->Lock(0, SoundOutput->SecondaryBufferSize, &Region1, &Region1Size, &Region2, &Region2Size, 0)));
    {
      int8_t *DestOut = (int8_t *)Region1;        
      for(DWORD ByteIndex = 0; ByteIndex < Region1Size; ++ByteIndex)
      {
        *DestOut++ = 0;
      }
      DestOut = (int8_t *)Region2;        
      for(DWORD ByteIndex = 0; ByteIndex < Region2Size; ++ByteIndex)
      {
        *DestOut++ = 0;
      }
    }
    GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
}

static void FillSoundBuffer(SoundOutput* SoundOutput, DWORD BytesToLock, DWORD BytesToWrite,
                               sound_output_buffer *SoundBuffer)
{
  VOID *Region1;
  DWORD Region1Size;
  VOID *Region2;
  DWORD Region2Size;
  if(SUCCEEDED(GlobalSecondaryBuffer->Lock(BytesToLock, BytesToWrite, &Region1, &Region1Size, &Region2, &Region2Size, 0)));
  {
    // TODO(Kai); assert that Region1Size/Region2Size is valid
    DWORD Region1SampleCount = Region1Size/SoundOutput->BytesPerSample;
    int16_t *DestOut = (int16_t *)Region1;
    int16_t *SourceSample = SoundBuffer->Samples;
    for(DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
    {
      double t = (2.0f * 3.14f)*(double)SoundOutput->RunningSampleIndex / (double)SoundOutput->WavePeriod;
      double SineValue = sinf(t);
      int16_t SampleValue = (int16_t)(SineValue * SoundOutput->ToneVolume);
      *DestOut++ = SampleValue++;
      *DestOut++ = SampleValue++;
      ++SoundOutput->RunningSampleIndex;
    }
    DWORD Region2SampleCount = Region2Size/SoundOutput->BytesPerSample;
    DestOut = (int16_t *)Region2;
    for(DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
    {
      double t = (2.0f * 3.14f)*(double)SoundOutput->RunningSampleIndex / (double)SoundOutput->WavePeriod;
      double SineValue = sinf(t);
      int16_t SampleValue = (int16_t)(SineValue * SoundOutput->ToneVolume);
      *DestOut++ = SampleValue++;
      *DestOut++ = SampleValue++;
      ++SoundOutput->RunningSampleIndex;
    }
    GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
  }
}

static window_dimension GetWindowDimension(HWND Window)
{
  window_dimension Result;

  RECT ClientRect;
  GetClientRect(Window, &ClientRect);
  Result.Height = ClientRect.bottom - ClientRect.top;
  Result.Width = ClientRect.right - ClientRect.left; 

  return Result;
}



static void ResizeDIBSection(windows_offscreen_buffer *Buffer ,int Width, int Height) // Resizing windows
{
  // TODO(Kai): Bulletproof this
  // Maybe don't free first, free after, then free first if that fails.

  if(Buffer->Memory)
  {
    VirtualFree(Buffer->Memory, 0, MEM_RELEASE); // Can use VirtualProtect for debugging reasons (Uses after free bug)
  }

  Buffer->Height = Height;
  Buffer->Width = Width;
  Buffer->Pitch = Width*4;

  Buffer->info.bmiHeader.biSize = sizeof(Buffer->info.bmiHeader);
  Buffer->info.bmiHeader.biWidth = Buffer->Width;
  Buffer->info.bmiHeader.biHeight = -Buffer->Height;
  Buffer->info.bmiHeader.biPlanes = 1;
  Buffer->info.bmiHeader.biBitCount = 32;
  Buffer->info.bmiHeader.biCompression = BI_RGB;
  Buffer->info.bmiHeader.biSizeImage = 0;
  Buffer->info.bmiHeader.biXPelsPerMeter = 0;
  Buffer->info.bmiHeader.biYPelsPerMeter = 0;
  Buffer->info.bmiHeader.biClrUsed = 0;
  Buffer->info.bmiHeader.biClrImportant = 0;

  int BitmapMemorySize = 4*(Buffer->Width * Buffer->Height); // 4 is the amount of bytes per pixel. 32/8 from the biBitCount
  Buffer->Memory = VirtualAlloc(0 , BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

  //TODO(Kai): Probably want to clear this to black
  }

static void UpdateWins(windows_offscreen_buffer *Buffer, int WindowWidth, int WindowHeight, HDC DeviceContent)
{
  int AspectRatio = 1280/720;
  //TODO(Kai): Aspect Ratio Correction

  WindowWidth = WindowWidth*AspectRatio;
  WindowHeight = WindowHeight*AspectRatio;

  StretchDIBits(DeviceContent,/* X, Y, Width, Height, X, Y, Width, Height,*/0, 0, WindowWidth, WindowHeight, 0, 0, Buffer->Width,
      Buffer->Height, Buffer->Memory, &Buffer->info, DIB_RGB_COLORS, SRCCOPY);

}      

LRESULT MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
  LRESULT Result = 0;

  switch(Message)
  {
    case WM_SIZE:
    {
      
      OutputDebugStringA("WM_SIZE\n");
    } break;
    case WM_DESTROY:
    {
      // TODO(Kai): Handle this as a an error - recreate window?
      GlobalRunning = false;
      OutputDebugStringA("WM_DESTROY\n"); 
    } break;
    case WM_CLOSE:
    {
      // TODO(Kai): Handle this as a message to the user
      GlobalRunning = false;
      OutputDebugStringA("WM_CLOSE\n");
    } break;
    case WM_ACTIVATEAPP:
    {
      OutputDebugStringA("WM_ACTIVATEAPP\n");
    } break;

    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
        uint32_t VKCode = WParam;
        bool WasDown = ((LParam & (1<<30)) !=0);
        bool IsDown = ((LParam & (1<<30)) == 0);
        if(WasDown != IsDown)
        {
          if(VKCode == 'W'){}
          else if(VKCode == 'A'){}
          else if(VKCode == 'S'){}
          else if(VKCode == 'D'){}
          else if(VKCode == 'Q'){}
          else if(VKCode == 'E'){}
          else if(VKCode == VK_UP){}
          else if(VKCode == VK_LEFT){}
          else if(VKCode == VK_DOWN){}
          else if(VKCode == VK_RIGHT){}
          else if(VKCode == VK_ESCAPE)
          {
            OutputDebugStringA("ESCAPE: ");
            if(IsDown)
            {
              OutputDebugStringA("IsDown");
            }
            if(WasDown)
            {
              OutputDebugStringA("WasDown");
            }
            OutputDebugStringA("\n");
          }
          else if(VKCode == VK_SPACE){}
        }
        bool AltKeyWasDown = ((LParam & (1<<29)) != 0);
        if((VKCode == VK_F4) && AltKeyWasDown)
        {
          GlobalRunning = false;
        }
    } break;

    case WM_PAINT:
    {
      OutputDebugStringA("WM_PAINT\n");
      PAINTSTRUCT Paint;
      HDC DeviceContext = BeginPaint(Window, &Paint);
      window_dimension Dimension = GetWindowDimension(Window);
      UpdateWins(&GlobalBackBuffer, Dimension.Width, Dimension.Height, DeviceContext);      
      EndPaint(Window, &Paint);
    } break;

    default:
    {
      Result = DefWindowProc(Window, Message, WParam, LParam);
    }break; 
  }
  return Result;
}

int CALLBACK WinMain(
  HINSTANCE Instance,
  HINSTANCE PrevInstance,
  LPSTR     CmdLine,
  int       ShowCmd
)
{
  LoadXInput();
  WNDCLASSEXA WindowClass = {}; // {} - Init to zero

  ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

  WindowClass.cbSize = sizeof(WNDCLASSEXA); //Set this before calling the GetClassInfoEx function
  WindowClass.style = CS_HREDRAW | CS_VREDRAW; 
  WindowClass.lpfnWndProc = MainWindowCallback;
  WindowClass.hInstance = Instance;
  //WindowClass.hIcon;
  WindowClass.lpszClassName = "Handmade Class";

  LARGE_INTEGER PerformanceCounterFrequencyResult;
  QueryPerformanceFrequency(&PerformanceCounterFrequencyResult);
  int64_t PerformanceCounterFrequency = PerformanceCounterFrequencyResult.QuadPart; 

  if(RegisterClassExA(&WindowClass))
  {
    HWND Window = CreateWindowExA(0, WindowClass.lpszClassName, "Handmade",
     WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT,
      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, Instance, 0);
    if(Window)
    {
      HDC DeviceContext = GetDC(Window);

      // Graphics Test
      int XOffset = 0;
      int YOffset = 0; 

      SoundOutput SoundOutput;
      InitDSound(Window, SoundOutput.SamplePerSecond, SoundOutput.SecondaryBufferSize);
      ClearSoundBuffer(&SoundOutput);
      GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

      LARGE_INTEGER LastCounter;
      QueryPerformanceCounter(&LastCounter);
      int64_t LastCycleCount = __rdtsc();

      while(GlobalRunning)
      {   
        
        MSG Message;
        while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
        {
          if(Message.message == WM_QUIT)
          {
            GlobalRunning = false;
          }

          TranslateMessage(&Message);
          DispatchMessage(&Message);
        }

        // TODO(Kai): Should we poll this more frequently 
        DWORD dwResult;    
        for (DWORD ControllerIndex=0; ControllerIndex< XUSER_MAX_COUNT; ++ControllerIndex )
        {
          XINPUT_STATE ControllerState;
          dwResult = XInputGetState(ControllerIndex, &ControllerState);
          if(dwResult == ERROR_SUCCESS)
          {
            // Note: Controller is connected
            // TODO(Kai): See if ControllerState.dwPacketNumber increments too rapidly
            XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

            bool DPadUp = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
            bool DPadDown = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
            bool DPadLeft = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
            bool DPadRight = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
            bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
            bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
            bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
            bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
            bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
            bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
            bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
            bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

            int16_t StickX = Pad->sThumbLX; 
            int16_t StickY = Pad->sThumbLY;
          }
          else
          {
            // Note: The Controller is not connected
          }
        }

        DWORD PlayCursor;
        DWORD WriteCursor;
        DWORD BytesToLock;
        DWORD TargetCursor;
        DWORD BytesToWrite;;
        bool SoundIsValid = false;
        if(SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
        {
          SoundIsValid = true;
          BytesToLock = (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample)
                                 % SoundOutput.SecondaryBufferSize;
          TargetCursor = (PlayCursor + (SoundOutput.LatencySampleCount*
                                  SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize);         
          // TODO(Kai): Change this to have a lower latency offset for more accurate sound
          // when we have implimented sound effects.
          if(BytesToLock == TargetCursor)
          {
            if(SoundOutput.IsSoundPlaying)
            {
              BytesToWrite = 0;
            }
            else
            {
                BytesToWrite = SoundOutput.SecondaryBufferSize;
            }
          }
          else if(BytesToLock > TargetCursor)
          {
            BytesToWrite = SoundOutput.SecondaryBufferSize - BytesToLock;
            BytesToWrite += PlayCursor;
          }
          else
          {
            BytesToWrite = TargetCursor - BytesToLock;
          }
        }
        else
        {
          //TODO(Kai): Logging
        }

        // Note: This is bad remove when possible
        int16_t Samples[48000 * 2];
        sound_output_buffer SoundBuffer = {};
        SoundBuffer.SamplePerSecond = SoundOutput.SamplePerSecond;
        SoundBuffer.SampleCount = BytesToWrite/SoundOutput.BytesPerSample;
        SoundBuffer.Samples = Samples;

        offscreen_buffer rBuffer = {};
        rBuffer.Memory = GlobalBackBuffer.Memory;
        rBuffer.Width = GlobalBackBuffer.Width;
        rBuffer.Height = GlobalBackBuffer.Height;
        rBuffer.Pitch = GlobalBackBuffer.Pitch;

        UpdateAndRendering(&rBuffer, &SoundBuffer);

        if(SoundIsValid)
        {
          FillSoundBuffer(&SoundOutput, BytesToLock, BytesToWrite, &SoundBuffer);
        }
        
        
        window_dimension Dimension = GetWindowDimension(Window);
        UpdateWins(&GlobalBackBuffer, Dimension.Width, Dimension.Height, DeviceContext);  
        ReleaseDC(Window, DeviceContext);





        int64_t EndCycleCount = __rdtsc();

        LARGE_INTEGER EndCounter;
        QueryPerformanceCounter(&EndCounter);

        int64_t CycleElasped = EndCycleCount - LastCycleCount;
        int64_t CounterElasped = EndCounter.QuadPart - LastCounter.QuadPart;
        int32_t MSperFrame = (int32_t)(1000* CounterElasped) / PerformanceCounterFrequency;
        int32_t FPS = PerformanceCounterFrequency / CounterElasped;
        int32_t MCPF = (int32_t)(CycleElasped/(1000*1000));

        char Buffer[256];
        sprintf(Buffer, "%dms, %dfps, %dmc/f\n", MSperFrame, FPS, MCPF);
        OutputDebugStringA(Buffer);

        LastCounter = EndCounter;
        LastCycleCount = EndCycleCount;
      }
    }
    else
    {
      //TODO(Kai): Logging
    }
  }
  
  return 0;
}
