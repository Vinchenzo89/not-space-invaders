#ifndef APP_H
#include <stdint.h>

#define APP_NAME "Not Space Invaders"

#define global static
#define internal static

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef size_t umi;

#define Assert(Expression) if (!(Expression)) {*(int *)0 = 0;}

struct app_key_state
{
    bool IsDown;
};

struct app_input 
{
    int ScreenWidth;
    int ScreenHeight;
    float FrameEllapsedSecs;
    app_key_state KeyState[256];
    app_key_state OldKeyState[256];
};

struct app_memory
{
    umi PerminantStorageSize;
    void *PerminantStorage;

    umi TransientStorageSize;
    void *TransientStorage;
};

struct memory_arena
{
    umi Size;
    umi Used;
    void *Memory;
};

#define ArraySize(Array) (sizeof(Array)/sizeof(Array[0]))

#define PushStruct(Arena, type) (type *)PushSize(Arena, sizeof(type))
#define PushArray(Arena, Size, type) (type *)PushSize(Arena, Size * sizeof(type))
inline void * 
PushSize(memory_arena *Arena, umi Size)
{
    Assert(Arena->Size >= (Arena->Used + Size));
    void *Result = ((u8 *)Arena->Memory + Arena->Used);
    Arena->Used += Size;
    return(Result);
}

enum app_rgba_u32_color
{
    Black = 0x00000000,
    Red   = 0xFFFF0000,
    Green = 0xFF00FF00,
    Blue  = 0xFF0000FF,
    White = 0xFFFFFFFF
};

inline u32
RGBA_U32(u8 R, u8 G, u8 B, u8 A)
{
    u32 Result = (
        B << 0  |
        G << 8  |
        R << 16 |
        A << 24
    );
    return(Result);
}

inline u32
RGB_U32(u8 R, u8 G, u8 B)
{
    u32 Result = (
        B << 0  |
        G << 8  |
        R << 16 |
        255 << 24
    );
    return(Result);
}

enum render_command_type
{
    RenderCommand_Unknown,
    RenderCommand_Clear,
    RenderCommand_Rectangle
};

struct render_command_header
{
    render_command_type Type;
    umi Size;
};

struct render_clear_color
{
    u32 Color;
};

struct render_rectangle
{
    u32 X, Y;
    u32 Width, Height;
    u32 Color;
};

struct render_commands
{
    umi PushBufferSize;
    umi PushBufferUsed;
    umi PushBufferEntryCount;
    void *PushBuffer;
};
#define CreateRenderCommands(Size, Buffer) {Size, 0, 0, Buffer}

#define PushRenderCommand(Commands, Type, type) (type *)PushRenderCommand_(Commands, Type, sizeof(type))
inline void *
PushRenderCommand_(render_commands *Commands, render_command_type Type, umi Size)
{
    u64 TotalUsed = (Commands->PushBufferUsed + sizeof(render_command_header) + Size);
    Assert(Commands->PushBufferSize >= TotalUsed);
    
    void *Result = ((u8 *)Commands->PushBuffer + Commands->PushBufferUsed);
    render_command_header *Header = (render_command_header *)Result; 
    Header->Type = Type;
    Header->Size = Size + sizeof(render_command_header);

    Result = ((u8 *)Result + sizeof(render_command_header));
    Commands->PushBufferUsed = TotalUsed;
    Commands->PushBufferEntryCount++;
    return(Result);
}

#define APP_H
#endif