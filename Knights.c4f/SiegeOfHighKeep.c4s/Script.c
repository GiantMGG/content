/*-- Siege of High Keep -- asymmetric castle siege --*/

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
	// (per-team crew/equipment assignment handled here)
	return true;
}

// (helpers + SiegeDirector timer land in Task 3)
