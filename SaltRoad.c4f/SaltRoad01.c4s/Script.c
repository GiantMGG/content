/*-- Salt Road, Act I: The Waiting Storm (cycle 146). --*/
/* Provision the failing oasis -- dates, sandstone, water -- then    */
/* hold it against the dusk scorpion raid. When the raid breaks, the */
/* caravan rides east and hands off to Act II (SetNextMission).      */

#strict 2

static g_iPhase;       // 0 prep, 1 raid, 2 caravan, 3 done
static g_iRaidWaves;   // raid waves launched so far
static g_iRaidClock;   // director ticks since the last wave
static g_iArrived;     // camels through the departure gate

static const SALTROAD1_DATES  = 6;    // dates stocked at camp
static const SALTROAD1_BLOCKS = 4;    // SNDS braced at the well
static const SALTROAD1_DUSK   = 2100; // dusk in frames (~60 director steps; effect `time` counts frames, DuneBurialSmoke `time>=175` precedent)
static const SALTROAD1_WAVES  = 4;    // raid waves per night

protected func Initialize()
{
	g_iPhase = 0;
	g_iRaidWaves = 0;
	g_iRaidClock = 0;
	g_iArrived = 0;

	// The caravan-to-be: two camels at the camp.
	CreateObject(CAML, 140, GroundY(140) - 20, NO_OWNER);
	CreateObject(CAML, 180, GroundY(180) - 20, NO_OWNER);
	// Date palms behind the camp (self-growing: Timer=3540).
	CreateObject(DATP, 90,  GroundY(90)  - 10, NO_OWNER);
	CreateObject(DATP, 115, GroundY(115) - 10, NO_OWNER);
	CreateObject(DATP, 240, GroundY(240) - 10, NO_OWNER);
	// The failing oasis basin and the sandstone quarry (both self-ticking).
	CreateObject(OASS, 220, GroundY(220) - 6, NO_OWNER);
	CreateObject(QRRY, 380, GroundY(380) - 10, NO_OWNER);

	Log("Salt Road Act I: the waiting storm begins.");
	StoryMessage("The oasis is failing. Stock the camp with dates, brace the well with sandstone, and keep the basin wet before dusk.");
	// Repeat-self button (Tutorial01 pattern).
	SetNextMission("SaltRoad.c4f\\SaltRoad01.c4s", "Replay Act I", "The Waiting Storm, once more.");
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

/* ---- prep-goal counters ---- */

global func DatesSecured()
{
	var d, count = 0;
	for (d in FindObjects(Find_ID(DATE), Find_AtRect(40, 0, 320, LandscapeHeight())))
		count++;
	return count;
}

global func BlocksBraced()
{
	var b, count = 0;
	for (b in FindObjects(Find_ID(SNDS), Find_AtRect(180, 0, 90, LandscapeHeight())))
		count++;
	return count;
}

global func BasinFilled()
{
	var oass = FindObject(OASS);
	if (!oass) return false;
	return GetMaterial(GetX(oass) - 2, GetY(oass) - 2) == Material("Water");
}

global func PrepGoalFilled()
{
	return DatesSecured() >= SALTROAD1_DATES
	    && BlocksBraced() >= SALTROAD1_BLOCKS
	    && BasinFilled();
}

/* ---- the director ---- */

global func FxSaltRoadActTimer(target, effect, time)
{
	// Defeat watch: no camels left means no caravan, no Salt Road.
	if (g_iPhase < 3 && ObjectCount(CAML) == 0)
	{
		g_iPhase = 3;
		StoryMessage("The camels are lost. The Salt Road ends here.");
		GameOver();
		return FX_OK;
	}

	if (g_iPhase == 0)
	{
		if (time >= SALTROAD1_DUSK)
		{
			g_iPhase = 1;
			g_iRaidClock = 0;
			StoryMessage("Dusk. The sand stirs -- scorpions!");
			// Sky-fade warning pulse (short, low intensity).
			LaunchWeatherEvent(SNDT, 40, 350);
		}
	}
	else if (g_iPhase == 1)
	{
		g_iRaidClock += 35;
		var live = ObjectCount(SCRP);
		if (g_iRaidWaves < SALTROAD1_WAVES && g_iRaidClock >= 180 && live < 4)
		{
			var rx = LandscapeWidth() / 2;
			SaltRoad_SpawnRaid(rx, GroundY(rx) - 40, 4 - live);
			g_iRaidWaves++;
			g_iRaidClock = 0;
		}
		if (g_iRaidWaves >= SALTROAD1_WAVES && ObjectCount(SCRP) == 0 && PrepGoalFilled())
		{
			g_iPhase = 2;
			StartCaravan();
			StoryMessage("The raid is broken. The caravan rides east at dawn.");
		}
	}
	return FX_OK;
}

// global, not private: bare-name called from the global effect director
// FxSaltRoadActTimer (func-map lesson, FirstLight.c4s precedent).
global func StartCaravan()
{
	var camel;
	for (camel in FindObjects(Find_ID(CAML)))
		SaltRoad_StartCaravan(camel, [LandscapeWidth() - 120]);
}

// Arrival callback (GameCall from the caravan puppet, Camel.c4d).
// global, not private: the puppet's global func in Camel.c4d resolves
// GameCall targets through the engine global map (func-map lesson).
global func SaltRoadCaravanArrived(object camel)
{
	g_iArrived++;
	if (g_iArrived >= ObjectCount(CAML))
	{
		StoryMessage("Act I complete: the caravan is provisioned and away.");
		SetNextMission("SaltRoad.c4f\\SaltRoad02.c4s", "Act II: The Dune Sea", "Ride with the caravan into the dune sea.");
		GameOver();
	}
	return true;
}
