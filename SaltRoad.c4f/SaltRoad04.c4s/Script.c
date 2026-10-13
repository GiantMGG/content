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

	// The salt works at the canyon pocket (Seed=226 terrain). The OASS
	// comes FIRST: its Construction() carves the basin notch into the
	// west slope's base before anything is painted.
	CreateObject(OASS, 646, GroundY(646) - 6, NO_OWNER);
	// The painted waterline: the pocket floor's top rows become the
	// pool the pan works. Painted AFTER the OASS exists (its DigFreeRect
	// must bite before water fills the notch) and kept at/below the
	// surface line, where material inserts stick (SaltChainSmoke
	// determinism note: above-surface inserts do not stick).
	DrawMaterialQuad("Water", 628, 59, 684, 59, 684, 76, 628, 76, false);
	// The pan on the bank: GroundY reads through the non-solid painted
	// water to the sand beneath, so the pan's base rests on the bank
	// below the waterline and its flank probes sample the pool.
	var pan = CreateObject(SLTP, 664, GroundY(664) - 6, NO_OWNER);
	// Permanent placement pin: at least one ShoreContact probe offset
	// (SaltPan.c4d) must read the painted Water from tick 0.
	if (pan)
	{
		var water = Material("Water");
		if (pan->GetMaterial(0, 10) != water
		 && pan->GetMaterial(-17, 4) != water
		 && pan->GetMaterial(17, 4) != water
		 && pan->GetMaterial(-17, -2) != water
		 && pan->GetMaterial(17, -2) != water)
			Log("Salt Road Act IV: salt pan not on the waterline (placement defect)");
	}

	// The quarry on the painted sandstone vein (Act-I verbatim shape):
	// painted vein + seated quarry + placement pin.
	var qy = GroundY(760);
	DrawMaterialQuad("Sandstone", 720, qy - 6, 800, qy - 6, 800, qy + 12, 720, qy + 12, false);
	var quarry = CreateObject(QRRY, 760, qy - 4, NO_OWNER);
	if (quarry)
		if (quarry->GetMaterial(0, 4) != Material("Sandstone"))
			Log("Salt Road Act IV: quarry not on sandstone (placement defect)");

	// Palm groves on the oasis slope (Act-I DATP shape).
	CreateObject(DATP, 612, GroundY(612) - 10, NO_OWNER);
	CreateObject(DATP, 620, GroundY(620) - 10, NO_OWNER);
	CreateObject(DATP, 628, GroundY(628) - 10, NO_OWNER);

	// The far market station.
	CreateObject(MKTS, LandscapeWidth() - 100, GroundY(LandscapeWidth() - 100) - 10, NO_OWNER);

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
// crew-wiped latch, departure check and ambush schedule land with Task
// 3. The director must be a GLOBAL func either way: effects on target
// 0 need GLOBAL callbacks (SaltChainSmoke lesson, FirstLightClimate-
// proven driver form).
global func FxSaltRoadActTimer(target, effect, time)
{
	return FX_OK;
}
