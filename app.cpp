#include "app.h"

inline float
Max(float A, float B)
{
    float Result = A > B ? A : B;
    return(Result);
}

inline float
Min(float A, float B)
{
    float Result = A > B ? B : A;
    return(Result);
}

union v2
{
    struct {
        float X;
        float Y;
    };
    struct {
        float Width;
        float Height;
    };
};

inline v2
operator+(v2 A, v2 B)
{
    v2 Result = {
        A.X + B.X,
        A.Y + B.Y
    };
    return(Result);
};

inline v2
operator+=(v2 A, v2 B)
{
    v2 Result = {
        A.X + B.X,
        A.Y + B.Y
    };
    return(Result);
};

inline v2
operator*(float Value, v2 A)
{
    v2 Result = {
        A.X * Value,
        A.Y * Value
    };
    return(Result);
}

inline v2
operator*(v2 A, float Value)
{
    v2 Result = {
        A.X * Value,
        A.Y * Value
    };
    return(Result);
}

union rec
{
    struct
    {
        float Left;
        float Top;
        float Right;
        float Bottom;
    };
};

inline bool
Overlap(rec A, rec B)
{
    bool LeftSideOverlap = (A.Left >= B.Left && A.Left <= B.Right);
    bool RightSideOverlap = (A.Right >= B.Left && A.Right <= B.Right);
    bool TopSideOverlap = (A.Top >= B.Top  && A.Top < B.Bottom);
    bool BottomSideOverlap = (A.Bottom > B.Top  && A.Bottom < B.Bottom);
    
    bool BottomRightOverlap = BottomSideOverlap && RightSideOverlap;
    bool BottomLeftOverlap = BottomSideOverlap && LeftSideOverlap;
    bool TopRightOverlap = TopSideOverlap && RightSideOverlap;
    bool TopLeftOverlap = TopSideOverlap && LeftSideOverlap;

    bool Result = (
        BottomRightOverlap || BottomLeftOverlap ||
        TopRightOverlap    || TopLeftOverlap
    );

    return(Result);
}

enum entity_state_type
{
    EntityState_Dead,
    EntityState_Alive
};

#define PlayerDim 20
#define InvaderDim 20
struct entity
{
    v2 P;
    v2 dP;
    v2 Dim;
    float FireRateSecs;
    float FireDelaySecs;
    entity_state_type State;
};

#define IsDead(State) (State == EntityState_Dead)

inline rec
EntityBounds(entity E)
{
    rec Result = {
        E.P.X,
        E.P.Y,
        E.P.X + E.Dim.Width,
        E.P.Y + E.Dim.Height
    };
    return(Result);
}

struct invader_fleet
{
    u32 ID;
    v2 P;
    v2 dP;
    v2 Dim;
    u32 DeadInvaders;
    u32 InvaderCount;
    entity *Invaders;
};

enum level_outcome_type
{
    LevelOutcome_Unknown,
    LevelOutcome_YouWin,
    LevelOutcome_YouLose
};

#define NUM_LEVELS 1
#define MAX_INVADERS 20
#define MAX_MISSLES 10
struct game_state
{
    bool Initialized;

    entity Player;
    entity PlayerMissiles[MAX_MISSLES];

    invader_fleet Fleet;
    entity Invaders[MAX_INVADERS];
    entity InvaderMissiles[MAX_MISSLES];

    level_outcome_type LevelOutcome;
};

inline bool
KeyIsDown(app_input *Input, u32 KeyCode)
{
    bool Result = Input->KeyState[KeyCode].IsDown;
    return(Result);
}

inline bool
KeyPressed(app_input *Input, u32 KeyCode)
{
    bool IsDown = Input->KeyState[KeyCode].IsDown;
    bool WasDown = Input->OldKeyState[KeyCode].IsDown;
    bool Result = (IsDown || (IsDown && WasDown));
    return(Result);
}

