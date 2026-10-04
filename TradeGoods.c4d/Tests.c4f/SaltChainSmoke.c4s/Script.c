/*-- SaltChainSmoke.c4s -- headless salt-chain integration test. --*/
/* RED-first gate (spec trade-campaign-merge cycle 193): asserts the */
/* SaltPan produce path. A pan with seawater contact yields SALT     */
/* within one DefCore Timer cycle (Timer=210), a dry pan yields none,*/
/* and SALT prices via the TradeLib market model.                    */
/* T6 note: the smoke's Map.bmp is a single-color template, so the   */
/* engine GENERATES the terrain from the RNG seed each run (verified */
/* cycle 193), and material inserts above the generated surface sit  */
/* in unmaterialized background and do not stick. The test therefore */
/* builds its rig below the surface (all y in the solid zone): a     */
/* snug pit for the wet pan, a walled seawater pool against its      */
/* right flank (pool walls are the untouched terrain), and a         */
/* dug-and-earth-refilled dry pad for the dry pan. There is no       */
/* terrain-sensitive coordinate left: the rig is deterministic on    */
/* any generated landscape.                                          */

#strict 2

static g_iStep;
static pWetPan, pDryPan;

/* First solid pixel at column iX, scanning down from y=20. */
global func SurfaceY(int iX) {
	var iY = 20;
	while (iY < 380 && !GBackSolid(iX, iY)) iY++;
	return iY;
}

/* Build the whole rig, self-anchored to the local surface: iWet is  */
/* the first solid row at the wet pan's column (observed ~199). Every */
/* piece sits below it in the solid zone, where inserted material    */
/* sticks; all offsets (15..39px down) tolerate +/- a few rows of     */
/* generated relief.                                                  */
global func BuildRig(int iWet) {
	var iEarth = Material("Earth");
	var iWater = Material("Water");
	var iX, iY;
	// Wet pan pit: dug socket under the pan's shape (x 16..44), floor
	// at iWet+26; the pan rests here and cannot drift.
	DigFreeRect(16, iWet + 15, 29, 12);
	// Seawater pool: interior x 47..90, from iWet+2 down 25 rows; the
	// west wall is the untouched terrain column 45/46 (1px clear of
	// the pan's right edge at x 44), the east wall the column 91/92.
	// The terrain tops at iWet, so the walls rise above the intended
	// water surface at iWet+2 -- the pool cannot spill.
	DigFreeRect(47, iWet + 2, 44, 25);
	for (iY = iWet + 2; iY < iWet + 27; iY++)
		for (iX = 47; iX < 91; iX++)
			InsertMaterial(iWater, iX, iY);
	// Dry pad: dig + earth-refill a block (x 104..156, from iWet+9)
	// covering every dry-pan sample, sealing out any generated
	// underground water.
	DigFreeRect(104, iWet + 9, 52, 31);
	for (iY = iWet + 9; iY < iWet + 40; iY++)
		for (iX = 104; iX < 156; iX++)
			InsertMaterial(iEarth, iX, iY);
}

protected func Initialize() {
	g_iStep = 0;
	// FirstLightClimate-proven driver form: effect on the scenario (target
	// 0) with GLOBAL callback funcs -- non-global funcs never fire under
	// --smoke-run (probe: .opencode/scratch/193/probe2.log, variant B).
	AddEffect("RunTest", 0, 1, 35, 0, 0);
	return true;
}

global func FxRunTestStart(target, effect, temp) { return 1; }

global func FxRunTestTimer(object target, int effect, int timer) {
	++g_iStep;
	if (g_iStep == 1) {
		// Step 0: SALT prices at a registered MarketStall.
		var pStall = CreateObject(MKTS, 50, 30, NO_OWNER);
		RegisterTradeGood(SALT, pStall, 8);
		if (GetMarketPriceAt(SALT, pStall) <= 0)
			FatalError("SaltChainSmoke FAIL step 0: SALT unpriced at stall");
		// Step 0b: build the rig anchored to the local surface, then
		// seat the wet pan beside the pool (flank samples at x 47 =
		// 17px water, ~13-20px below the water surface) and the dry
		// pan on the sealed pad 100px east.
		var iWet = SurfaceY(30);
		BuildRig(iWet);
		pWetPan = CreateObject(SLTP, 30, iWet + 20, NO_OWNER);
		pDryPan = CreateObject(SLTP, 130, iWet + 20, NO_OWNER);
		if (!pWetPan || !pDryPan)
			FatalError("SaltChainSmoke FAIL step 0: SLTP not spawned");
		return 1;
	}
	if (g_iStep >= 8) {
		// Frame ~280: the wet pan's Timer (created frame ~35, fires at
		// 210 -> ~frame 245) must have produced SALT; the dry pan none.
		var iWet = ContentsCount(SALT, pWetPan);
		var iDry = ContentsCount(SALT, pDryPan);
		if (iWet < 1)
			FatalError(Format("SaltChainSmoke FAIL step %d: wet pan produced no SALT (%d)", g_iStep, iWet));
		if (iDry != 0)
			FatalError(Format("SaltChainSmoke FAIL step %d: dry pan produced SALT (%d)", g_iStep, iDry));
		Log("SaltChainSmoke PASS");
		GameOver();
	}
	return 1;
}
