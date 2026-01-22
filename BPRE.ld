__aeabi_idiv = 0x081e3b68|1;
__aeabi_idivmod = 0x081e3b68|1;
__aeabi_uidiv = 0x081e3b68|1;
__aeabi_uidivmod = 0x081e3b68|1;
__divsi3 = 0x081e4018|1;
__udivsi3 = 0x081e460c|1;
__umodsi3 = 0x081e4684|1;
memcpy = 0x081e5e78|1;
memset = 0x081e5ed8|1;
strcmp = 0x081e5f2c|1;
strcpy = 0x081e5f88|1;
memmove = 0x081e8e60|1;


/*  EWRAM   */
gSpecialVar_0x8000 = 0x020370b8;
gSpecialVar_0x8001 = 0x020370ba;
gSpecialVar_0x8002 = 0x020370bc;
gSpecialVar_0x8003 = 0x020370be;
gSpecialVar_0x8004 = 0x020370c0;
gSpecialVar_0x8005 = 0x020370c2;
gSpecialVar_0x8006 = 0x020370c4;
gSpecialVar_0x8007 = 0x020370c6;
gSpecialVar_0x8008 = 0x020370c8;
gSpecialVar_0x8009 = 0x020370ca;
gSpecialVar_0x800A = 0x020370cc;
gSpecialVar_0x800B = 0x020370ce;
gSpecialVar_Result = 0x020370d0;
gSpecialVar_LastTalked = 0x020370d2;
gSpecialVar_Facing = 0x020370d4;
gSpecialVar_MonBoxId = 0x020370d6;
gSpecialVar_MonBoxPos = 0x020370d8;
gSpecialVar_TextColor = 0x020370da;
gSpecialVar_PrevTextColor = 0x020370dc;
/*battle_main.c*/
gBattleTypeFlags = 0x02022b4c;
gBattleOutcome = 0x02023e8a;
gPlayerParty = 0x02024284;
gEnemyParty = 0x0202402c;
/*event_object_movement.c*/
gLinkPlayerObjectEvents = 0x02031dec;
gObjectEvents = 0x02036e38;
/*field_effect.c */
gPlayerAvatar = 0x02037078;
gFieldEffectArguments = 0x020386e0;
/*fldeff_teleport.c*/
gMapHeader = 0x02036dfc;
gPostMenuFieldCallback = 0x0203b0c4;
/*item_use.c */
sItemUseOnFieldCB = 0x02039998;
/*follow_me.c*/
gSprites = 0x0202063c;
gFieldEffectArguments = 0x020386e0;


/*  IWRAM   */
gSaveBlock1Ptr = 0x03005008;
gSaveBlock2Ptr = 0x0300500c;
/*battle_main.c*/
gPreBattleCallback1 = 0x03004f80;
gMain = 0x030030f0;
gReceivedRemoteLinkPlayers = 0x03003f64;
/*field_contol_avatar*/
gSelectedObjectEvent = 0x03005074;
/*field_player_avatar.c*/
gTasks = 0x03005090;
/*fldeff_teleport.c*/
gFieldCallback2 = 0x03005024;
/*overworld.c*/
gQuestLogPlaybackState = 0x03005e88;
gInputToStoreInQuestLogMaybe = 0x03005078;
gTotalCameraPixelOffsetY = 0x03005068;
gTotalCameraPixelOffsetX = 0x0300506c;
/*follow_me.c*/
sScriptContext1 = 0x03000eb0;


/*  CONST   */
/*event_object_movement.c*/
sSpriteStepFuncsBySpeed = 0x083a719c;
sSpriteStepCountsBySpeed = 0x083a71b0;
/*field_contol_avatar*/
gDirectionToVectors = 0x0826d2d8;
EventScript_Waterfall = 0x081be2b7;
EventScript_CantUseWaterfall = 0x081be2ff;
EventScript_CurrentTooFast = 0x081a6b0d;
EventScript_UseSurf = 0x081a6ac8;
/*item_use.c */
gUnknown_8416451 = 0x08416451;


