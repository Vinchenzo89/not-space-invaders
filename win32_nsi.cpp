#include <Windows.h>

#include "app.cpp"

struct win32_screen_buffer
{    
    BITMAPINFO BitmapInfo;
    u32 Width = 0;
    u32 Height = 0;
    u8 *Buffer = 0;
};

struct win32_window_dimension
{
    int Width;
    int Height;
};

global win32_screen_buffer GlobalBackBuffer;
global bool GlobalWindowRunning;
global int64_t GlobalPerformanceFrequency;

inline LARGE_INTEGER
Win32GetWallClock()
{
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return(Result);
}

inline win32_window_dimension
Win32GetWindowDimensions(HWND WindowHandle)
{
    win32_window_dimension Result = {};
    RECT Rect;
    GetWindowRect(WindowHandle, &Rect);
    Result.Width = Rect.right - Rect.left;
    Result.Height = Rect.bottom - Rect.top;
    return(Result);
}

inline float
Win32GetSecondsEllapsed(LARGE_INTEGER StartTime, LARGE_INTEGER EndTime)
{ 
    float Result = (float)(EndTime.QuadPart - StartTime.QuadPart)
        / (float)GlobalPerformanceFrequency;
    return(Result);
}

inline void *
Win32AllocateMemory(u64 Size)
{
    void *Result = VirtualAlloc(0, Size, MEM_COMMIT, PAGE_READWRITE);
    return(Result);
}

inline void
Win32ReleaseMemory(void *Data)
{
    VirtualFree(Data, 0, MEM_RELEASE);
}

internal void
Win32ResizeScreenBuffer(u32 Width, u32 Height)
{
    if (GlobalBackBuffer.Buffer) 
    {
        Win32ReleaseMemory(&GlobalBackBuffer.Buffer);
        GlobalBackBuffer.Buffer = 0;
    }

    GlobalBackBuffer.Width = Width;
    GlobalBackBuffer.Height = Height;

    u32 PixelSize = 4;
    u32 BufferSize = Width * Height * PixelSize;
    GlobalBackBuffer.Buffer = (u8 *)Win32AllocateMemory(BufferSize);

    BITMAPINFOHEADER Header = {};
    Header.biSize = sizeof(BITMAPINFOHEADER);
    Header.biWidth = Width;
    Header.biHeight = -Height; // Origin is Top Left
    Header.biPlanes = 1;
    Header.biBitCount = 32;
    Header.biCompression = BI_RGB;

    GlobalBackBuffer.BitmapInfo = {};
    GlobalBackBuffer.BitmapInfo.bmiHeader = Header;
}

static void
RenderSomething(render_commands *RenderCommands)
{
    if (GlobalBackBuffer.Buffer)
    {
        u32 BufferWidth = GlobalBackBuffer.Width;
        u32 BufferHeight = GlobalBackBuffer.Height;

        void *BufferEntry = RenderCommands->PushBuffer;
        for (umi Index = 0; 
             Index < RenderCommands->PushBufferEntryCount; 
             ++Index)
        {
            render_command_header *Header = (render_command_header *)BufferEntry;
            switch (Header->Type)
            {
                case RenderCommand_Rectangle:
                {
                    render_rectangle *Command =
                        (render_rectangle *)((u8 *)Header + sizeof(render_command_header));
                    
                    for (u32 Y = Command->Y;
                         Y < (Command->Y + Command->Height);
                         ++Y)
                    {
                        for (u32 X = Command->X;
                             X < (Command->X + Command->Width);
                             ++X)
                        {
                            u32 Offset = ((X % BufferWidth) + ((Y % BufferHeight) * BufferWidth));
                            u32 *Pixel = ((u32 *)GlobalBackBuffer.Buffer + Offset);
                            *Pixel = Command->Color;
                        }
                    }
                } break;

                case RenderCommand_Clear:
                {
                    render_clear_color *Command =
                        (render_clear_color *)((u8 *)Header + sizeof(render_command_header));

                    for (u32 Index = 0; Index < (BufferWidth*BufferHeight); ++Index)
                    {
                        u32 *Pixel = ((u32 *)GlobalBackBuffer.Buffer + Index);
                        *Pixel = Command->Color;
                    }
                } break;
            }

            BufferEntry = (u8 *)BufferEntry + Header->Size;
        }
    }
}

static void
Win32PollWindowInput(app_input *Input)
{
    // NOTE: (Marcus) Temporary way to handle messages
    MSG Message;
    while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
    {
        switch (Message.message)
        {
            case WM_QUIT:
            {
                GlobalWindowRunning = false;
            } break;

            // key controls
            case WM_KEYUP:
            case WM_KEYDOWN:
            case WM_SYSKEYUP:
            case WM_SYSKEYDOWN:
            {
                // wParam gives key code.
                u32 KeyCode = (u32)Message.wParam;
                #define KeyIsDownBitFlag (1 << 31)
                #define KeyWasDownBitFlag (1 << 30)
                bool IsDown  = (KeyIsDownBitFlag  & Message.lParam) == 0;
                bool WasDown = (KeyWasDownBitFlag & Message.lParam) != 0;
                Input->KeyState[KeyCode].IsDown = IsDown;
                Input->OldKeyState[KeyCode].IsDown = WasDown;

                // char Text[256];
                // wsprintf(Text, "Key %d IsDown %d WasDown %d \n", KeyCode, IsDown, WasDown);
                // OutputDebugStringA(Text);
            } break;
        }

        TranslateMessage(&Message);
        DispatchMessage(&Message);
    }
}

