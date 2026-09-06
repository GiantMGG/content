/*-- CamelArtSmoke.c4s — cycle-99 art wiring pin (spec               --*/
/* clonk-gfx-skill). Synchronous RunSmokeSteps() pattern              --*/
/* (DesertSmoke/ScorpionArtSmoke norm).                               --*/
#strict 3

protected func Initialize()
{
	RunSmokeSteps();
	return true;
}

func RunSmokeSteps()
{
	/* Step 0: def loads + Picture rect is the phase-0 sprite. */
	var camel = CreateObject(CAML, 50, 30, NO_OWNER);
	if (!camel)
		FatalError("CamelArtSmoke FAIL step 0: CAML not spawned");
	if (GetDefCoreVal("Picture", "DefCore", CAML, 0) != 0
	 || GetDefCoreVal("Picture", "DefCore", CAML, 1) != 0
	 || GetDefCoreVal("Picture", "DefCore", CAML, 2) != 36
	 || GetDefCoreVal("Picture", "DefCore", CAML, 3) != 26)
		FatalError("CamelArtSmoke FAIL step 0: Picture mismatch");

	/* Step 1: Walk wiring — Length=2, facet/dirs/delay unchanged. */
	if (GetActMapVal("Length", "Walk", CAML) != 2)
		FatalError("CamelArtSmoke FAIL step 1: Walk Length != 2");
	if (GetActMapVal("Directions", "Walk", CAML) != 2)
		FatalError("CamelArtSmoke FAIL step 1: Walk Directions != 2");
	if (GetActMapVal("FlipDir", "Walk", CAML) != 1)
		FatalError("CamelArtSmoke FAIL step 1: Walk FlipDir != 1");
	if (GetActMapVal("Delay", "Walk", CAML) != 3)
		FatalError("CamelArtSmoke FAIL step 1: Walk Delay != 3");
	if (GetActMapVal("Facet", "Walk", CAML, 0) != 0
	 || GetActMapVal("Facet", "Walk", CAML, 1) != 0
	 || GetActMapVal("Facet", "Walk", CAML, 2) != 36
	 || GetActMapVal("Facet", "Walk", CAML, 3) != 26)
		FatalError("CamelArtSmoke FAIL step 1: Walk Facet mismatch");

	/* Step 1b: multi-action wiring — Jump/Tumble/Swim bands + Turn. */
	if (GetActMapVal("Facet", "Jump", CAML, 0) != 0
	 || GetActMapVal("Facet", "Jump", CAML, 1) != 26
	 || GetActMapVal("Facet", "Jump", CAML, 2) != 36
	 || GetActMapVal("Facet", "Jump", CAML, 3) != 26)
		FatalError("CamelArtSmoke FAIL step 1b: Jump Facet mismatch");
	if (GetActMapVal("Length", "Jump", CAML) != 1)
		FatalError("CamelArtSmoke FAIL step 1b: Jump Length != 1");
	if (GetActMapVal("Facet", "Tumble", CAML, 0) != 0
	 || GetActMapVal("Facet", "Tumble", CAML, 1) != 52
	 || GetActMapVal("Facet", "Tumble", CAML, 2) != 36
	 || GetActMapVal("Facet", "Tumble", CAML, 3) != 26)
		FatalError("CamelArtSmoke FAIL step 1b: Tumble Facet mismatch");
	if (GetActMapVal("Length", "Tumble", CAML) != 1)
		FatalError("CamelArtSmoke FAIL step 1b: Tumble Length != 1");
	if (GetActMapVal("Facet", "Swim", CAML, 0) != 0
	 || GetActMapVal("Facet", "Swim", CAML, 1) != 78
	 || GetActMapVal("Facet", "Swim", CAML, 2) != 36
	 || GetActMapVal("Facet", "Swim", CAML, 3) != 26)
		FatalError("CamelArtSmoke FAIL step 1b: Swim Facet mismatch");
	if (GetActMapVal("Length", "Swim", CAML) != 1)
		FatalError("CamelArtSmoke FAIL step 1b: Swim Length != 1");
	if (GetActMapVal("Facet", "Turn", CAML, 0) != 0
	 || GetActMapVal("Facet", "Turn", CAML, 1) != 0
	 || GetActMapVal("Facet", "Turn", CAML, 2) != 36
	 || GetActMapVal("Facet", "Turn", CAML, 3) != 26)
		FatalError("CamelArtSmoke FAIL step 1b: Turn Facet mismatch");
	if (GetActMapVal("Length", "Turn", CAML) != 1)
		FatalError("CamelArtSmoke FAIL step 1b: Turn Length != 1");

	/* Step 2: phase round-trip, both dirs. */
	camel->SetAction("Walk");
	var d = 0;
	while (d < 2)
	{
		camel->SetDir(d);
		camel->SetPhase(0);
		if (camel->GetPhase() != 0)
			FatalError("CamelArtSmoke FAIL step 2: phase 0 round-trip");
		camel->SetPhase(1);
		if (camel->GetPhase() != 1)
			FatalError("CamelArtSmoke FAIL step 2: phase 1 round-trip");
		d++;
	}

	Log("CamelArtSmoke PASS");
	GameOver();
	return true;
}
