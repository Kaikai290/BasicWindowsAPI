#include "win64_main.h"
#include "application_main.cpp"

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


static bool GlobalRunning = true;
static windows_offscreen_buffer GlobalBackBuffer;
static LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
static int64_t GlobalPerformanceCounterFrequency; 




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
    XInputLibary = LoadLibraryA("xinput1_3.dll");
  }
  if(XInputLibary)
  {
    XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibary, "XInputGetState");
    XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibary, "XInputSetState");
  }
}

static debug_read_file_result DEBUGPlatformReadEntireFile(char *Filename)
{
  debug_read_file_result Result = {};

  HANDLE FileHandle =  CreateFileA(Filename,
  GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
  if(FileHandle != INVALID_HANDLE_VALUE)
  {
    LARGE_INTEGER FileSize;
    if(GetFileSizeEx(FileHandle, &FileSize))
    {
      //TODO: Define for maximun values UInt32Max
      Assert(FileSize.QuadPart < 0xFFFFFFFF)
      uint32_t FileSize32 = (uint32_t)FileSize.QuadPart;
      Result.Contents = VirtualAlloc(0, FileSize.QuadPart, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
      if(Result.Contents)
      {
        DWORD BytesRead;
        if(ReadFile(FileHandle, Result.Contents, (uint32_t)FileSize.QuadPart, &BytesRead, 0) &&
        (FileSize32 == BytesRead))
        {
          //NOTE:File read successfully
          Result.ContentsSize = FileSize32;
        }
        else
        {
          //TODO: Logging
          DEBUGPlatformFreeFileMemory(Result.Contents);
          Result.Contents = 0;
        }
      }
      else
      {
        //TODO: Logging if VirtualAlloc Fails
      }
    }
    else
    {
      //TODO: Logging if We couldnt get file size
    }
    CloseHandle(FileHandle);
  }
  else
  {
    //TODO: Logging
  }
  return Result;
}

static void DEBUGPlatformFreeFileMemory(void *Memory)
{
  if(Memory)
  {
    VirtualFree(Memory, 0, MEM_RELEASE);
  }
}

static bool DEBUGPlatformWriteEntireFile(char *Filename, uint64_t MemorySize, void *Memory)
{
  bool Result = false;
  HANDLE FileHandle =  CreateFileA(Filename,
  GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
  if(FileHandle != INVALID_HANDLE_VALUE)
  {

    DWORD BytesWritten;
    if(WriteFile(FileHandle, Memory, (uint32_t)MemorySize, &BytesWritten, 0))
    {
      //NOTE:File read successfully
      Result = (BytesWritten == MemorySize);
    }
    else
    {
      //TODO: Logging
    }
    CloseHandle(FileHandle);
  }
  else
  {
    //TODO: Logging
  }
  return Result;
}

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
  if(SUCCEEDED(GlobalSecondaryBuffer->Lock(0, SoundOutput->SecondaryBufferSize, &Region1, &Region1Size, &Region2, &Region2Size, 0)))
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
  if(SUCCEEDED(GlobalSecondaryBuffer->Lock(BytesToLock, BytesToWrite, &Region1, &Region1Size, &Region2, &Region2Size, 0)))
  {
    // TODO(Kai); assert that Region1Size/Region2Size is valid
    DWORD Region1SampleCount = Region1Size/SoundOutput->BytesPerSample;
    int16_t *DestOut = (int16_t *)Region1;
    int16_t *SourceSample = SoundBuffer->Samples;
    for(DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
    {
      *DestOut++ = *SourceSample++;
      *DestOut++ = *SourceSample++;
      ++SoundOutput->RunningSampleIndex;
    }
    DWORD Region2SampleCount = Region2Size/SoundOutput->BytesPerSample;
    DestOut = (int16_t *)Region2;
    for(DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
    {
      *DestOut++ = *SourceSample++;
      *DestOut++ = *SourceSample++;
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
  int BytesPerPixel = 4;

  //Note: When the biHeight is negative, this is the clue for windows to treat this bitmap as 
  //top-down, meaning the first 3 bytes are the colour of the top-left pixel

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

  int BitmapMemorySize = BytesPerPixel*(Buffer->Width * Buffer->Height); // 4 is the amount of bytes per pixel. 32/8 from the biBitCount
  Buffer->Memory = VirtualAlloc(0 , BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
  Buffer->Pitch = Width * BytesPerPixel;
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
        Assert(!"Keyboard input came in through a non-dispatch message!");
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

static void ProcessXInputDigitalButton(button_state *OldState, button_state *NewState,
DWORD XInputButtonState, DWORD ButtonBit)
{
  NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
  NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}
static void ProcessKeyboardMessage(button_state *NewState, bool IsDown)
{
  Assert(NewState->EndedDown != IsDown);
  NewState->EndedDown = IsDown;
  ++NewState->HalfTransitionCount;
}
static void ProcessPendingMessages(controller_input *KeyboardController)
{
  MSG Message;
  while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
  {
    switch(Message.message)
    {
      case WM_QUIT:
      {
        GlobalRunning = false;
      }break;
      case WM_SYSKEYDOWN:
      case WM_SYSKEYUP:
      case WM_KEYDOWN:
      case WM_KEYUP:
      {
        uint32_t VKCode = (uint32_t)Message.wParam;
        bool WasDown = ((Message.lParam & (1<<30)) !=0);
        bool IsDown = ((Message.lParam & (1<<31)) == 0);
        if(WasDown != IsDown)
        {
          if(VKCode == 'W')
          {
            ProcessKeyboardMessage(&KeyboardController->MoveUp, IsDown);
          }
          else if(VKCode == 'A')
          {
            ProcessKeyboardMessage(&KeyboardController->MoveLeft, IsDown);
          }
          else if(VKCode == 'S')
          {
            ProcessKeyboardMessage(&KeyboardController->MoveDown, IsDown);
          }
          else if(VKCode == 'D')
          {
            ProcessKeyboardMessage(&KeyboardController->MoveRight, IsDown);
          }
          else if(VKCode == 'Q'){}
          else if(VKCode == 'E'){}
          else if(VKCode == VK_UP)
          {
            ProcessKeyboardMessage(&KeyboardController->ActionUp, IsDown);
          }
          else if(VKCode == VK_LEFT)
          {
            ProcessKeyboardMessage(&KeyboardController->ActionLeft, IsDown);
          }
          else if(VKCode == VK_DOWN)
          {
            ProcessKeyboardMessage(&KeyboardController->ActionDown, IsDown);
          }
          else if(VKCode == VK_RIGHT)
          {
            ProcessKeyboardMessage(&KeyboardController->ActionRight, IsDown);
          }
          else if(VKCode == VK_ESCAPE)
          {
            GlobalRunning = false;
          }
          else if(VKCode == VK_SPACE){}
        }
        bool AltKeyWasDown = ((Message.lParam & (1<<29)) != 0);
        if((VKCode == VK_F4) && AltKeyWasDown)
        {
          GlobalRunning = false;
        }
      }break;
      default:
      {
        TranslateMessage(&Message);
        DispatchMessage(&Message);
      }break;
    }
  }

}

static float ProcessXInputStickValue(SHORT Value, SHORT DeadZoneThreshold)
{
  float Result = 0;
  if (Value < -DeadZoneThreshold)
  {
    Result = (float) Value / 32768.0f;
  }
  else if (Value > DeadZoneThreshold)
  {
    Result = (float) Value / 32767.0f;
  }
  return Result;
}


inline LARGE_INTEGER GetWallClock()
{
  LARGE_INTEGER Result;
  QueryPerformanceCounter(&Result);
  return Result;
}

inline float GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
  double Result = (((double)End.QuadPart - Start.QuadPart) / (double)GlobalPerformanceCounterFrequency);
  return (float)Result;
}

int CALLBACK WinMain(
  HINSTANCE Instance,
  HINSTANCE PrevInstance,
  LPSTR     CmdLine,
  int       ShowCmd
)
{
  LARGE_INTEGER PerformanceCounterFrequencyResult;
  QueryPerformanceFrequency(&PerformanceCounterFrequencyResult);
  GlobalPerformanceCounterFrequency = PerformanceCounterFrequencyResult.QuadPart; 

  //NOTE: Set the windows Scheduler granularity to 1ms, so that our sleep can be more granullar
  UINT DesiredScheldulerMS = 1;
  bool SleepIsGranular =  (timeBeginPeriod(DesiredScheldulerMS) == TIMERR_NOERROR);

  LoadXInput();
  WNDCLASSEXA WindowClass = {}; // {} - Init to zero

  ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

  WindowClass.cbSize = sizeof(WNDCLASSEXA); //Set this before calling the GetClassInfoEx function
  WindowClass.style = CS_HREDRAW | CS_VREDRAW; 
  WindowClass.lpfnWndProc = MainWindowCallback;
  WindowClass.hInstance = Instance;
  //WindowClass.hIcon;
  WindowClass.lpszClassName = "Handmade Class";

  //TODO: How do we realiably query this on windows
  int MonitorRefreshHz = 144;
  int GameUpdateHz = MonitorRefreshHz / 2;
  float TargetSecondsPerFrame = 1.0f / (float) GameUpdateHz;

  if(RegisterClassExA(&WindowClass))
  {
    HWND Window = CreateWindowExA(0, WindowClass.lpszClassName, "Handmade",
                                  WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT,
                                  CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, Instance, 0);
    if(Window)
    {
      HDC DeviceContext = GetDC(Window);
      SoundOutput SoundOutput;

      InitDSound(Window, SoundOutput.SamplePerSecond, SoundOutput.SecondaryBufferSize);
      ClearSoundBuffer(&SoundOutput);
      GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
      //TODO: Combine with bitmap VirtualAlloc
      int16_t *Samples = (int16_t *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize,
                                                MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

#if INTERNAL
      LPVOID BaseAddress = (LPVOID)Terabytes(2);
#else
      LPVOID BaseAddress = 0;
#endif

      application_memory Memory = {};
      Memory.PermanentStorageSize = Megabytes(64);
      Memory.TransientStorageSize = Gigabytes(4); // This is for stuff that can be reloaded

      uint64_t TotalSize = Memory.PermanentStorageSize + Memory.TransientStorageSize;
      Memory.PermanentStorage = VirtualAlloc(BaseAddress, TotalSize,
                                                MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
      Memory.TransientStorage = ((uint8_t *)Memory.PermanentStorage + Memory.PermanentStorageSize);

      if(Samples && Memory.PermanentStorage && Memory.TransientStorage)
      {

        application_input Input[2] = {};
        application_input *NewInput = &Input[0];
        application_input *OldInput = &Input[1];

        LARGE_INTEGER LastCounter = GetWallClock();

        int64_t LastCycleCount = __rdtsc();

        while(GlobalRunning)
        {    
          controller_input *OldKeyboardController = GetController(OldInput, 0);
          controller_input *NewKeyboardController = GetController(NewInput, 0);
          controller_input ZeroController = {};
          *NewKeyboardController = ZeroController;
          NewKeyboardController->IsConnected = true;
          for(int ButtonIndex = 0; ButtonIndex < ArrayCount(NewKeyboardController->Buttons); ButtonIndex++)
          {
            NewKeyboardController->Buttons[ButtonIndex].EndedDown = OldKeyboardController->Buttons[ButtonIndex].EndedDown;
          }

          ProcessPendingMessages(NewKeyboardController);

          // TODO: Need to not poll disconnected controllersto avoid framerate hits
          // TODO(Kai): Should we poll this more frequently 
          DWORD MaxControllerCount = XUSER_MAX_COUNT;
          if(MaxControllerCount > (ArrayCount(NewInput->Controllers) - 1))
          {
            MaxControllerCount = (ArrayCount(NewInput->Controllers) - 1);
          }
          //??DWORD dwResult; // Remove From loop?
          for (DWORD ControllerIndex=0; ControllerIndex < MaxControllerCount; ++ControllerIndex )
          {
            DWORD OurControllerIndex = ControllerIndex + 1;
            controller_input *OldController = GetController(OldInput, OurControllerIndex);
            controller_input *NewController = GetController(NewInput, OurControllerIndex);

            XINPUT_STATE ControllerState;
            if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
            {
              NewController->IsConnected = true;
              // Note: Controller is connected
              // TODO(Kai): See if ControllerState.dwPacketNumber increments too rapidly
              XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

              bool DPadUp = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
              bool DPadDown = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
              bool DPadLeft = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
              bool DPadRight = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);

              
              NewController->StickAverageX = ProcessXInputStickValue(Pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
              NewController->StickAverageY = ProcessXInputStickValue(Pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
              if((NewController->StickAverageX != 0.f) || (NewController->StickAverageY !=0.0f))
              {
                NewController->IsAnalog = true;
              }

              if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP)
              {
                NewController->StickAverageY = 1.0f;
                NewController->IsAnalog = false;

              }
              if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
              {
                NewController->StickAverageY = -1.0f;
                NewController->IsAnalog = false;
              }
              if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
              {
                NewController->StickAverageX = 1.0f;
                NewController->IsAnalog = false;
              }
              if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
              {
                NewController->StickAverageX = -1.0f;
                NewController->IsAnalog = false;
              }

              float Threshold = 0.5f;
              ProcessXInputDigitalButton(&OldController->MoveLeft, &NewController->MoveLeft, (NewController->StickAverageX < -Threshold) ? 1 : 0, 1);
              ProcessXInputDigitalButton(&OldController->MoveRight, &NewController->MoveRight, (NewController->StickAverageX > Threshold) ? 1 : 0, 1);
              ProcessXInputDigitalButton(&OldController->MoveDown, &NewController->MoveDown, (NewController->StickAverageY < -Threshold) ? 1 : 0, 1);
              ProcessXInputDigitalButton(&OldController->MoveUp, &NewController->MoveUp, (NewController->StickAverageY > Threshold) ? 1 : 0, 1);
              
              ProcessXInputDigitalButton(&OldController->ActionDown, &NewController->ActionDown,
                      Pad->wButtons, XINPUT_GAMEPAD_A);
              ProcessXInputDigitalButton(&OldController->ActionRight, &NewController->ActionRight,
                      Pad->wButtons, XINPUT_GAMEPAD_B);
              ProcessXInputDigitalButton(&OldController->ActionLeft, &NewController->ActionLeft,
                      Pad->wButtons, XINPUT_GAMEPAD_X);
              ProcessXInputDigitalButton(&OldController->ActionUp, &NewController->ActionUp,
                      Pad->wButtons, XINPUT_GAMEPAD_Y);


              // bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
              // bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
              // bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
              // bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);


              
            }
            else
            {
              // Note: The Controller is not connected
              NewController->IsConnected = false;
            }
          }

          DWORD PlayCursor = 0;
          DWORD WriteCursor = 0;
          DWORD BytesToLock = 0;
          DWORD TargetCursor = 0;
          DWORD BytesToWrite = 0;
          bool SoundIsValid = false;
          //TODO: Tighten up sound logic so that we know where we should
          // be writing to and can anticipate the time spent in the game update
          if(SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
          {
            SoundIsValid = true;
            BytesToLock = (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample)
                                  % SoundOutput.SecondaryBufferSize;
            TargetCursor = ((PlayCursor + (SoundOutput.LatencySampleCount*
                                    SoundOutput.BytesPerSample)) % SoundOutput.SecondaryBufferSize);         
            if(BytesToLock > TargetCursor)
            {
              BytesToWrite = SoundOutput.SecondaryBufferSize - BytesToLock;
              BytesToWrite += TargetCursor;
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
          sound_output_buffer SoundBuffer = {};
          SoundBuffer.SamplePerSecond = SoundOutput.SamplePerSecond;
          SoundBuffer.SampleCount = BytesToWrite/SoundOutput.BytesPerSample;
          SoundBuffer.Samples = Samples;


          graphics_offscreen_buffer gBuffer = {};
          gBuffer.Memory = GlobalBackBuffer.Memory;
          gBuffer.Width = GlobalBackBuffer.Width;
          gBuffer.Height = GlobalBackBuffer.Height;
          gBuffer.Pitch = GlobalBackBuffer.Pitch;

          UpdateAndRendering(&gBuffer, &SoundBuffer, NewInput, &Memory);

          if(SoundIsValid)
          {
            FillSoundBuffer(&SoundOutput, BytesToLock, BytesToWrite, &SoundBuffer);
          }
          

          LARGE_INTEGER WorkCounter = GetWallClock();
          double WorkSecondsElasped = GetSecondsElapsed(LastCounter, WorkCounter);

          double SecondsElaspedForFrame = WorkSecondsElasped;
          if(SecondsElaspedForFrame < TargetSecondsPerFrame)
          {
            if(SleepIsGranular)
            {    
              DWORD SleepMS = (DWORD)((TargetSecondsPerFrame - SecondsElaspedForFrame) * 1000.0f);
              if(SleepMS > 0)
              {
                Sleep(SleepMS);
              }
            }

            float TestSecondsElaspedForFrame = GetSecondsElapsed(LastCounter, GetWallClock());

            //TOOD: Get Assert Working
            //Assert (TestSecondsElaspedForFrame < TargetSecondsPerFrame);
            while(SecondsElaspedForFrame < TargetSecondsPerFrame)
            {
              SecondsElaspedForFrame = GetSecondsElapsed(LastCounter,  GetWallClock());
            }
          }
          else
          {
            //TODO: Missed Frame Rate(Logging)
          }

          window_dimension Dimension = GetWindowDimension(Window);
          UpdateWins(&GlobalBackBuffer, Dimension.Width, Dimension.Height, DeviceContext);  

          application_input *Temp = NewInput;
          NewInput = OldInput;
          OldInput = Temp;

          // TODO: Should these be cleared?
          LARGE_INTEGER EndCounter = GetWallClock();
          double MSperFrame = 1000.0f* GetSecondsElapsed(LastCounter, EndCounter);
          LastCounter = EndCounter;

          uint64_t EndCycleCount = __rdtsc();
          uint64_t CyclesElapsed = EndCycleCount - LastCycleCount;
          LastCycleCount = EndCycleCount;

          double FPS =  0.0f; //(double)(GlobalPerformanceCounterFrequency /  (double)CounterElapsed);
          double MCPF =  (double)( (double)CyclesElapsed/(1000*1000));

          char Buffer[256];
          sprintf_s(Buffer, "%.02fms/f, %0.02ffps, %0.02fmc/f\n", MSperFrame, FPS, MCPF);
          OutputDebugStringA(Buffer);
        }
      }
      else
      {
        //TODO(Kai): Logging
      }
    }
    else
    {
      //TODO(Kai): Logging
    }
  
  

}
  return 0;
}