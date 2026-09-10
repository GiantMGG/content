/*-- AgricultureArtSmoke.c4s — cycle-110 art wiring pin (spec           --*/
/* agriculture-art-batch). Synchronous RunSmokeSteps() pattern           --*/
/* (AquacultureSmoke/DatePalmArtSmoke norm).                             --*/

#strict 2

protected func Initialize()
{
	RunSmokeSteps();
	return true;
}

func AssertLobsterAction(string szAction, int iFx, int iFy, int iLen, int iDir, int iFlip, int iDelay)
{
	if (GetActMapVal("Facet", szAction, LBST, 0) != iFx
	 || GetActMapVal("Facet", szAction, LBST, 1) != iFy
	 || GetActMapVal("Facet", szAction, LBST, 2) != 16
	 || GetActMapVal("Facet", szAction, LBST, 3) != 12)
		FatalError(Format("AgricultureArtSmoke FAIL: %s Facet mismatch", szAction));
	if (GetActMapVal("Length", szAction, LBST) != iLen)
		FatalError(Format("AgricultureArtSmoke FAIL: %s Length != %d", szAction, iLen));
	if (GetActMapVal("Directions", szAction, LBST) != iDir)
		FatalError(Format("AgricultureArtSmoke FAIL: %s Directions != %d", szAction, iDir));
	if (GetActMapVal("FlipDir", szAction, LBST) != iFlip)
		FatalError(Format("AgricultureArtSmoke FAIL: %s FlipDir != %d", szAction, iFlip));
	if (GetActMapVal("Delay", szAction, LBST) != iDelay)
		FatalError(Format("AgricultureArtSmoke FAIL: %s Delay != %d", szAction, iDelay));
}

func AssertPicture(id idDef, int iPx, int iPy, int iPw, int iPh, string szDef)
{
	if (GetDefCoreVal("Picture", "DefCore", idDef, 0) != iPx
	 || GetDefCoreVal("Picture", "DefCore", idDef, 1) != iPy
	 || GetDefCoreVal("Picture", "DefCore", idDef, 2) != iPw
	 || GetDefCoreVal("Picture", "DefCore", idDef, 3) != iPh)
		FatalError(Format("AgricultureArtSmoke FAIL: %s Picture mismatch", szDef));
}

