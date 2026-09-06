/*-- RulesGoalsOverride.c4s — headless --parameter Rules=/Goals= override test. --*/

#strict 2

protected func Initialize()
{
	AddEffect("RunTest", 0, 1, 35);
	return true;
}

global func FxRunTestTimer(target, effect, time)
{
	if (ObjectCount(C4Id("NMTT")) != 1)
		FatalError("RulesGoalsOverride FAIL: expected exactly 1 NMTT rule object");
	if (ObjectCount(C4Id("MONE")) != 1)
		FatalError("RulesGoalsOverride FAIL: expected exactly 1 MONE goal object");
	if (ObjectCount(C4Id("ENRG")) != 0)
		FatalError("RulesGoalsOverride FAIL: scenario-default ENRG rule was not replaced");
	if (ObjectCount(C4Id("MELE")) != 0)
		FatalError("RulesGoalsOverride FAIL: scenario-default MELE goal was not replaced");
	Log("RulesGoalsOverride PASS");
	GameOver();
	return -1;
}