internal void
Win32BlitImageToScreen(HDC DeviceContext, int ScreenWidth, int ScreenHeight, win32_screen_buffer Buffer)
{
    StretchDIBits(
        DeviceContext,
        0, 0, ScreenWidth, ScreenHeight,
        0, 0, Buffer.Width, Buffer.Height,
        Buffer.Buffer,
        &Buffer.BitmapInfo,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}

LRESULT CALLBACK
Win32WindowCallback(HWND WindowHandle, 
                    UINT Message, 
                    WPARAM WParam, 
                    LPARAM LParam)
{
    LRESULT Result = 0;

    switch(Message)
    {
        case WM_SIZE:
        {
            RECT Rect;
            GetWindowRect(WindowHandle, &Rect);
            int Height = Rect.bottom - Rect.top;
            int Width = Rect.right - Rect.left;
            Win32ResizeScreenBuffer(Width, Height);
        } 
        break;

        case WM_DESTROY:
        {
            GlobalWindowRunning = false;
        } 
        break;

        case WM_CLOSE:
        {
            GlobalWindowRunning = false;
        } 
        break;

        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("Window Activate App Message\n");
        } 
        break;

        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(WindowHandle, &Paint);
            win32_window_dimension Dimension = Win32GetWindowDimensions(WindowHandle);
            Win32BlitImageToScreen(DeviceContext, Dimension.Width, Dimension.Height,
                                    GlobalBackBuffer);
            EndPaint(WindowHandle, &Paint);
        }
        break;

        default:
        {
            Result = DefWindowProc(WindowHandle, Message, WParam, LParam);
        } break;
    }

    return(Result);
}

int CALLBACK 
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
{
    LARGE_INTEGER PerformanceCounterFrequency;
    QueryPerformanceFrequency(&PerformanceCounterFrequency);
    GlobalPerformanceFrequency = PerformanceCounterFrequency.QuadPart;

    // NOTE: (Marcus) Set Window Schedular to 1ms granularity
    // so that Sleep() can be more granular
    bool SleepIsGranular = (timeBeginPeriod(1) == TIMERR_NOERROR);

    WNDCLASS WindowClass = {};
    // TODO: Casey says this may not be necessary
    WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32WindowCallback;
    WindowClass.hInstance = Instance;
    // WindowClass.hIcon;
    WindowClass.lpszClassName = "NsiWindowClass";

    if (RegisterClass(&WindowClass))
    {
        HWND WindowHandle = 
            CreateWindowEx(
                0,
                WindowClass.lpszClassName,
                APP_NAME, 
                WS_OVERLAPPEDWINDOW|WS_VISIBLE, 
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                0,
                0,
                Instance, 
                0);
        
        if (WindowHandle)
        {
            float DesiredFPS = 60;
            float DesiredSecsPerFrame = (1.0f / DesiredFPS);
            float DesiredMsPerFrame = (DesiredSecsPerFrame * 1000);

            app_memory Memory = {};
            Memory.PerminantStorageSize = 1000000;
            Memory.TransientStorageSize = 256000;
            u64 TotalMemory = (Memory.PerminantStorageSize + Memory.TransientStorageSize);
            Memory.PerminantStorage = Win32AllocateMemory(TotalMemory);
            Memory.TransientStorage = ((u8 *)Memory.PerminantStorage + Memory.PerminantStorageSize);
            
            u64 PushBufferSize = 1000000;
            void *PushBuffer = Win32AllocateMemory(PushBufferSize);

            app_input Input = {};
            LARGE_INTEGER LastCounter = {};
            float SecondsEllapsedForFrame = DesiredSecsPerFrame;
            GlobalWindowRunning = true;
            while (GlobalWindowRunning)
            {
                LARGE_INTEGER WorkCounterStart, WorkCounterEnd;
                WorkCounterStart = Win32GetWallClock();

                HDC DeviceContext = GetDC(WindowHandle);
                win32_window_dimension Dim = Win32GetWindowDimensions(WindowHandle);
                Input.ScreenWidth = Dim.Width;
                Input.ScreenHeight = Dim.Height;
                Input.FrameEllapsedSecs = SecondsEllapsedForFrame;
                render_commands RenderCommands = CreateRenderCommands(PushBufferSize, PushBuffer);

                Win32PollWindowInput(&Input);
                GameUpdateAndRender(Input, &Memory, &RenderCommands);
                RenderSomething(&RenderCommands);

                Win32BlitImageToScreen(DeviceContext, Dim.Width, Dim.Height, GlobalBackBuffer);
                ReleaseDC(WindowHandle, DeviceContext);

                WorkCounterEnd = Win32GetWallClock();
                SecondsEllapsedForFrame = Win32GetSecondsEllapsed(WorkCounterStart, WorkCounterEnd);
                if (SecondsEllapsedForFrame < DesiredSecsPerFrame)
                {
                    while (SecondsEllapsedForFrame < DesiredSecsPerFrame)
                    {
                        DWORD SleepMS = 1000.0f * (DesiredSecsPerFrame - SecondsEllapsedForFrame);
                        Sleep(SleepMS);

                        SecondsEllapsedForFrame = Win32GetSecondsEllapsed(WorkCounterStart, Win32GetWallClock());
                    }
                }

                WorkCounterEnd = Win32GetWallClock();
                int64_t CounterEllapsed = (WorkCounterEnd.QuadPart - WorkCounterStart.QuadPart);
                int64_t CounterFrequency = GlobalPerformanceFrequency;
                int32_t MSPerFrame = (int32_t)(1000*CounterEllapsed)/CounterFrequency;
                int32_t FPS = CounterFrequency / CounterEllapsed;

                char Text[256];
                wsprintf(Text, "%s - FPS: %d  MS: %d", APP_NAME, FPS, MSPerFrame);
                SetWindowTextA(WindowHandle, Text);
            }
        }
    }

    return 0;
}