func RunSmokeSteps()
{
	/* Step 0: lobster spawns + physical block tripwire + Picture. */
	var pLobster = CreateObject(LBST, 50, 30, NO_OWNER);
	if (!pLobster)
		FatalError("AgricultureArtSmoke FAIL step 0: LBST not spawned");
	if (GetDefCoreVal("Width", "DefCore", LBST) != 16
	 || GetDefCoreVal("Height", "DefCore", LBST) != 12
	 || GetDefCoreVal("Offset", "DefCore", LBST, 0) != -8
	 || GetDefCoreVal("Offset", "DefCore", LBST, 1) != -6)
		FatalError("AgricultureArtSmoke FAIL step 0: LBST physical block changed");
	AssertPicture(LBST, 0, 0, 16, 12, "LBST");

	/* Step 0b: LBST ActMap post-rewire numbers (Length=2 x4 actions). */
	AssertLobsterAction("Walk", 0, 0, 2, 2, 1, 1);
	AssertLobsterAction("Swim", 0, 12, 2, 2, 1, 1);
	AssertLobsterAction("Jump", 0, 24, 2, 2, 1, 1);
	AssertLobsterAction("Turn", 0, 36, 2, 2, 1, 3);

	/* Step 0c: LBST Dead facet — the dedicated belly-up frame. */
	if (GetActMapVal("Facet", "Dead", LBST, 0) != 32
	 || GetActMapVal("Facet", "Dead", LBST, 1) != 0
	 || GetActMapVal("Facet", "Dead", LBST, 2) != 16
	 || GetActMapVal("Facet", "Dead", LBST, 3) != 12)
		FatalError("AgricultureArtSmoke FAIL step 0c: Dead Facet mismatch");
	if (GetActMapVal("Length", "Dead", LBST) != 1)
		FatalError("AgricultureArtSmoke FAIL step 0c: Dead Length != 1");

	/* Step 0d: LBST action round-trip in both dirs. */
	pLobster->SetDir(DIR_Left);
	pLobster->SetAction("Walk");
	if (pLobster->GetAction() != "Walk")
		FatalError("AgricultureArtSmoke FAIL step 0d: Walk round-trip");
	pLobster->SetDir(DIR_Right);
	pLobster->SetAction("Swim");
	if (pLobster->GetAction() != "Swim")
		FatalError("AgricultureArtSmoke FAIL step 0d: Swim round-trip");
	pLobster->SetAction("Jump");
	if (pLobster->GetAction() != "Jump")
		FatalError("AgricultureArtSmoke FAIL step 0d: Jump round-trip");
	pLobster->SetAction("Turn");
	if (pLobster->GetAction() != "Turn")
		FatalError("AgricultureArtSmoke FAIL step 0d: Turn round-trip");
	pLobster->SetAction("Dead");
	if (pLobster->GetAction() != "Dead")
		FatalError("AgricultureArtSmoke FAIL step 0d: Dead round-trip");

	/* Steps 1-6: the wave-A props + items, one Picture assert each. */
	var pCooked = CreateObject(CLBS, 60, 30, NO_OWNER);
	if (!pCooked)
		FatalError("AgricultureArtSmoke FAIL step 1: CLBS not spawned");
	AssertPicture(CLBS, 0, 0, 16, 12, "CLBS");

	var pDead = CreateObject(DLBS, 70, 30, NO_OWNER);
	if (!pDead)
		FatalError("AgricultureArtSmoke FAIL step 2: DLBS not spawned");
	AssertPicture(DLBS, 0, 0, 16, 12, "DLBS");

	var pPearl = CreateObject(APRL, 80, 30, NO_OWNER);
	if (!pPearl)
		FatalError("AgricultureArtSmoke FAIL step 3: APRL not spawned");
	AssertPicture(APRL, 0, 0, 6, 6, "APRL");

	var pOyster = CreateObject(OYST, 90, 30, NO_OWNER);
	if (!pOyster)
		FatalError("AgricultureArtSmoke FAIL step 4: OYST not spawned");
	AssertPicture(OYST, 0, 0, 16, 16, "OYST");
	if (GetActMapVal("Facet", "Closed", OYST, 0) != 0
	 || GetActMapVal("Facet", "Closed", OYST, 1) != 0
	 || GetActMapVal("Facet", "Closed", OYST, 2) != 16
	 || GetActMapVal("Facet", "Closed", OYST, 3) != 16)
		FatalError("AgricultureArtSmoke FAIL step 4: Closed Facet mismatch");
	if (GetActMapVal("Facet", "Open", OYST, 0) != 16
	 || GetActMapVal("Facet", "Open", OYST, 1) != 0
	 || GetActMapVal("Facet", "Open", OYST, 2) != 16
	 || GetActMapVal("Facet", "Open", OYST, 3) != 16)
		FatalError("AgricultureArtSmoke FAIL step 4: Open Facet mismatch");

	var pPool = CreateObject(TDPL, 150, 30, NO_OWNER);
	if (!pPool)
		FatalError("AgricultureArtSmoke FAIL step 5: TDPL not spawned");
	AssertPicture(TDPL, 0, 0, 40, 24, "TDPL");
	if (GetActMapVal("Facet", "Exposed", TDPL, 0) != 0
	 || GetActMapVal("Facet", "Exposed", TDPL, 1) != 0
	 || GetActMapVal("Facet", "Exposed", TDPL, 2) != 40
	 || GetActMapVal("Facet", "Exposed", TDPL, 3) != 24)
		FatalError("AgricultureArtSmoke FAIL step 5: Exposed Facet mismatch");
	if (GetActMapVal("Facet", "Flooded", TDPL, 0) != 40
	 || GetActMapVal("Facet", "Flooded", TDPL, 1) != 0
	 || GetActMapVal("Facet", "Flooded", TDPL, 2) != 40
	 || GetActMapVal("Facet", "Flooded", TDPL, 3) != 24)
		FatalError("AgricultureArtSmoke FAIL step 5: Flooded Facet mismatch");

	var pTrap = CreateObject(LBTP, 200, 30, NO_OWNER);
	if (!pTrap)
		FatalError("AgricultureArtSmoke FAIL step 6: LBTP not spawned");
	AssertPicture(LBTP, 0, 0, 20, 24, "LBTP");

	/* Step 7: wheat — golden-Ready Picture + stage round-trip. */
	var pWheat = CreateObject(AGWH, 250, 30, NO_OWNER);
	if (!pWheat)
		FatalError("AgricultureArtSmoke FAIL step 7: AGWH not spawned");
	AssertPicture(AGWH, 24, 0, 12, 20, "AGWH");
	pWheat->SetAction("Seedling");
	if (pWheat->GetAction() != "Seedling")
		FatalError("AgricultureArtSmoke FAIL step 7: Seedling round-trip");
	pWheat->SetAction("Growing");
	if (pWheat->GetAction() != "Growing")
		FatalError("AgricultureArtSmoke FAIL step 7: Growing round-trip");
	pWheat->SetAction("Ready");
	if (pWheat->GetAction() != "Ready")
		FatalError("AgricultureArtSmoke FAIL step 7: Ready round-trip");

	/* Step 8: apple tree — one mature sprite, PICTURE_OOB fixed. */
	var pTree = CreateObject(AGAT, 300, 30, NO_OWNER);
	if (!pTree)
		FatalError("AgricultureArtSmoke FAIL step 8: AGAT not spawned");
	AssertPicture(AGAT, 0, 0, 24, 40, "AGAT");
	if (GetActMapVal("Facet", "Idle", AGAT, 0) != 0
	 || GetActMapVal("Facet", "Idle", AGAT, 1) != 0
	 || GetActMapVal("Facet", "Idle", AGAT, 2) != 24
	 || GetActMapVal("Facet", "Idle", AGAT, 3) != 40)
		FatalError("AgricultureArtSmoke FAIL step 8: Idle Facet mismatch");

	Log("AgricultureArtSmoke PASS");
	GameOver();
	return true;
}
