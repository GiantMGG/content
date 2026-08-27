/*-- Stormwatch -- five-storm coastal showcase. --*/

#strict 2

static g_fInitializedPlayers;
static g_iChapter;

// Stub — full CrisisDirector implementation lands in Task 2.
protected func Initialize()
{
	g_iChapter = 0;
	return true;
}

func InitializePlayer(iPlr)
{
	if (g_fInitializedPlayers) return;
	g_fInitializedPlayers = 1;
	return true;
}
