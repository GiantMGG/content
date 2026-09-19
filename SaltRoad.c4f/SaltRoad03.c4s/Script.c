/*-- Salt Road, Act III: The Queen's Toll (cycle 146). --*/
/* The scorpion queen holds the canyon pass. Settle her toll by      */
/* payment (ten dates on her shrine) or by combat (bring her down).  */
/* Both paths converge on the shared SaltRoad_Ending() epilogue.     */

#strict 2

static g_iPhase;         // 0 toll, 1 ended
static g_iBroodClock;    // director ticks since last brood tick

static const SALTROAD3_TOLL     = 10;   // dates on the shrine slab
static const SALTROAD3_SHRINE_X = 300;  // shrine slab center
static const SALTROAD3_BROOD    = 350;  // brood cadence, director ticks

protected func Initialize()
{
	g_iPhase = 0;
	g_iBroodClock = 0;

	// The shrine: sandstone pillars flanking the offering slab.
	CreateObject(SNDS, SALTROAD3_SHRINE_X - 60, GroundY(SALTROAD3_SHRINE_X - 60) - 10, NO_OWNER);
	CreateObject(SNDS, SALTROAD3_SHRINE_X + 60, GroundY(SALTROAD3_SHRINE_X + 60) - 10, NO_OWNER);
	// Date palm groves: the toll must be grown, not granted.
	var i, x;
	for (i = 0; i < 8; i++)
	{
		x = 120 + i * 120;
		CreateObject(DATP, x, GroundY(x) - 10, NO_OWNER);
	}
	// The queen holds the narrows.
	SaltRoad_SpawnQueen(LandscapeWidth() / 2, GroundY(LandscapeWidth() / 2) - 20);

	Log("Salt Road Act III: the queen's toll awaits.");
	StoryMessage("$MsgIntro$");
	SetNextMission("SaltRoad.c4f\\SaltRoad03.c4s", "$BtnReplayActIII$", "$BtnReplayActIIIDesc$");
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

// The queen is the SCRP tagged by SaltRoad_SpawnQueen: poll the tag, never
// a static queen pointer (#strict 2: no nil in this script, return 0).
global func FindQueen()
{
	var s;
	for (s in FindObjects(Find_ID(SCRP)))
		if (GetEffect("SaltRoadQueen", s))
			return s;
	return 0;
}

global func ShrineOfferings()
{
	var d, count = 0;
	for (d in FindObjects(Find_ID(DATE), Find_AtRect(SALTROAD3_SHRINE_X - 60, 0, 120, LandscapeHeight())))
		count++;
	return count;
}

global func FxSaltRoadActTimer(target, effect, time)
{
	if (g_iPhase != 0) return FX_OK;

	// The brood swells while the queen lives.
	g_iBroodClock += 35;
	if (g_iBroodClock >= SALTROAD3_BROOD)
	{
		g_iBroodClock = 0;
		var queen = FindQueen();
		if (queen) SaltRoad_QueenBrood(queen);
	}

	// PAY: the shrine offering is complete.
	if (ShrineOfferings() >= SALTROAD3_TOLL)
	{
		StoryMessage("$MsgTollPaid$");
		return Ending();
	}

	// FIGHT: the queen has fallen.
	if (!FindQueen())
	{
		StoryMessage("$MsgQueenSlain$");
		return Ending();
	}
	return FX_OK;
}

// global, not private: bare-name `return Ending()` from the global effect
// director (func-map lesson). The single deterministic epilogue
// (SaltRoad_Ending, Scorpion.c4d): both Act 3 win paths converge here.
global func Ending()
{
	g_iPhase = 1;
	SaltRoad_Ending();
	return true;
}