internal void
InitializeLevel(app_input Input, game_state *GameState)
{
    char InvaderLayout[] = R"(
        0X0X0X0X0X|
        X0XX00XX0X|
        XX0X00X0XX|
        00X0000X00
    )";

    u32 Count = 0;
    float StartX = 5;
    float StartY = 5;
    float PaddingX = 10;
    float PaddingY = 20;
    float OffsetX = StartX;
    float OffsetY = StartY;
    for (u32 C = 0; C < ArraySize(InvaderLayout); ++C)
    {
        char I = InvaderLayout[C];
        switch(I)
        {
            case '0':
                OffsetX += InvaderDim + PaddingX;
            break;

            case 'X': {
                OffsetX += InvaderDim + PaddingX;
                entity *Invader = &GameState->Invaders[Count];
                Invader->P = {OffsetX, OffsetY};
                Invader->Dim = {InvaderDim,InvaderDim};
                Invader->FireRateSecs = 0.5;
                Invader->State = EntityState_Alive;
                ++Count;
            } break;

            case '|':
                OffsetX = StartX;
                OffsetY += InvaderDim + PaddingY;
            break;
        }
    }

    OffsetX += InvaderDim;
    OffsetY += InvaderDim + PaddingY;

    GameState->Fleet.P = {StartX,StartY};
    GameState->Fleet.dP = {200.0f, 10.0f};
    GameState->Fleet.Dim = {OffsetX-StartX,OffsetY-StartY};
    GameState->Fleet.Invaders = GameState->Invaders;
    GameState->Fleet.InvaderCount = Count;
}

internal void
AddMissile(game_state *GameState, entity Player)
{
    for (u32 Index = 0;
         Index < ArraySize(GameState->PlayerMissiles);
         ++Index)
    {
        entity *FreeMissle = &GameState->PlayerMissiles[Index];
        if (FreeMissle->State == EntityState_Dead)
        {
            FreeMissle->P = Player.P;
            FreeMissle->dP = {0,-500};
            FreeMissle->Dim = {5,10};
            FreeMissle->State = EntityState_Alive;
            break;
        }
    }
}

inline 

internal void
AdvancePositions(app_input Input, entity *Entities, u32 Count)
{
    for (u32 Index = 0;
         Index < Count;
         ++Index)
    {
        entity *Entity = &Entities[Index];
        v2 P = Entity->P;
        v2 dP = Entity->dP;
        P = P + (dP * Input.FrameEllapsedSecs);
        Entity->P = P;
        if (P.Y <= 0 || P.Y > Input.ScreenHeight)
        {
            Entity->State = EntityState_Dead;
        }
    }
}

internal void
AdvanceInvaderFleet(app_input Input, invader_fleet *Fleet)
{
    v2 P = Fleet->P;
    v2 dP = Fleet->dP;
    v2 Dim = Fleet->Dim;
    P = P + (dP * Input.FrameEllapsedSecs);

    if ((P.X + Dim.Width) >= Input.ScreenWidth)
        dP.X *= -1;
    if (P.X <= 0)
        dP.X *= -1;
    
    Fleet->P = P;
    Fleet->dP = dP;

    for (u32 I = 0; I < Fleet->InvaderCount; ++I)
    {
        entity *Invader = &Fleet->Invaders[I];
        v2 IdP = (dP * Input.FrameEllapsedSecs);
        Invader->P = Invader->P + IdP;
        Invader->dP = dP;
    }
}

struct CollisionResult
{
    u32 CollisionCount;
};

internal CollisionResult
DetectCollisions(entity *GroupA, u32 GroupACount, 
                 entity *GroupB, u32 GroupBCount)
{
    CollisionResult Result = {};

    for (u32 A = 0; A < GroupACount; ++A)
    {
        entity *EntA = &GroupA[A];
        if (IsDead(EntA->State)) continue;

        for (u32 B = 0; B < GroupBCount; ++B)
        {
            entity *EntB = &GroupB[B];
            if (IsDead(EntB->State)) continue;

            rec EntADim = EntityBounds(*EntA);
            rec EntBDim = EntityBounds(*EntB);
            if (Overlap(EntADim, EntBDim))
            {
                EntA->State = EntityState_Dead;
                EntB->State = EntityState_Dead;
                ++Result.CollisionCount;
            }
        }
    }

    return(Result);
}

