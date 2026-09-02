/*-- ScorpionArtSmoke.c4s — cycle-93 art wiring pin (spec             --*/
/* hostile-object-bugfix). Synchronous RunSmokeSteps() pattern        --*/
/* (DesertSmoke/ScorpionStagingRepro norm).                           --*/
#strict 3

protected func Initialize()
{
	RunSmokeSteps();
	return true;
}

func RunSmokeSteps()
{
	/* Step 0: def loads + Picture rect is the phase-0 sprite. */
	var scorp = CreateObject(SCRP, 50, 30, NO_OWNER);
	if (!scorp)
		FatalError("ScorpionArtSmoke FAIL step 0: SCRP not spawned");
	if (GetDefCoreVal("Picture", "DefCore", SCRP, 0) != 0
	 || GetDefCoreVal("Picture", "DefCore", SCRP, 1) != 0
	 || GetDefCoreVal("Picture", "DefCore", SCRP, 2) != 20
	 || GetDefCoreVal("Picture", "DefCore", SCRP, 3) != 12)
		FatalError("ScorpionArtSmoke FAIL step 0: Picture mismatch");

	/* Step 1: Walk wiring — Length=2, facet/dirs/delay unchanged. */
	if (GetActMapVal("Length", "Walk", SCRP) != 2)
		FatalError("ScorpionArtSmoke FAIL step 1: Walk Length != 2");
	if (GetActMapVal("Directions", "Walk", SCRP) != 2)
		FatalError("ScorpionArtSmoke FAIL step 1: Walk Directions != 2");
	if (GetActMapVal("FlipDir", "Walk", SCRP) != 1)
		FatalError("ScorpionArtSmoke FAIL step 1: Walk FlipDir != 1");
	if (GetActMapVal("Delay", "Walk", SCRP) != 2)
		FatalError("ScorpionArtSmoke FAIL step 1: Walk Delay != 2");
	if (GetActMapVal("Facet", "Walk", SCRP, 0) != 0
	 || GetActMapVal("Facet", "Walk", SCRP, 1) != 0
	 || GetActMapVal("Facet", "Walk", SCRP, 2) != 20
	 || GetActMapVal("Facet", "Walk", SCRP, 3) != 12)
		FatalError("ScorpionArtSmoke FAIL step 1: Walk Facet mismatch");

	/* Step 2: phase round-trip, both dirs. */
	scorp->SetAction("Walk");
	var d = 0;
	while (d < 2)
	{
		scorp->SetDir(d);
		scorp->SetPhase(0);
		if (scorp->GetPhase() != 0)
			FatalError("ScorpionArtSmoke FAIL step 2: phase 0 round-trip");
		scorp->SetPhase(1);
		if (scorp->GetPhase() != 1)
			FatalError("ScorpionArtSmoke FAIL step 2: phase 1 round-trip");
		d++;
	}

	Log("ScorpionArtSmoke PASS");
	GameOver();
	return true;
}
