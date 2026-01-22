#include "global.h"
#include "event_data.h"
#include "script.h"

/**
 * ::ACIMUT::
 * 2022/03/29
 * 
 * Cambiar estas funciones como specials.
*/

// follow me script commands
#include "follow_me.h"


//hacer hook a la tabla de specials.
//special 0x10
void Setfollower(void)
{
    u8 localId = gSpecialVar_0x8000;
    // CFRU-like convenience: if not provided, use the last talked NPC.
    if (localId == 0)
        localId = (u8)gSpecialVar_LastTalked;
    u16 flags = gSpecialVar_0x8001;

    SetUpFollowerSprite(localId, flags);
}

/*
bool8 ScrCmd_setfollower(struct ScriptContext *ctx)
{
    u8 localId = ScriptReadByte(ctx);
    u16 flags = ScriptReadHalfword(ctx);

    SetUpFollowerSprite(localId, flags);
    return FALSE;
}


//special 0x11
bool8 ScrCmd_destroyfollower(struct ScriptContext *ctx)
{
    DestroyFollower();
    return FALSE;
}

//special 0x12
bool8 ScrCmd_facefollower(struct ScriptContext *ctx)
{
    PlayerFaceFollowerSprite();
    return FALSE;
}

//special 0x13
bool8 ScrCmd_checkfollower(struct ScriptContext *ctx)
{
    CheckPlayerHasFollower();
    return FALSE;
}
*/
