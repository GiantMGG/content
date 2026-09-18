/*-- CogArtSmoke.c4s — cycle-144 art wiring pin (spec art-pipeline-v3).    --*/
/*-- Synchronous RunSmokeSteps() pattern (SeaTradeSmoke norm; AddEffect    --*/
/*-- timers do NOT fire in the smoke harness). Asserts the CGSH Sailing    --*/
/*-- facet (0,0,60,40) Delay 20 StartCall=Wind2Sail, Picture (0,0,64,64),   --*/
/*-- and that a fresh cog spawns already in the Sailing action.            --*/

#strict 2

protected func Initialize()
{
	RunSmokeSteps();
	return true;
}

func RunSmokeSteps()
{
	/* Step 0: CGSH spawns + Picture stays in-bounds (0,0,64,64). */
	var pCog = CreateObject(CGSH, 50, 30, NO_OWNER);
	if (!pCog)
		FatalError("CogArtSmoke FAIL step 0: CGSH not spawned");
	if (GetDefCoreVal("Picture", "DefCore", CGSH, 0) != 0
	 || GetDefCoreVal("Picture", "DefCore", CGSH, 1) != 0
	 || GetDefCoreVal("Picture", "DefCore", CGSH, 2) != 64
	 || GetDefCoreVal("Picture", "DefCore", CGSH, 3) != 64)
		FatalError("CogArtSmoke FAIL step 0: Picture mismatch");

	/* Step 1: Sailing facet (0,0,60,40) + Delay 20 + StartCall=Wind2Sail. */
	if (GetActMapVal("Facet", "Sailing", CGSH, 0) != 0
	 || GetActMapVal("Facet", "Sailing", CGSH, 1) != 0
	 || GetActMapVal("Facet", "Sailing", CGSH, 2) != 60
	 || GetActMapVal("Facet", "Sailing", CGSH, 3) != 40)
		FatalError("CogArtSmoke FAIL step 1: Sailing Facet mismatch");
	if (GetActMapVal("Delay", "Sailing", CGSH) != 20)
		FatalError("CogArtSmoke FAIL step 1: Sailing Delay != 20");
	if (GetActMapVal("StartCall", "Sailing", CGSH) != "Wind2Sail")
		FatalError("CogArtSmoke FAIL step 1: Sailing StartCall != Wind2Sail");

	/* Step 2: the cog's Initialize put it straight into Sailing. */
	if (pCog->GetAction() != "Sailing")
		FatalError("CogArtSmoke FAIL step 2: new cog not in Sailing action");

	Log("CogArtSmoke PASS");
	GameOver();
}
