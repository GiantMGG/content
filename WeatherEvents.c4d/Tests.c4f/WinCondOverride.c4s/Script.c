/*-- WinCondOverride.c4s — headless winning-condition --parameter override test. --*/

#strict 2

protected func Initialize()
{
	AddEffect("RunTest", 0, 1, 35);
	return true;
}

global func FxRunTestTimer(target, effect, time)
{
	var pGoal;
	if (ObjectCount(C4Id("VALG")) != 1)
		FatalError("WinCondOverride FAIL: expected exactly 1 VALG goal object (25 placed objects must merge to one)");
	if (!(pGoal = FindObject(C4Id("VALG"))))
		FatalError("WinCondOverride FAIL: no VALG goal object found");
	if (LocalN("TargetScore", pGoal) != 2500)
		FatalError("WinCondOverride FAIL: VALG TargetScore is not 2500 (the 25x100 merge contract)");
	if (ObjectCount(C4Id("GLDM")) != 0)
		FatalError("WinCondOverride FAIL: baseline GLDM goal was not removed by the CooperativeGoal family swap");
	if (ObjectCount(C4Id("MNTK")) != 0)
		FatalError("WinCondOverride FAIL: MNTK must not be present");
	if (ObjectCount(C4Id("KILC")) != 1)
		FatalError("WinCondOverride FAIL: expected exactly 1 KILC rule object");
	if (ObjectCount(C4Id("FGRV")) != 0)
		FatalError("WinCondOverride FAIL: KillTheCaptain must not pull FGRV");
	if (ObjectCount(C4Id("ENRG")) != 1)
		FatalError("WinCondOverride FAIL: unrelated ENRG rule was not preserved (surgical semantics)");
	Log("WinCondOverride PASS");
	GameOver();
	return -1;
}