/*  FUNCS   */
VarGet = 0x0806e568|1;
VarSet = 0x0806e584|1;
FlagSet = 0x0806e680|1;
FlagClear = 0x0806e6a8|1;
FlagGet = 0x0806e6d0|1;
/*battle_main.c*/
RandomlyGivePartyPokerus = 0x08043a68|1;
PartySpreadPokerus = 0x08043b40|1;
UpdateRoamerHPStatus = 0x08141fe8|1;
SetRoamerInactive = 0x0814201c|1;
m4aSongNumStop = 0x081dd1c0|1;
SetMainCallback2 = 0x08000544|1;
/*event_object_movement.c*/
GetObjectEventIdByLocalIdAndMapInternal = 0x0805dff4|1;
GetObjectEventIdByLocalId = 0x0805e044|1;
RemoveObjectEventIfOutsideView = 0x0805ed70|1;
AreZCoordsCompatible = 0x0806835c|1;
sub_8112CAC = 0x08112cac|1;
ObjectEventClearHeldMovementIfActive = 0x08063d1c|1;
ObjectEventIsMovementOverridden = 0x08063c70|1;
UnfreezeObjectEvent = 0x080689f4|1;
MoveCoordsInDirection = 0x08063a6c|1;
MoveCoordsInDirection = 0x08063a6c|1;
MetatileBehavior_IsWaterfall = 0x0805a104|1;
MapGridGetMetatileBehaviorAt = 0x08058f78|1;
/*field_contol_avatar*/
GetObjectEventIdByXYZ = 0x0805f894|1;
MetatileBehavior_IsCounter = 0x08059ec4|1;
ObjectEventCheckHeldMovementStatus = 0x08063d68|1;
InUnionRoom = 0x0811b0d0|1;
GetObjectEventScriptPointerByObjectEventId = 0x0805fc38|1;
GetRamScript = 0x08069d8c|1;
MetatileBehavior_IsSemiDeepWater = 0x08059cc8|1;
PartyHasMonWithSurf = 0x0805c83c|1;
IsPlayerFacingSurfableFishableWater = 0x0805c8b0|1;
IsPlayerSurfingNorth = 0x0805c88c|1;
/*field_effect.c */
Task_FallWarpFieldEffect = 0x08084484|1;
ScriptContext2_Disable = 0x0806994c|1;
CameraObjectReset1 = 0x0805faa8|1;
UnfreezeObjectEvents = 0x08068a5c|1;
InstallCameraPanAheadCallback = 0x0805adf8|1;
PlayerGetDestCoords = 0x0805c538|1;
sub_8055B38 = 0x08055b38|1;
SetPlayerAvatarTransitionFlags = 0x0805bdec|1;
SetHelpContext = 0x0812b1f0|1;
DestroyTask = 0x08077508|1;
FindTaskIdByFunc = 0x08077688|1;
ObjectEventClearHeldMovementIfFinished = 0x08063d7c|1;
/*ObjectEventSetHeldMovement = 0x08063ca4|1;*/
GetFaceDirectionMovementAction = 0x08063eb8|1;
GetPlayerFacingDirection = 0x0805c6c4|1;
PlaySE = 0x080722cc|1;
FieldEffectActiveListContains = 0x0808382c|1;
ObjectEventSetGraphicsId = 0x0805f060|1;
GetPlayerAvatarGraphicsIdByStateId = 0x0805c808|1;
GetJumpSpecialMovementAction = 0x080641c0|1;
FieldEffectStart = 0x08083444|1;
/*field_player_avatar.c*/
CheckForPlayerAvatarCollision = 0x0805bb1c|1;
Task_StopSurfingInit = 0x0805d1d4|1;
PlayerJumpLedge = 0x0805c23c|1;
PlayerFaceDirection = 0x0805c1f4|1;
PlayerNotOnBikeCollide = 0x0805c1d0|1;
IsRunningDisallowed = 0x080bd488|1;
PlayerIsMovingOnRockStairs = 0x0805baac|1;
PlayerGoSlow = 0x0805c0ec|1;
PlayerGoSpeed1 = 0x0805c104|1;
PlayerGoSpeed2 = 0x0805c11c|1;
PlayerRun = 0x0805c17c|1;
PlayerRunSlow = 0x0805c194|1;
GetPlayerAvatarGraphicsIdByStateIdAndGender = 0x0805c7e0|1;
SpawnSpecialObjectEvent = 0x0805e7f4|1;
CreateWarpArrowSprite = 0x080db0c4|1;
ObjectEventTurn = 0x0805f218|1;
ClearPlayerAvatarInfo = 0x0805c95c|1;
SetPlayerAvatarStateMask = 0x0805c970|1;
ScriptContext2_Enable = 0x08069940|1;
FreezeObjectEvents = 0x08068974|1;
Overworld_ClearSavedMusic = 0x08055e84|1;
Overworld_ChangeMusicToDefault = 0x08055f1c|1;
/*field_screen_effect.c*/
WarpFadeInScreenWithDelay = 0x0807dbac|1;
sub_807DE78 = 0x0807de78|1;
Overworld_PlaySpecialMapMusic = 0x08055dc4|1;
QuestLog_DrawPreviouslyOnQuestHeaderIfInPlaybackMode = 0x08111cf0|1;
SetPlayerVisibility = 0x0807dcb0|1;
FieldSetDoorOpened = 0x0805b268|1;
DoOutwardBarnDoorWipe = 0x0807f114|1;
GetDoorSoundEffect = 0x0805b364|1;
FieldAnimateDoorOpen = 0x0805b310|1;
FieldIsDoorAnimationRunning = 0x0805b350|1;
/*GetObjectEventIdByLocalIdAndMap = 0x0805df60|1;*/
FieldAnimateDoorClose = 0x0805b2d0|1;
FieldFadeTransitionBackgroundEffectIsFinished = 0x0807e418|1;
walkrun_is_standing_still = 0x08069554|1;
FuncIsActiveTask = 0x08077650|1;
Task_BarnDoorWipe = 0x0807f204|1;
GetWalkNormalMovementAction = 0x08063f84|1;
StartTeleportInPlayerAnim = 0x0805dc04|1;
WaitTeleportInPlayerAnim = 0x0805dc24|1;
/*fldeff_teleport.c*/
FieldCallback_Teleport = 0x080f6730|1;
Overworld_MapTypeAllowsTeleportAndFly = 0x080561fc|1;
FieldCallback_PrepareFadeInFromMenu = 0x081248b0|1;
/*item_use.c */
DisplayItemMessageInCurrentContext = 0x080a10c4|1;
sub_80A103C = 0x080a103c|1;
PrintNotTheTimeToUseThat = 0x080a1110|1;
MetatileBehavior_IsIsolatedVerticalRail = 0x0805a210|1;
MetatileBehavior_IsIsolatedHorizontalRail = 0x0805a214|1;
MetatileBehavior_IsVerticalRail = 0x0805a218|1;
MetatileBehavior_IsHorizontalRail = 0x0805a21c|1;
Overworld_IsBikingAllowed = 0x08055c9c|1;
IsBikingDisallowedByPlayer = 0x080bd540|1;
TestPlayerAvatarFlags = 0x0805c74c|1;
GetOnOffBike = 0x080bd5c8|1;
ClearPlayerHeldMovementAndUnfreezeObjectEvents = 0x080696c0|1;
/*party_menu.c */
FieldCallback_Surf = 0x0812497c|1;
FieldCallback_Waterfall = 0x08124adc|1;
GetXYCoordsOneStepInFrontOfPlayer = 0x0805c4f4|1;
/*overworld.c*/
GetInitialPlayerAvatarState = 0x08055a6c|1;
ResetInitialPlayerAvatarState = 0x080559e4|1;
sub_8112B3C = 0x08112b3c|1;
PlayerAvatarTransition_ReturnToField = 0x0805bea8|1;
UpdatePlayerAvatarTransitionState = 0x0805beb8|1;
FieldClearPlayerInput = 0x0806c888|1;
FieldGetPlayerInput = 0x0806c8bc|1;
FieldInput_HandleCancelSignpost = 0x0806cd30|1;
ScriptContext2_IsEnabled = 0x08069958|1;
ProcessPlayerFieldInput = 0x0806cac8|1;
sub_81127F8 = 0x081127f8|1;
DismissMapNamePopup = 0x080982ec|1;
player_step = 0x0805b3e0|1;
RunQuestLogCB = 0x0811092c|1;
GetFaceDirectionAnimNum = 0x08063430|1;
InitOverworldBgs = 0x080562b0|1;
ResumeMap = 0x08057024|1;
ReloadObjectsAndRunReturnToFieldMapScript = 0x08057100|1;
sub_8057114 = 0x08057114|1;
sub_8056F08 = 0x08056f08|1;
map_post_load_hook_exec = 0x080565ec|1;
QuestLog_InitPalettesBackup = 0x08111f14|1;
SetHelpContextForMap = 0x0812b35c|1;
ResetObjectEvents = 0x0805de74|1;
GetCameraFocusCoords = 0x08059808|1;
TrySpawnObjectEvents = 0x0805ec30|1;
TryRunOnWarpIntoMapScript = 0x08069c9c|1;
/*new_game.c*/
ResetTrainerTowerResults = 0x0815ee0c|1;
/*srccmd.c*/
ScriptReadHalfword = 0x080698f8|1;
/*follow_me.c*/
MovementType_None = 0x0805ffb4|1;
SetSurfBlob_BobState = 0x080dc44c|1;
DestroySprite = 0x08007280|1;
StartUnderwaterSurfBlobBobbing = 0x080dc6b0|1;
MoveObjectEventToMapCoords = 0x0805f724|1;
CreateTask = 0x0807741c|1;
MetatileBehavior_IsIce = 0x08059bb4|1;
MetatileBehavior_IsUnknownMovement48 = 0x08059dc0|1;
ObjectEventClearHeldMovement = 0x08063d34|1;
GetJumpMovementAction = 0x08064194|1;
TryFadeOutOldMapMusic = 0x08055f88|1;
WarpFadeOutScreen = 0x0807dc18|1;
PlayRainStoppingSoundEffect = 0x0807b004|1;
Task_Teleport2Warp = 0x0807e718|1;
Sin = 0x08044e30|1;
Cos = 0x08044e4c|1;
RemoveObjectEvent = 0x0805e4b4|1;
GetObjectEventTemplateByLocalIdAndMap = 0x0805fd5c|1;
TrySpawnObjectEventTemplate = 0x0805e72c|1;
Overworld_SetObjEventTemplateMovementType = 0x08055114|1;



