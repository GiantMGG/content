/*-- Salt Road, Act IV: The Salt Run (cycle 226). --*/
/* Work the desert's first salt works at the oasis: boil the brine    */
/* in the salt pans, load two camels with SALT, and run the loaded    */
/* caravan east past scripted ambushes to the far market station.     */
/* Win: the salt is delivered; lose: the caravan dies with it.        */

#strict 2

static g_iPhase;      // 0 salt works, 1 on the road, 2 ended
static g_iArrived;    // camels through the market gate
static g_iAmbushIdx;  // next ambush threshold index
static g_iCrewSeen;   // crew-wiped watch latch (see Task 3)

static const SALTROAD4_CARGO    = 6;  // SALT riding camels to depart
static const SALTROAD4_DELIVERY = 6;  // SALT in the market rect to win

protected func Initialize()
{
	g_iPhase = 0;
	g_iArrived = 0;
	g_iAmbushIdx = 0;
	g_iCrewSeen = 0;

	// The caravan-to-be: two camels at the west camp.
	CreateObject(CAML, 140, GroundY(140) - 20, NO_OWNER);
	CreateObject(CAML, 180, GroundY(180) - 20, NO_OWNER);

	Log("Salt Road Act IV: the salt run begins.");
	StoryMessage("$MsgIntro$");
	// Repeat-self button (Tutorial01 pattern).
	SetNextMission("SaltRoad.c4f\\SaltRoad04.c4s", "$BtnReplayActIV$", "$BtnReplayActIVDesc$");
	AddEffect("SaltRoadAct", 0, 1, 35, 0);
	return true;
}

// global, not private: the effect-director global func below calls these
// bare-name, and bare-name calls inside global funcs resolve only through
// the engine-owned global func map -- scenario-local scope is not visible
// there (cycle-146 func-map lesson, FirstLight.c4s precedent).
global func GroundY(int x)
{
	var y = 0;
	while (y < LandscapeHeight() && !GBackSolid(x, y)) y++;
	return y;
}

global func StoryMessage(string msg)
{
	var plr = 0;
	if (GetPlayerCount() > 0) plr = GetPlayerByIndex(0);
	return CustomMessage(msg, 0, plr, 0, 0, 0xffffff, 0, "Portrait:SCLK::0000ff::1", MSG_Bottom);
}

/* ---- the director ---- */

// Skeleton scope (cycle 226, Task 1): the win re-check, defeat watch,
// crew-wiped latch, departure check and ambush schedule land with Tasks
// 2-3. The director must be a GLOBAL func either way: effects on target
// 0 need GLOBAL callbacks (SaltChainSmoke lesson, FirstLightClimate-
// proven driver form).
global func FxSaltRoadActTimer(target, effect, time)
{
	return FX_OK;
}
