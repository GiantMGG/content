/*-- WinCondMode.c4s — headless Mode=--parameter override test (additive melee). --*/

#strict 2

protected func Initialize()
{
	AddEffect("RunTest", 0, 1, 35);
	return true;
}

global func FxRunTestTimer(target, effect, time)
{
	var pGoal;
	if (ObjectCount(C4Id("MELE")) != 1)
		FatalError("WinCondMode FAIL: Mode=Melee did not add exactly 1 MELE goal object");
	if (ObjectCount(C4Id("VALG")) != 1)
		FatalError("WinCondMode FAIL: authored VALG goal objects did not merge to one");
	if (!(pGoal = FindObject(C4Id("VALG"))))
		FatalError("WinCondMode FAIL: no VALG goal object found");
	if (LocalN("TargetScore", pGoal) != 300)
		FatalError("WinCondMode FAIL: authored VALG count 3 must survive the mode switch (TargetScore 300)");
	Log("WinCondMode PASS");
	GameOver();
	return -1;
}
