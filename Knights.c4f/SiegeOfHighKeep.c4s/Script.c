/*-- Siege of High Keep -- asymmetric castle siege --*/
/*                                                                          */
/* Pure C4Script content. No engine files are touched. See                  */
/* specs/2026-08-27-1500-castle-siege-scenario.md.                          */
/*                                                                          */
/* Three attackers (KNIG x3 each) besiege a pre-built elevated stone        */
/* castle defended by 1 player with a KING (whose death = attacker          */
/* victory) plus 3 KNIG bodyguards. The SiegeDirector effect tracks         */
/* King-alive / timer-remaining / engine-count and calls GameOver() on      */
/* any win condition.                                                       */

#strict 2

static g_iTimeRemaining;   // in seconds
static g_fInitialized;
static const SIEGE_TIME_LIMIT = 300;  // calibrated cycle 181, .opencode/scratch/181/pacing

static const g_SiegeEngines0 = SCAT;
static const g_SiegeEngines1 = TRBT;
static const g_SiegeEngines2 = BRAM;

protected func Initialize()
{
	g_fInitialized = 0;
	g_iTimeRemaining = SIEGE_TIME_LIMIT;
	BuildCastle();
	SpawnAttackerEngines();
	SpawnAmmoPiles();
	AddEffect("SiegeDirector", this, 1, 35, this);
	return true;
}

protected func InitializePlayer(int iPlr)
{
	// Per-side briefing (Teams.txt binds player slots to sides):
	// player 0 rides [Player1] (attackers, team 1), player 1 rides
	// [Player2] (defenders, team 2). Late joiners swell the attackers.
	if (iPlr == 1)
	{
		PlayerMessage(iPlr, "$MsgIntroDefense$");
		Log("$MsgIntroDefense$");
	}
	else
	{
		PlayerMessage(iPlr, "$MsgIntro$");
		Log("$MsgIntro$");
	}
	return true;
}

// --- Castle pre-placement (mirrors Hammerfest.c4s + Castle.c4s) ---

func BuildCastle()
{
	// Castle sits on the plateau (right ~60% of map).
	var iCX = LandscapeWidth() * 70 / 100;
	var iCY = LandscapeHeight() * 35 / 100;

	// Rectangular ring of CPW2 walls (4 segments) + 2 CPT2 corner towers
	// flanking the west-facing SGAT gate.
	CreateConstruction(CPW2, iCX - 40, iCY,      -1, 100, 1);  // north wall
	CreateConstruction(CPW2, iCX + 40, iCY,      -1, 100, 1);  // south wall
	CreateConstruction(CPW2, iCX,      iCY - 30, -1, 100, 1);  // east wall
	CreateConstruction(CPT2, iCX - 40, iCY,      -1, 100, 1);  // NW tower
	CreateConstruction(CPT2, iCX + 40, iCY,      -1, 100, 1);  // SW tower
	CreateConstruction(SGAT, iCX,      iCY,      -1, 100, 1);  // west gate

	// King's keep (CST3) in the interior courtyard.
	CreateConstruction(CST3, iCX, iCY - 10, -1, 100, 1);

	// BoilingOilCauldron pre-placed on the gatehouse battlement.
	// iCY-85: the construction seats itself 13 px at creation, so the
	// cauldron rests at iCY-98 -- the battlement shelf above the west gate
	// (verified headless: BOIL GetY stays at iCY-98 for 100+ frames; the
	// old iCY-5 placement had no collision and fell out of the world).
	CreateConstruction(BOIL, iCX, iCY - 85, -1, 100, 1);

	CreateObject(KING, iCX, iCY - 20, 1);

	return true;
}

func SpawnAttackerEngines()
{
	// Three siege engines pre-placed on the attacker side (west, ground level).
	var iAX = LandscapeWidth() * 20 / 100;
	var iAY = LandscapeHeight() * 80 / 100;
	CreateObject(SCAT, iAX,      iAY, NO_OWNER);
	CreateObject(TRBT, iAX + 30, iAY, NO_OWNER);
	CreateObject(BRAM, iAX + 60, iAY, NO_OWNER);
	return true;
}

