/*-- ScorpionArtSmoke.c4s — cycle-125 24x14 six-phase re-pin (spec    --*/
/*-- scorpion-scale-animation-fix). Synchronous RunSmokeSteps()        --*/
/*-- pattern (DesertSmoke/ScorpionStagingSmoke norm).                  --*/
#strict 3

protected func Initialize()
{
	RunSmokeSteps();
	return true;
}

func RunSmokeSteps()
{
	/* Step 0: def loads + Picture rect is the walk phase-0 sprite. */
	var scorp = CreateObject(SCRP, 50, 30, NO_OWNER);
	if (!scorp)
		FatalError("ScorpionArtSmoke FAIL step 0: SCRP not spawned");
	if (GetDefCoreVal("Picture", "DefCore", SCRP, 0) != 0
	 || GetDefCoreVal("Picture", "DefCore", SCRP, 1) != 0
	 || GetDefCoreVal("Picture", "DefCore", SCRP, 2) != 24
	 || GetDefCoreVal("Picture", "DefCore", SCRP, 3) != 14)
		FatalError("ScorpionArtSmoke FAIL step 0: Picture mismatch");

	/* Step 1: Walk wiring — Length=6, Delay=2, facet + tx/ty. */
	if (GetActMapVal("Length", "Walk", SCRP) != 6)
		FatalError("ScorpionArtSmoke FAIL step 1: Walk Length != 6");
	if (GetActMapVal("Directions", "Walk", SCRP) != 2)
		FatalError("ScorpionArtSmoke FAIL step 1: Walk Directions != 2");
	if (GetActMapVal("FlipDir", "Walk", SCRP) != 1)
		FatalError("ScorpionArtSmoke FAIL step 1: Walk FlipDir != 1");
	if (GetActMapVal("Delay", "Walk", SCRP) != 2)
		FatalError("ScorpionArtSmoke FAIL step 1: Walk Delay != 2");
	if (GetActMapVal("Facet", "Walk", SCRP, 0) != 0
	 || GetActMapVal("Facet", "Walk", SCRP, 1) != 0
	 || GetActMapVal("Facet", "Walk", SCRP, 2) != 24
	 || GetActMapVal("Facet", "Walk", SCRP, 3) != 14
	 || GetActMapVal("Facet", "Walk", SCRP, 4) != -2
	 || GetActMapVal("Facet", "Walk", SCRP, 5) != -2)
		FatalError("ScorpionArtSmoke FAIL step 1: Walk Facet mismatch");

	/* Step 1b: per-action row bands + Length. */
	if (GetActMapVal("Facet", "Turn", SCRP, 0) != 0
	 || GetActMapVal("Facet", "Turn", SCRP, 1) != 14
	 || GetActMapVal("Facet", "Turn", SCRP, 2) != 24
	 || GetActMapVal("Facet", "Turn", SCRP, 3) != 14
	 || GetActMapVal("Facet", "Turn", SCRP, 4) != -2
	 || GetActMapVal("Facet", "Turn", SCRP, 5) != -2)
		FatalError("ScorpionArtSmoke FAIL step 1b: Turn Facet mismatch");
	if (GetActMapVal("Length", "Turn", SCRP) != 2)
		FatalError("ScorpionArtSmoke FAIL step 1b: Turn Length != 2");
	if (GetActMapVal("Facet", "Jump", SCRP, 0) != 0
	 || GetActMapVal("Facet", "Jump", SCRP, 1) != 28
	 || GetActMapVal("Facet", "Jump", SCRP, 2) != 24
	 || GetActMapVal("Facet", "Jump", SCRP, 3) != 14
	 || GetActMapVal("Facet", "Jump", SCRP, 4) != -2
	 || GetActMapVal("Facet", "Jump", SCRP, 5) != -2)
		FatalError("ScorpionArtSmoke FAIL step 1b: Jump Facet mismatch");
	if (GetActMapVal("Length", "Jump", SCRP) != 2)
		FatalError("ScorpionArtSmoke FAIL step 1b: Jump Length != 2");
	if (GetActMapVal("Facet", "Tumble", SCRP, 0) != 0
	 || GetActMapVal("Facet", "Tumble", SCRP, 1) != 42
	 || GetActMapVal("Facet", "Tumble", SCRP, 2) != 24
	 || GetActMapVal("Facet", "Tumble", SCRP, 3) != 14
	 || GetActMapVal("Facet", "Tumble", SCRP, 4) != -2
	 || GetActMapVal("Facet", "Tumble", SCRP, 5) != -2)
		FatalError("ScorpionArtSmoke FAIL step 1b: Tumble Facet mismatch");
	if (GetActMapVal("Length", "Tumble", SCRP) != 2)
		FatalError("ScorpionArtSmoke FAIL step 1b: Tumble Length != 2");
	if (GetActMapVal("Facet", "Swim", SCRP, 0) != 0
	 || GetActMapVal("Facet", "Swim", SCRP, 1) != 56
	 || GetActMapVal("Facet", "Swim", SCRP, 2) != 24
	 || GetActMapVal("Facet", "Swim", SCRP, 3) != 14
	 || GetActMapVal("Facet", "Swim", SCRP, 4) != -2
	 || GetActMapVal("Facet", "Swim", SCRP, 5) != -2)
		FatalError("ScorpionArtSmoke FAIL step 1b: Swim Facet mismatch");
	if (GetActMapVal("Length", "Swim", SCRP) != 2)
		FatalError("ScorpionArtSmoke FAIL step 1b: Swim Length != 2");

	/* Step 2: phase round-trip 0-5, both dirs. */
	scorp->SetAction("Walk");
	var d = 0;
	while (d < 2)
	{
		scorp->SetDir(d);
		scorp->SetAction("Walk");   /* SetDir fired TurnAction=Turn; re-enter Walk */
		var p = 0;
		while (p < 6)
		{
			scorp->SetPhase(p);
			if (scorp->GetPhase() != p)
				FatalError("ScorpionArtSmoke FAIL step 2: phase round-trip");
			p++;
		}
		d++;
	}

	Log("ScorpionArtSmoke PASS");
	GameOver();
	return true;
}
