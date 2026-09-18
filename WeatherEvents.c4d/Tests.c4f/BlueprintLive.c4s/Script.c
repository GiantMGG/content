/*-- BlueprintLive.c4s -- headless live-round test of the PlrStart Knowledge
     override chain (roadmap blueprints-live-round).

  Proves: a setup-time Knowledge edit (--parameter Knowledge=WMIL=1 replaces
  the scenario's base [Player1] knowledge list) reaches the live round — the
  joining player knows WMIL (GetPlrKnowledge) and can actually build it
  (CreateConstruction completes at 100%). No "Smoke" suffix on purpose: the
  tests/CMakeLists.txt auto-glob must not double-register this scenario. --*/

#strict 2

protected func Initialize()
{
	AddEffect("RunTest", 0, 1, 35);
	return true;
}

global func FxRunTestTimer(target, effect, time)
{
	var plr, clonk, pMill;

	// Step 1: the live round must have the joining player at all ...
	plr = GetPlayerByIndex(0);
	if (plr == NO_OWNER)
		FatalError("BlueprintLive FAIL: no player 0 in the live round");

	// ... and the override's knowledge must be what THAT player knows.
	if (!GetPlrKnowledge(plr, WMIL))
		FatalError("BlueprintLive FAIL: WMIL knowledge did not reach the live round");

	// ... on an existing crew clonk.
	clonk = GetCrew(plr);
	if (!clonk)
		FatalError("BlueprintLive FAIL: player has no crew clonk");

	// The windmill must be buildable: 100% completion = built outright.
	pMill = clonk->CreateConstruction(WMIL, 0, 0, plr, 100, true, false);
	if (!pMill)
		FatalError("BlueprintLive FAIL: CreateConstruction(WMIL) refused");
	if (pMill->GetID() != WMIL)
		FatalError(Format("BlueprintLive FAIL: built object id %s != WMIL", C4IdText(pMill->GetID())));

	Log("BlueprintLive PASS");
	GameOver();
	return -1;
}