func SpawnAmmoPiles()
{
	// Ammo piles (SROK / FPOT / SBLD / BOMB) sit next to each engine.
	var iAX = LandscapeWidth() * 20 / 100;
	var iAY = LandscapeHeight() * 80 / 100;
	for (var i = 0; i < 5; ++i)
	{
		CreateObject(SROK, iAX + 5 + i * 3, iAY + 5, NO_OWNER);
		CreateObject(FPOT, iAX + 35 + i * 3, iAY + 5, NO_OWNER);
		CreateObject(SBLD, iAX + 65 + i * 3, iAY + 5, NO_OWNER);
		CreateObject(BOMB, iAX + 95 + i * 3, iAY + 5, NO_OWNER);
	}
	return true;
}

// Callable from the global-scope SiegeDirector timer effect, which can only
// resolve other `global func`s (cycle-181 finding, see FxSiegeDirectorTimer).
global func EliminateLosers(int iLosingTeam)
{
	for (var i = 0; i < GetPlayerCount(); ++i)
	{
		var iPlr = GetPlayerByIndex(i);
		if (GetPlayerTeam(iPlr) == iLosingTeam)
			EliminatePlayer(iPlr);
	}
	return true;
}

// Mirror the match outcome onto the GLST goal object so the round-results
// goals board shows the correct fulfilled state (Goal_Siege). The goal's
// own IsFulfilled is read by the GOAL framework; GameOver stays with the
// director. Guarded: a goal-less game (Goals= not wired) must not crash.
global func RecordSiegeOutcome(int iWinningTeam)
{
	var pGoal = FindObject(GLST);
	if (pGoal) pGoal->~SiegeEnded(iWinningTeam);
	return true;
}

// --- SiegeDirector effect ---
// Cycle-181 finding (director_probe.md): plain-func effect callbacks never
// resolve for AddEffect(name, this, 1, 35, this) — the callback script is
// looked up in the global script engine where only `global func`s live. The
// two callbacks below MUST stay `global func` or the 1v3 timer is dead.

global func FxSiegeDirectorStart(object target, int effect, int temp)
{
	if (temp) return;
	return 1;
}

global func FxSiegeDirectorTimer(object target, int effect, int timer)
{
	// Tick once per second (35-frame interval ~= 1s).
	if (timer % 35 == 0) --g_iTimeRemaining;
	// FIX-1: visible countdown -- one message per full minute crossed
	// (5:00 → 4:00 → 3:00 → 2:00 → 1:00), on-screen and in the log.
	// Message's 2nd param is the target object; 0 = global message.
	// (nil is a strict-3+ keyword; this scenario is #strict 2.)
	if (timer % 35 == 0 && g_iTimeRemaining > 0 && g_iTimeRemaining % 60 == 0)
	{
		if (g_iTimeRemaining == 60)
		{
			Message("$MsgTimeLeftOne$");
			Log("$MsgTimeLeftOne$");
		}
		else
		{
			Message("$MsgTimeLeft$", 0, g_iTimeRemaining / 60);
			Log("$MsgTimeLeft$", g_iTimeRemaining / 60);
		}
	}

	var pKing = FindObject(KING);
	if (!pKing || !GetAlive(pKing))
	{
		Log("$MsgAttackersWin$");
		Message("$MsgAttackersWin$");
		RecordSiegeOutcome(1);  // attackers' goal fulfilled
		EliminateLosers(2);  // eliminate defenders
		GameOver();
		return -1;
	}
	if (g_iTimeRemaining <= 0)
	{
		Log("$MsgTimeUp$");
		Message("$MsgTimeUp$");
		Log("$MsgDefendersWin$");
		Message("$MsgDefendersWin$");
		RecordSiegeOutcome(2);  // defenders' goal fulfilled
		EliminateLosers(1);  // eliminate attackers
		GameOver();
		return -1;
	}
	var iEnginesLeft = 0;
	if (FindObject(g_SiegeEngines0)) ++iEnginesLeft;
	if (FindObject(g_SiegeEngines1)) ++iEnginesLeft;
	if (FindObject(g_SiegeEngines2)) ++iEnginesLeft;
	if (iEnginesLeft == 0)
	{
		Log("$MsgEnginesDestroyed$");
		Message("$MsgEnginesDestroyed$");
		Log("$MsgDefendersWin$");
		Message("$MsgDefendersWin$");
		RecordSiegeOutcome(2);  // defenders' goal fulfilled
		EliminateLosers(1);
		GameOver();
		return -1;
	}
	return 1;
}
