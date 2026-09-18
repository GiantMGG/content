/*-- WindmillArtSmoke.c4s — cycle-144 art wiring pin (spec                 --*/
/*-- art-pipeline-v3). Synchronous RunSmokeSteps() pattern                 --*/
/*-- (ScorpionArtSmoke/AgricultureArtSmoke norm). Asserts the AGWM/AGWG    --*/
/*-- Picture rects, per-action facets + Length/Delay, and the wing's       --*/
/*-- Turn phase round-trip (SetPhase 0->3).                                --*/

#strict 2

protected func Initialize()
{
	RunSmokeSteps();
	return true;
}

func AssertPicture(id idDef, int iPx, int iPy, int iPw, int iPh, string szDef)
{
	if (GetDefCoreVal("Picture", "DefCore", idDef, 0) != iPx
	 || GetDefCoreVal("Picture", "DefCore", idDef, 1) != iPy
	 || GetDefCoreVal("Picture", "DefCore", idDef, 2) != iPw
	 || GetDefCoreVal("Picture", "DefCore", idDef, 3) != iPh)
		FatalError(Format("WindmillArtSmoke FAIL: %s Picture mismatch", szDef));
}

func AssertFacet(id idDef, string szDef, string szAction, int iFx, int iFy, int iFw, int iFh)
{
	if (GetActMapVal("Facet", szAction, idDef, 0) != iFx
	 || GetActMapVal("Facet", szAction, idDef, 1) != iFy
	 || GetActMapVal("Facet", szAction, idDef, 2) != iFw
	 || GetActMapVal("Facet", szAction, idDef, 3) != iFh)
		FatalError(Format("WindmillArtSmoke FAIL: %s %s Facet mismatch", szDef, szAction));
}

func RunSmokeSteps()
{
	/* Step 0: AGWM spawns + Picture points at the 28x58 idle facet. */
	var pMill = CreateObject(AGWM, 50, 30, NO_OWNER);
	if (!pMill)
		FatalError("WindmillArtSmoke FAIL step 0: AGWM not spawned");
	AssertPicture(AGWM, 0, 0, 28, 58, "AGWM");

	/* Step 1: AGWM Idle wiring — facet (0,0,28,58), Length 1, Delay 10. */
	AssertFacet(AGWM, "AGWM", "Idle", 0, 0, 28, 58);
	if (GetActMapVal("Length", "Idle", AGWM) != 1)
		FatalError("WindmillArtSmoke FAIL step 1: Idle Length != 1");
	if (GetActMapVal("Delay", "Idle", AGWM) != 10)
		FatalError("WindmillArtSmoke FAIL step 1: Idle Delay != 10");

	/* Step 2: AGWM Grinding band — facet (0,58,28,58), Length 3, Delay 10. */
	AssertFacet(AGWM, "AGWM", "Grinding", 0, 58, 28, 58);
	if (GetActMapVal("Length", "Grinding", AGWM) != 3)
		FatalError("WindmillArtSmoke FAIL step 2: Grinding Length != 3");
	if (GetActMapVal("Delay", "Grinding", AGWM) != 10)
		FatalError("WindmillArtSmoke FAIL step 2: Grinding Delay != 10");

	/* Step 3: AGWG wing spawns + Picture stays (0,0,40,40). */
	var pWing = CreateObject(AGWG, 40, 30, NO_OWNER);
	if (!pWing)
		FatalError("WindmillArtSmoke FAIL step 3: AGWG not spawned");
	AssertPicture(AGWG, 0, 0, 40, 40, "AGWG");

	/* Step 4: AGWG Turn wiring — facet (0,0,40,40), Length 4, Delay 2. */
	AssertFacet(AGWG, "AGWG", "Turn", 0, 0, 40, 40);
	if (GetActMapVal("Length", "Turn", AGWG) != 4)
		FatalError("WindmillArtSmoke FAIL step 4: Turn Length != 4");
	if (GetActMapVal("Delay", "Turn", AGWG) != 2)
		FatalError("WindmillArtSmoke FAIL step 4: Turn Delay != 2");

	/* Step 5: wing phase round-trip SetPhase 0->3 (identical rotor phases;
	   engine-side Wind2Turn -> SetRDir is the animator). */
	pWing->SetAction("Turn");
	var p = 0;
	while (p < 4)
	{
		pWing->SetPhase(p);
		if (pWing->GetPhase() != p)
			FatalError("WindmillArtSmoke FAIL step 5: wing phase round-trip");
		p++;
	}

	Log("WindmillArtSmoke PASS");
	GameOver();
	return true;
}
