/*-- OptionsOverride.c4s — headless --parameter override test. --*/

#strict 2

static const TEAMDIST_Random = 3;  // C4TeamList::TEAMDIST_Random (C4Teams.h:123)

protected func Initialize()
{
	AddEffect("RunTest", 0, 1, 35);
	return true;
}

global func FxRunTestTimer(target, effect, time)
{
	if (GetTeamConfig(TEAM_Dist) != TEAMDIST_Random)
		FatalError("OptionsOverride FAIL: TeamDist override not applied");
	if (!GetTeamConfig(TEAM_TeamColors))
		FatalError("OptionsOverride FAIL: TeamColors override not applied");
	Log("OptionsOverride PASS");
	GameOver();
	return -1;
}
