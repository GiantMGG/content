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
static g_fInitializedPlayers;

local const g_SiegeEngines = [SCAT, TRBT, BRAM];

protected func Initialize()
{
	g_fInitialized = 0;
	g_fInitializedPlayers = 0;
	g_iTimeRemaining = 600;  // 10 min
	BuildCastle();
	SpawnAttackerEngines();
	SpawnAmmoPiles();
	AddEffect("SiegeDirector", this, 1, 35, this);
	return true;
}

protected func InitializePlayer(int iPlr)
{
	if (g_fInitializedPlayers) return;
	g_fInitializedPlayers = 1;
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
	CreateConstruction(BOIL, iCX, iCY - 5, -1, 100, 1);

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

func EliminateLosers(int iLosingTeam)
{
	for (var i = 0; i < GetPlayerCount(); ++i)
	{
		var iPlr = GetPlayerByIndex(i);
		if (GetPlayerTeam(iPlr) == iLosingTeam)
			EliminatePlayer(iPlr);
	}
	return true;
}

// --- SiegeDirector effect ---

func FxSiegeDirectorStart(object target, int effect, int temp)
{
	if (temp) return;
	return 1;
}

func FxSiegeDirectorTimer(object target, int effect, int timer)
{
	// Tick once per second (35-frame interval ~= 1s).
	if (timer % 35 == 0) --g_iTimeRemaining;

	var pKing = FindObject(KING);
	if (!pKing || !GetAlive(pKing))
	{
		Log("$MsgAttackersWin$");
		EliminateLosers(2);  // eliminate defenders
		GameOver();
		return -1;
	}
	if (g_iTimeRemaining <= 0)
	{
		Log("$MsgDefendersWin$");
		EliminateLosers(1);  // eliminate attackers
		GameOver();
		return -1;
	}
	var iEnginesLeft = 0;
	for (var id in g_SiegeEngines)
		if (FindObject(id)) ++iEnginesLeft;
	if (iEnginesLeft == 0)
	{
		Log("$MsgDefendersWin$");
		EliminateLosers(1);
		GameOver();
		return -1;
	}
	return 1;
}
