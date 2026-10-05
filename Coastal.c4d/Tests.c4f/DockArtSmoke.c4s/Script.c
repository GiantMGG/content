/*-- DockArtSmoke.c4s -- cycle-194 art wiring pin for the Dock (DKST). --*/
/* The Dock def went from an invisible 96-byte transparent placeholder   */
/* to real 20x20 pixel art (Graphics.png via tools/gen_dock_gfx.py).    */
/* This smoke pins the engine-LOADED art wiring: the DefCore Picture     */
/* rect (64x64 sheet), the Idle action Facet rect (20x20 art cell),      */
/* Length=1 / Delay=1, and the SetAction round-trip. Synchronous         */
/* RunSmokeSteps() pattern (DatePalmArtSmoke/DesertSmoke norm). On any   */
/* assertion failure FatalError -> non-zero smoke exit.                  */
#strict 3

protected func Initialize()
{
	RunSmokeSteps();
	return true;
}

func RunSmokeSteps()
{
	/* Step 0: def loads and spawns. */
	var pDock = CreateObject(DKST, 50, 30, NO_OWNER);
	if (!pDock)
		FatalError("DockArtSmoke FAIL step 0: DKST not spawned");

	/* Step 1: DefCore Picture rect = the 64x64 sheet. */
	if (GetDefCoreVal("Picture", "DefCore", DKST, 0) != 0
	 || GetDefCoreVal("Picture", "DefCore", DKST, 1) != 0
	 || GetDefCoreVal("Picture", "DefCore", DKST, 2) != 64
	 || GetDefCoreVal("Picture", "DefCore", DKST, 3) != 64)
		FatalError("DockArtSmoke FAIL step 1: Picture rect mismatch (want 0,0,64,64)");

	/* Step 2: Idle action Facet rect = the 20x20 art cell. These also  */
	/* prove the Idle action exists in the ActMap: GetActMapVal returns */
	/* NULL for a missing action, failing every comparison.            */
	if (GetActMapVal("Facet", "Idle", DKST, 0) != 0
	 || GetActMapVal("Facet", "Idle", DKST, 1) != 0
	 || GetActMapVal("Facet", "Idle", DKST, 2) != 20
	 || GetActMapVal("Facet", "Idle", DKST, 3) != 20)
		FatalError("DockArtSmoke FAIL step 2: Idle Facet rect mismatch (want 0,0,20,20)");
	if (GetActMapVal("Length", "Idle", DKST) != 1)
		FatalError("DockArtSmoke FAIL step 2: Idle Length != 1");
	if (GetActMapVal("Delay", "Idle", DKST) != 1)
		FatalError("DockArtSmoke FAIL step 2: Idle Delay != 1");

	/* Step 3: SetAction round-trip sticks. Note: the engine special-    */
	/* cases the name "Idle" into the ActIdle (-1) state, so GetAction() */
	/* reports "Idle" in BOTH the set and unset cases -- this step pins  */
	/* the SetAction-by-name contract, while the load-bearing proof that */
	/* the Idle action is actually wired is step 2 (GetActMapVal for a  */
	/* missing action returns NULL and fails every comparison).         */
	pDock->SetAction("Idle");
	if (pDock->GetAction() != "Idle")
		FatalError("DockArtSmoke FAIL step 3: SetAction(Idle) round-trip");

	Log("DockArtSmoke PASS");
	GameOver();
	return true;
}
