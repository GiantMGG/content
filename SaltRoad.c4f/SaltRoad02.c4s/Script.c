/*-- Salt Road, Act II: The Dune Sea (cycle 146). --*/
/* Escort two camels across the dune sea. Choose the high ridge      */
/* (fast, exposed to sand-drift burial) or the wadi (slow, quicksand */
/* and scripted ambushes). Mid-crossing the storm hits. Reach the     */
/* far station. Lose if all camels die.                               */

#strict 2

static g_iPhase;        // 0 route choice, 1 crossing, 2 done
static g_iRoute;        // 0 none, 1 ridge, 2 wadi
static g_iStormDone;    // 0 pending, 1 launched
static g_iAmbushIdx;    // next wadi ambush threshold
static g_iArrived;

static const SALTROAD2_TOLERANCE = 40;

protected func Initialize()
{
	g_iPhase = 0;
	g_iRoute = 0;
	g_iStormDone = 0;
	g_iAmbushIdx = 0;
	g_iArrived = 0;

	// The caravan.
	CreateObject(CAML, 140, GroundY(140) - 20, NO_OWNER);
	CreateObject(CAML, 180, GroundY(180) - 20, NO_OWNER);
	// The sand-drift director keeps the dunes migrating all game.
	CreateObject(SDRF, LandscapeWidth() / 2, 30, NO_OWNER);
	// Route beacons: sandstone cairns. Stand at one to choose the road.
	CreateObject(SNDS, 240, GroundY(240) - 10, NO_OWNER);
	CreateObject(SNDS, 340, GroundY(340) - 10, NO_OWNER);
	// The far station cairn marks the arrival end of the map.
	CreateObject(SNDS, LandscapeWidth() - 60, GroundY(LandscapeWidth() - 60) - 10, NO_OWNER);
	// Wadi quicksand (spec §Act 2, review F3): fixed QKSD patches on the
	// slow corridor. The patches sit between the wadi waypoints (380/560,
	// 560/740, 920/1100) so the caravan crosses them, and QKSD self-timers
	// its Sink() call (DefCore Timer=4) -- no script upkeep needed.
	CreateObject(QKSD, 470, GroundY(470) - 2, NO_OWNER);
	CreateObject(QKSD, 650, GroundY(650) - 2, NO_OWNER);
	CreateObject(QKSD, 1010, GroundY(1010) - 2, NO_OWNER);

	Log("Salt Road Act II: the dune sea opens.");
	StoryMessage("$MsgIntro$");
	SetNextMission("SaltRoad.c4f\\SaltRoad02.c4s", "$BtnReplayActII$", "$BtnReplayActIIDesc$");
	AddEffect("SaltRoadAct", 0, 1, 35, 0);
	return true;
}

// global, not private: the effect-director global func below and the
// caravan helpers call these bare-name, and bare-name calls inside global
// funcs resolve only through the engine-owned global func map -- scenario-
// local scope is not visible there (cycle-146 func-map lesson,
// FirstLight.c4s precedent).
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

// global, not private: bare-name called from the global director.
global func ChooseRoute(int route)
{
	g_iPhase = 1;
	g_iRoute = route;
	var waypoints;
	if (route == 1)
		waypoints = [400, 700, 1000, LandscapeWidth() - 120];
	else
		waypoints = [380, 560, 740, 920, 1100, LandscapeWidth() - 120];
	var camel;
	for (camel in FindObjects(Find_ID(CAML)))
		SaltRoad_StartCaravan(camel, waypoints);
	if (route == 1)
		StoryMessage("$MsgRidge$");
	else
		StoryMessage("$MsgWadi$");
}

global func LeadCamelX()
{
	var camel, maxx = 0;
	for (camel in FindObjects(Find_ID(CAML)))
		if (GetX(camel) > maxx) maxx = GetX(camel);
	return maxx;
}

global func AmbushCheck()
{
	// Wadi ambushes fire when the lead camel crosses scripted thresholds.
	var thresholds = [600, 900];
	if (g_iAmbushIdx < GetLength(thresholds) && LeadCamelX() >= thresholds[g_iAmbushIdx])
	{
		var x = thresholds[g_iAmbushIdx];
		SaltRoad_SpawnRaid(x, GroundY(x) - 40, 2);
		g_iAmbushIdx++;
		StoryMessage("$MsgAmbush$");
	}
}

global func FxSaltRoadActTimer(target, effect, time)
{
	// Win re-check (review F1): same soft-lock seam as Act I -- a camel
	// killed in transit after the first arrival never fires another
	// callback, so poll completion here every director tick.
	if (g_iPhase < 2 && g_iArrived > 0 && g_iArrived >= ObjectCount(CAML))
	{
		g_iPhase = 2;
		return CaravanComplete();
	}

	// Defeat watch: the caravan is dead, the crossing is lost.
	if (g_iPhase < 2 && ObjectCount(CAML) == 0)
	{
		g_iPhase = 2;
		StoryMessage("$MsgDefeat$");
		GameOver();
		return FX_OK;
	}

	if (g_iPhase == 0)
	{
		// Route choice: a clonk standing at either cairn.
		var clnk;
		for (clnk in FindObjects(Find_ID(CLNK), Find_NoContainer()))
		{
			if (Abs(GetX(clnk) - 240) < SALTROAD2_TOLERANCE
			 && Abs(GetY(clnk) - GroundY(240)) < SALTROAD2_TOLERANCE + 20)
				{ ChooseRoute(1); break; }
			if (Abs(GetX(clnk) - 340) < SALTROAD2_TOLERANCE
			 && Abs(GetY(clnk) - GroundY(340)) < SALTROAD2_TOLERANCE + 20)
				{ ChooseRoute(2); break; }
		}
	}
	else if (g_iPhase == 1)
	{
		// Mid-crossing storm: once the lead camel passes mid-map.
		if (!g_iStormDone && LeadCamelX() > LandscapeWidth() / 2)
		{
			g_iStormDone = 1;
			StoryMessage("$MsgStorm$");
			LaunchWeatherEvent(SNDT, 50, 1400);
		}
		if (g_iRoute == 2) AmbushCheck();
	}
	return FX_OK;
}

// The single Act-II victory: message, next-mission button and game over.
// Called from the arrival callback and the director's win re-check
// (review F1). global, not private: bare-name called from the director.
global func CaravanComplete()
{
	g_iPhase = 2;
	StoryMessage("$MsgActComplete$");
	SetNextMission("SaltRoad.c4f\\SaltRoad03.c4s", "$BtnNextActIII$", "$BtnNextActIIIDesc$");
	GameOver();
	return true;
}

// Arrival callback (GameCall from the caravan puppet, Camel.c4d).
// GameCall receivers must be public (script-local): global funcs are
// engine-owned and invisible to Game.Script's own-table lookup.
public func SaltRoadCaravanArrived(object camel)
{
	g_iArrived++;
	if (g_iArrived >= ObjectCount(CAML)) return CaravanComplete();
	return true;
}
