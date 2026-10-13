/*-- LighthouseArtSmoke.c4s -- cycle-196 art wiring pin for the LHGT   --*/
/* beacon tower. The Lighthouse def went from an invisible 96-byte      */
/* transparent placeholder to real 20x40 pixel art (Graphics.png via    */
/* tools/gen_lighthouse_gfx.py). This smoke pins the engine-LOADED art  */
/* wiring: the DefCore Picture rect (64x64 sheet), the Idle action      */
/* Facet rect (20x40 art cell), Length=1 / Delay=1, and the SetAction   */
/* round-trip. Synchronous RunSmokeSteps() pattern (DockArtSmoke/       */
/* DatePalmArtSmoke norm). On any assertion failure FatalError ->       */
/* non-zero smoke exit.                                                 */
#strict 3

protected func Initialize()
{
	RunSmokeSteps();
	return true;
}

func RunSmokeSteps()
{
	/* Step 0: def loads and spawns. */
	var pLhgt = CreateObject(LHGT, 50, 30, NO_OWNER);
	if (!pLhgt)
		FatalError("LighthouseArtSmoke FAIL step 0: LHGT not spawned");

	/* Step 1: DefCore Picture rect = the 64x64 sheet. */
	if (GetDefCoreVal("Picture", "DefCore", LHGT, 0) != 0
	 || GetDefCoreVal("Picture", "DefCore", LHGT, 1) != 0
	 || GetDefCoreVal("Picture", "DefCore", LHGT, 2) != 64
	 || GetDefCoreVal("Picture", "DefCore", LHGT, 3) != 64)
		FatalError("LighthouseArtSmoke FAIL step 1: Picture rect mismatch (want 0,0,64,64)");

	/* Step 2: Idle action Facet rect = the 20x40 art cell. These also  */
	/* prove the Idle action exists in the ActMap: GetActMapVal returns */
	/* NULL for a missing action, failing every comparison.            */
	if (GetActMapVal("Facet", "Idle", LHGT, 0) != 0
	 || GetActMapVal("Facet", "Idle", LHGT, 1) != 0
	 || GetActMapVal("Facet", "Idle", LHGT, 2) != 20
	 || GetActMapVal("Facet", "Idle", LHGT, 3) != 40)
		FatalError("LighthouseArtSmoke FAIL step 2: Idle Facet rect mismatch (want 0,0,20,40)");
	if (GetActMapVal("Length", "Idle", LHGT) != 1)
		FatalError("LighthouseArtSmoke FAIL step 2: Idle Length != 1");
	if (GetActMapVal("Delay", "Idle", LHGT) != 1)
		FatalError("LighthouseArtSmoke FAIL step 2: Idle Delay != 1");

	/* Step 3: SetAction round-trip sticks. Note: the engine special-    */
	/* cases the name "Idle" into the ActIdle (-1) state, so GetAction() */
	/* reports "Idle" in BOTH the set and unset cases -- this step pins  */
	/* the SetAction-by-name contract, while the load-bearing proof that */
	/* the Idle action is actually wired is step 2 (GetActMapVal for a  */
	/* missing action returns NULL and fails every comparison).         */
	pLhgt->SetAction("Idle");
	if (pLhgt->GetAction() != "Idle")
		FatalError("LighthouseArtSmoke FAIL step 3: SetAction(Idle) round-trip");

	/* Step 4: the Lit action (cycle 221) -- the gold-lamp twin facet. */
	/* A missing action makes GetActMapVal return NULL and fail every  */
	/* comparison (same discipline as step 2).                         */
	if (GetActMapVal("Facet", "Lit", LHGT, 0) != 20
	 || GetActMapVal("Facet", "Lit", LHGT, 1) != 0
	 || GetActMapVal("Facet", "Lit", LHGT, 2) != 20
	 || GetActMapVal("Facet", "Lit", LHGT, 3) != 40)
		FatalError("LighthouseArtSmoke FAIL step 4: Lit Facet rect mismatch (want 20,0,20,40)");
	if (GetActMapVal("Length", "Lit", LHGT) != 1)
		FatalError("LighthouseArtSmoke FAIL step 4: Lit Length != 1");
	if (GetActMapVal("Delay", "Lit", LHGT) != 1)
		FatalError("LighthouseArtSmoke FAIL step 4: Lit Delay != 1");

	/* Step 5: lit visual -- the sync flips the facet and keeps exactly  */
	/* one BFLM at the lamp-top anchor (the pFlame guard prevents a      */
	/* double spawn; Initialize's sync already created it).              */
	pLhgt->UpdateBeaconVisual();
	if (pLhgt->GetAction() != "Lit")
		FatalError("LighthouseArtSmoke FAIL step 5: action is not Lit after sync");
	var pFlm = FindObject(BFLM);
	if (!pFlm)
		FatalError("LighthouseArtSmoke FAIL step 5: no BFLM overlay while lit");
	if (Abs(GetX(pFlm) - GetX(pLhgt)) > 4
	 || Abs(GetY(pFlm) - (GetY(pLhgt) - 14)) > 4)
		FatalError("LighthouseArtSmoke FAIL step 5: BFLM off the lamp-top anchor");

	/* Step 6: dark transition -- direct sync call, no tick reliance.   */
	LocalN("bLit", pLhgt) = 0;
	pLhgt->UpdateBeaconVisual();
	if (pLhgt->GetAction() != "Idle")
		FatalError("LighthouseArtSmoke FAIL step 6: action is not Idle after dark flip");
	if (FindObject(BFLM))
		FatalError("LighthouseArtSmoke FAIL step 6: BFLM overlay survived the dark flip");

	/* Step 7: relight -- the sync respawns facet + overlay.            */
	LocalN("bLit", pLhgt) = 1;
	pLhgt->UpdateBeaconVisual();
	if (pLhgt->GetAction() != "Lit")
		FatalError("LighthouseArtSmoke FAIL step 7: action is not Lit after relight");
	if (!FindObject(BFLM))
		FatalError("LighthouseArtSmoke FAIL step 7: BFLM overlay not respawned on relight");

	Log("LighthouseArtSmoke PASS");
	GameOver();
	return true;
}
