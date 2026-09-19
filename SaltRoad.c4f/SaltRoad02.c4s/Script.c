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

	Log("Salt Road Act II: the dune sea opens.");
	StoryMessage("Two roads east: the high ridge, or the wadi. Walk to a cairn to choose.");
	SetNextMission("SaltRoad.c4f\\SaltRoad02.c4s", "Replay Act II", "The Dune Sea, once more.");
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
		StoryMessage("The high ridge: fast going, but the wind owns it. Dig buried camels free!");
	else
		StoryMessage("The wadi: slow ground, quicksand, and worse things waiting.");
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
		StoryMessage("Ambush! Scorpions in the sand!");
	}
}

global func FxSaltRoadActTimer(target, effect, time)
{
	// Defeat watch: the caravan is dead, the crossing is lost.
	if (g_iPhase < 2 && ObjectCount(CAML) == 0)
	{
		g_iPhase = 2;
		StoryMessage("The caravan is lost beneath the dunes.");
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
			StoryMessage("The sky turns ochre. The storm is on you!");
			LaunchWeatherEvent(SNDT, 50, 1400);
		}
		if (g_iRoute == 2) AmbushCheck();
	}
	return FX_OK;
}

// Arrival callback (GameCall from the caravan puppet, Camel.c4d).
// global, not private: the puppet's global func in Camel.c4d resolves
// GameCall targets through the engine global map (func-map lesson).
global func SaltRoadCaravanArrived(object camel)
{
	g_iArrived++;
	if (g_iArrived >= ObjectCount(CAML))
	{
		g_iPhase = 2;
		StoryMessage("Act II complete: the caravan is across the dune sea.");
		SetNextMission("SaltRoad.c4f\\SaltRoad03.c4s", "Act III: The Queen's Toll", "The narrows, and what waits there.");
		GameOver();
	}
	return true;
}
