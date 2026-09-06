/*-- DatePalmArtSmoke.c4s — cycle-100 art wiring pin (spec            --*/
/* placeholder-art-debt-drive-down). Synchronous RunSmokeSteps()      --*/
/* pattern (DesertSmoke/ScorpionArtSmoke norm).                       --*/
#strict 3

protected func Initialize()
{
	RunSmokeSteps();
	return true;
}

func RunSmokeSteps()
{
	/* Step 0: def loads + Picture rect is the Ready facet. */
	var palm = CreateObject(DATP, 50, 30, NO_OWNER);
	if (!palm)
		FatalError("DatePalmArtSmoke FAIL step 0: DATP not spawned");
	if (GetDefCoreVal("Picture", "DefCore", DATP, 0) != 40
	 || GetDefCoreVal("Picture", "DefCore", DATP, 1) != 0
	 || GetDefCoreVal("Picture", "DefCore", DATP, 2) != 20
	 || GetDefCoreVal("Picture", "DefCore", DATP, 3) != 36)
		FatalError("DatePalmArtSmoke FAIL step 0: Picture mismatch");

	/* Step 1: Seedling wiring — facet, Length, Directions, Delay. */
	if (GetActMapVal("Facet", "Seedling", DATP, 0) != 0
	 || GetActMapVal("Facet", "Seedling", DATP, 1) != 0
	 || GetActMapVal("Facet", "Seedling", DATP, 2) != 20
	 || GetActMapVal("Facet", "Seedling", DATP, 3) != 36)
		FatalError("DatePalmArtSmoke FAIL step 1: Seedling Facet mismatch");
	if (GetActMapVal("Length", "Seedling", DATP) != 1)
		FatalError("DatePalmArtSmoke FAIL step 1: Seedling Length != 1");
	if (GetActMapVal("Directions", "Seedling", DATP) != 1)
		FatalError("DatePalmArtSmoke FAIL step 1: Seedling Directions != 1");
	if (GetActMapVal("Delay", "Seedling", DATP) != 10)
		FatalError("DatePalmArtSmoke FAIL step 1: Seedling Delay != 10");

	/* Step 1b: Growing wiring. */
	if (GetActMapVal("Facet", "Growing", DATP, 0) != 20
	 || GetActMapVal("Facet", "Growing", DATP, 1) != 0
	 || GetActMapVal("Facet", "Growing", DATP, 2) != 20
	 || GetActMapVal("Facet", "Growing", DATP, 3) != 36)
		FatalError("DatePalmArtSmoke FAIL step 1b: Growing Facet mismatch");
	if (GetActMapVal("Length", "Growing", DATP) != 1)
		FatalError("DatePalmArtSmoke FAIL step 1b: Growing Length != 1");
	if (GetActMapVal("Directions", "Growing", DATP) != 1)
		FatalError("DatePalmArtSmoke FAIL step 1b: Growing Directions != 1");
	if (GetActMapVal("Delay", "Growing", DATP) != 10)
		FatalError("DatePalmArtSmoke FAIL step 1b: Growing Delay != 10");

	/* Step 1c: Ready wiring. */
	if (GetActMapVal("Facet", "Ready", DATP, 0) != 40
	 || GetActMapVal("Facet", "Ready", DATP, 1) != 0
	 || GetActMapVal("Facet", "Ready", DATP, 2) != 20
	 || GetActMapVal("Facet", "Ready", DATP, 3) != 36)
		FatalError("DatePalmArtSmoke FAIL step 1c: Ready Facet mismatch");
	if (GetActMapVal("Length", "Ready", DATP) != 1)
		FatalError("DatePalmArtSmoke FAIL step 1c: Ready Length != 1");
	if (GetActMapVal("Directions", "Ready", DATP) != 1)
		FatalError("DatePalmArtSmoke FAIL step 1c: Ready Directions != 1");
	if (GetActMapVal("Delay", "Ready", DATP) != 10)
		FatalError("DatePalmArtSmoke FAIL step 1c: Ready Delay != 10");

	/* Step 2: stage round-trip — SetAction sticks for all 3 stages. */
	palm->SetAction("Seedling");
	if (palm->GetAction() != "Seedling")
		FatalError("DatePalmArtSmoke FAIL step 2: Seedling round-trip");
	palm->SetAction("Growing");
	if (palm->GetAction() != "Growing")
		FatalError("DatePalmArtSmoke FAIL step 2: Growing round-trip");
	palm->SetAction("Ready");
	if (palm->GetAction() != "Ready")
		FatalError("DatePalmArtSmoke FAIL step 2: Ready round-trip");

	Log("DatePalmArtSmoke PASS");
	GameOver();
	return true;
}
