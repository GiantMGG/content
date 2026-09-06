/*-- DesertArtSmoke.c4s — cycle-100 art wiring pin (spec             --*/
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
	/* Step 0: Date spawns + Picture rect (8x8 fruit). */
	var date = CreateObject(DATE, 50, 30, NO_OWNER);
	if (!date)
		FatalError("DesertArtSmoke FAIL step 0: DATE not spawned");
	if (GetDefCoreVal("Picture", "DefCore", DATE, 0) != 0
	 || GetDefCoreVal("Picture", "DefCore", DATE, 1) != 0
	 || GetDefCoreVal("Picture", "DefCore", DATE, 2) != 8
	 || GetDefCoreVal("Picture", "DefCore", DATE, 3) != 8)
		FatalError("DesertArtSmoke FAIL step 0: DATE Picture mismatch");

	/* Step 1: Oasis spawns + Picture rect (24x8 pond). */
	var oasis = CreateObject(OASS, 60, 30, NO_OWNER);
	if (!oasis)
		FatalError("DesertArtSmoke FAIL step 1: OASS not spawned");
	if (GetDefCoreVal("Picture", "DefCore", OASS, 0) != 0
	 || GetDefCoreVal("Picture", "DefCore", OASS, 1) != 0
	 || GetDefCoreVal("Picture", "DefCore", OASS, 2) != 24
	 || GetDefCoreVal("Picture", "DefCore", OASS, 3) != 8)
		FatalError("DesertArtSmoke FAIL step 1: OASS Picture mismatch");

	/* Step 2: Quarry spawns + Picture rect (30x24 rock). */
	var quarry = CreateObject(QRRY, 80, 30, NO_OWNER);
	if (!quarry)
		FatalError("DesertArtSmoke FAIL step 2: QRRY not spawned");
	if (GetDefCoreVal("Picture", "DefCore", QRRY, 0) != 0
	 || GetDefCoreVal("Picture", "DefCore", QRRY, 1) != 0
	 || GetDefCoreVal("Picture", "DefCore", QRRY, 2) != 30
	 || GetDefCoreVal("Picture", "DefCore", QRRY, 3) != 24)
		FatalError("DesertArtSmoke FAIL step 2: QRRY Picture mismatch");

	/* Step 3: Quicksand spawns + Picture rect (20x6 patch). */
	var quicksand = CreateObject(QKSD, 90, 30, NO_OWNER);
	if (!quicksand)
		FatalError("DesertArtSmoke FAIL step 3: QKSD not spawned");
	if (GetDefCoreVal("Picture", "DefCore", QKSD, 0) != 0
	 || GetDefCoreVal("Picture", "DefCore", QKSD, 1) != 0
	 || GetDefCoreVal("Picture", "DefCore", QKSD, 2) != 20
	 || GetDefCoreVal("Picture", "DefCore", QKSD, 3) != 6)
		FatalError("DesertArtSmoke FAIL step 3: QKSD Picture mismatch");

	/* Step 4: SandDrift spawns + Picture rect (10x10 drift). */
	var drift = CreateObject(SDRF, 55, 30, NO_OWNER);
	if (!drift)
		FatalError("DesertArtSmoke FAIL step 4: SDRF not spawned");
	if (GetDefCoreVal("Picture", "DefCore", SDRF, 0) != 0
	 || GetDefCoreVal("Picture", "DefCore", SDRF, 1) != 0
	 || GetDefCoreVal("Picture", "DefCore", SDRF, 2) != 10
	 || GetDefCoreVal("Picture", "DefCore", SDRF, 3) != 10)
		FatalError("DesertArtSmoke FAIL step 4: SDRF Picture mismatch");

	/* Step 5: SandstoneBlock spawns + Picture rect (8x8 block). */
	var block = CreateObject(SNDS, 65, 30, NO_OWNER);
	if (!block)
		FatalError("DesertArtSmoke FAIL step 5: SNDS not spawned");
	if (GetDefCoreVal("Picture", "DefCore", SNDS, 0) != 0
	 || GetDefCoreVal("Picture", "DefCore", SNDS, 1) != 0
	 || GetDefCoreVal("Picture", "DefCore", SNDS, 2) != 8
	 || GetDefCoreVal("Picture", "DefCore", SNDS, 3) != 8)
		FatalError("DesertArtSmoke FAIL step 5: SNDS Picture mismatch");

	Log("DesertArtSmoke PASS");
	GameOver();
	return true;
}
