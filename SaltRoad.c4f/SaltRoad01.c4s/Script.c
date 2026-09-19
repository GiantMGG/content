/*-- Salt Road, Act I: The Waiting Storm (cycle 146). --*/
/* Provision the failing oasis -- dates, sandstone, water -- then    */
/* hold it against the dusk scorpion raid. When the raid breaks, the */
/* caravan rides east and hands off to Act II (SetNextMission).      */

#strict 2

static g_iPhase;       // 0 prep, 1 raid, 2 caravan, 3 done
static g_iRaidWaves;   // raid waves launched so far
static g_iRaidClock;   // director ticks since the last wave
static g_iRaidKills;   // kill-confirmed scorpion deaths since the raid began
static g_iRaidLastLive;// live SCRP count at the last director tick (kill-watch baseline)
static g_iRaidSpawned; // scorpions spawned by the scripted waves
static g_iRaidStall;   // grace clock: director ticks with no kill after the last wave
static g_iArrived;     // camels through the departure gate

static const SALTROAD1_DATES  = 6;    // dates stocked at camp
static const SALTROAD1_BLOCKS = 4;    // SNDS braced at the well
static const SALTROAD1_DUSK   = 2100; // dusk in frames (~60 director steps; effect `time` counts frames, DuneBurialSmoke `time>=175` precedent)
static const SALTROAD1_WAVES  = 4;    // raid waves per night
static const SALTROAD1_STALL  = 45;   // grace bound (director ticks, ~40s): re-command any raiders back to the camp
static const SALTROAD1_STALL_HARD = 90;  // hard bound (~80s): no last raider in reach, advance and log it

protected func Initialize()
{
	g_iPhase = 0;
	g_iRaidWaves = 0;
	g_iRaidClock = 0;
	g_iRaidKills = 0;
	g_iRaidLastLive = 0;
	g_iRaidSpawned = 0;
	g_iRaidStall = 0;
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
	StoryMessage("$MsgIntro$");
	// Repeat-self button (Tutorial01 pattern).
	SetNextMission("SaltRoad.c4f\\SaltRoad01.c4s", "$BtnReplayActI$", "$BtnReplayActIDesc$");
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
	// Win re-check (review F1): the win condition used to live only in the
	// arrival callback -- if the second camel was killed in transit, no
	// further callback ever fired and the act soft-locked forever. Poll it
	// here every director tick: some camel arrived AND every surviving
	// camel has arrived => the crossing is complete.
	if (g_iPhase < 3 && g_iArrived > 0 && g_iArrived >= ObjectCount(CAML))
	{
		g_iPhase = 3;
		return CaravanComplete();
	}

	// Defeat watch: no camels left means no caravan, no Salt Road.
	if (g_iPhase < 3 && ObjectCount(CAML) == 0)
	{
		g_iPhase = 3;
		StoryMessage("$MsgDefeat$");
		GameOver();
		return FX_OK;
	}

	if (g_iPhase == 0)
	{
		if (time >= SALTROAD1_DUSK)
		{
			g_iPhase = 1;
			g_iRaidClock = 0;
			StoryMessage("$MsgDusk$");
			// Sky-fade warning pulse (short, low intensity).
			LaunchWeatherEvent(SNDT, 40, 350);
		}
	}
	else if (g_iPhase == 1)
	{
		g_iRaidClock += 35;
		var live = ObjectCount(SCRP);

		// Kill-confirmation watch (review F2): count confirmed scorpion
		// deaths since the raid began, as the fall in the on-map count
		// since the last tick. A same-tick scripted wave spawn net-zeroes;
		// the baseline is re-snapshotted after every spawn below.
		if (live < g_iRaidLastLive)
		{
			g_iRaidKills += g_iRaidLastLive - live;
			g_iRaidStall = 0;   // progress: reset the stall grace clock
		}
		g_iRaidLastLive = live;

		if (g_iRaidWaves < SALTROAD1_WAVES && g_iRaidClock >= 180 && live < 4)
		{
			var rx = LandscapeWidth() / 2;
			g_iRaidSpawned += SaltRoad_SpawnRaid(rx, GroundY(rx) - 40, 4 - live);
			g_iRaidWaves++;
			g_iRaidClock = 0;
			g_iRaidLastLive = ObjectCount(SCRP);  // post-spawn baseline
		}

		// Raid-clear gate (review F2): the old `ObjectCount(SCRP) == 0`
		// test stalled forever when one scorpion got trapped in terrain the
		// player cannot reach (quarry pit, oasis basin, outcrop pocket).
		// Advance on a kill-confirmed counter -- every scripted raider fell
		// -- with a bounded grace fallback for unreachable remainder.
		if (g_iRaidWaves >= SALTROAD1_WAVES && PrepGoalFilled())
		{
			if (g_iRaidKills >= g_iRaidSpawned)
			{
				g_iRaidStall = 0;
				CaravanDeparts();
			}
			else
			{
				g_iRaidStall++;
				if (g_iRaidStall >= SALTROAD1_STALL)
					RecommandRaiders();
				if (g_iRaidStall >= SALTROAD1_STALL_HARD)
				{
					Log(Format("Salt Road Act I: raid cleared by grace period -- %d/%d raiders confirmed dead, the remainder unreachable. Caravan departs.", g_iRaidKills, g_iRaidSpawned));
					CaravanDeparts();
				}
			}
		}
		else
		{
			g_iRaidStall = 0;
		}
	}
	return FX_OK;
}

// Global, not private: bare-name called from the global effect director
// FxSaltRoadActTimer (func-map lesson, FirstLight.c4s precedent).
global func CaravanDeparts()
{
	g_iPhase = 2;
	StartCaravan();
	StoryMessage("$MsgRaidBroken$");
	return true;
}

// The raid broke, but a stray scorpion may be stuck somewhere the player
// cannot even see. Re-command every survivor to Attack the nearest target
// so terrain-trapped raiders path out of their pocket (review F2).
global func RecommandRaiders()
{
	var scorpion;
	for (scorpion in FindObjects(Find_ID(SCRP)))
	{
		var victim = SaltRoad_NearestVictim(GetX(scorpion), GetY(scorpion));
		if (victim) SetCommand(scorpion, "Attack", victim);
	}
	return true;
}

// global, not private: bare-name called from the global effect director
// FxSaltRoadActTimer (func-map lesson, FirstLight.c4s precedent).
global func StartCaravan()
{
	var camel;
	for (camel in FindObjects(Find_ID(CAML)))
		SaltRoad_StartCaravan(camel, [LandscapeWidth() - 120]);
}

// The single Act-I victory: message, next-mission button and game over.
// Called from the arrival callback and from the director's win re-check
// (review F1), so a straggler death after the first arrival still ends
// the act. global, not private: bare-name called from the global director.
global func CaravanComplete()
{
	g_iPhase = 3;
	StoryMessage("$MsgActComplete$");
	SetNextMission("SaltRoad.c4f\\SaltRoad02.c4s", "$BtnNextActII$", "$BtnNextActIIDesc$");
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