internal void
GameUpdateAndRender(app_input Input, app_memory *Memory, render_commands *RenderCommands)
{
    game_state *GameState = (game_state *)Memory->PerminantStorage;
    if (!GameState->Initialized)
    {
        GameState->Initialized = true;
        GameState->Player.FireRateSecs = 0.2f;
        GameState->Player.P = {(float)Input.ScreenWidth/2.0f, (float)Input.ScreenHeight-100.0f};
        GameState->Player.Dim = {PlayerDim, PlayerDim};

        InitializeLevel(Input, GameState);
    }

    float PlayerSpeed = 500 * Input.FrameEllapsedSecs;
    if (KeyIsDown(&Input, (u32)'A'))
    {
        GameState->Player.P.X -= PlayerSpeed;
    }
    if (KeyIsDown(&Input, (u32)'D'))
    {
        GameState->Player.P.X += PlayerSpeed;
    }
    
    float FireRate = GameState->Player.FireRateSecs;
    float FireDelay = Max(GameState->Player.FireDelaySecs - Input.FrameEllapsedSecs, 0);
    GameState->Player.FireDelaySecs = FireDelay;
    if (KeyIsDown(&Input, 0x20) && FireDelay <= 0)
    {
        GameState->Player.FireDelaySecs = FireRate;
        AddMissile(GameState, GameState->Player);
    }

    AdvanceInvaderFleet(Input, &GameState->Fleet);

    AdvancePositions(Input, GameState->PlayerMissiles, ArraySize(GameState->PlayerMissiles));

    CollisionResult Fails = DetectCollisions(
        {&GameState->Player}, 1,
        GameState->InvaderMissiles, ArraySize(GameState->InvaderMissiles));

    CollisionResult Hits = DetectCollisions(
        GameState->PlayerMissiles, ArraySize(GameState->PlayerMissiles),
        GameState->Invaders, ArraySize(GameState->Invaders));
    GameState->Fleet.DeadInvaders += Hits.CollisionCount;


    render_clear_color *Clear = PushRenderCommand(RenderCommands, RenderCommand_Clear, render_clear_color);
    Clear->Color = Black;

    render_rectangle *Rectangle = PushRenderCommand(RenderCommands, RenderCommand_Rectangle, render_rectangle);
    Rectangle->X = GameState->Player.P.X;
    Rectangle->Y = GameState->Player.P.Y;
    Rectangle->Width = GameState->Player.Dim.Width;
    Rectangle->Height = GameState->Player.Dim.Height;
    Rectangle->Color = RGB_U32(0, 255, 150);

    for (u32 Index = 0; 
         Index < ArraySize(GameState->Invaders); 
         ++Index)
    {
        entity Invader = GameState->Invaders[Index];
        if (!Invader.State) continue;

        render_rectangle *Rect = PushRenderCommand(RenderCommands, RenderCommand_Rectangle, render_rectangle);
        Rect->Color = RGB_U32(0, 150, 255);
        Rect->X = Invader.P.X;
        Rect->Y = Invader.P.Y;
        Rect->Width = Invader.Dim.Width;
        Rect->Height = Invader.Dim.Height; 
    }

    for (u32 Index = 0; 
         Index < ArraySize(GameState->PlayerMissiles); 
         ++Index)
    {
        entity Missle = GameState->PlayerMissiles[Index];
        if (!Missle.State) continue;

        render_rectangle *Rect = PushRenderCommand(RenderCommands, RenderCommand_Rectangle, render_rectangle);
        Rect->Color = Red;
        Rect->X = Missle.P.X;
        Rect->Y = Missle.P.Y;
        Rect->Width = Missle.Dim.Width;
        Rect->Height = Missle.Dim.Height; 
    }

    level_outcome_type Outcome = GameState->Fleet.DeadInvaders >= GameState->Fleet.InvaderCount
        ? LevelOutcome_YouWin
        : LevelOutcome_Unknown;
    Outcome = Fails.CollisionCount > 0
        ? LevelOutcome_YouLose
        : Outcome;
    GameState->LevelOutcome = Outcome;

    if (GameState->LevelOutcome)
    {
        u32 Color = GameState->LevelOutcome == LevelOutcome_YouWin
            ? Green
            : Red;
        
        // You Lose
        render_rectangle *Rect = PushRenderCommand(RenderCommands, RenderCommand_Rectangle, render_rectangle);
        Rect->Color = Color;
        Rect->X = Input.ScreenWidth/3;
        Rect->Y = Input.ScreenWidth/4;
        Rect->Width = Input.ScreenWidth - (2 * Rect->X);
        Rect->Height = 40;
    }

    return;
}