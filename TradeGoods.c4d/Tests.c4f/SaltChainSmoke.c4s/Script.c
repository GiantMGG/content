/*-- SaltChainSmoke.c4s -- headless salt-chain integration test. --*/
/* RED-first gate (spec trade-campaign-merge cycle 193): asserts the */
/* SaltPan produce path. A pan with seawater contact yields SALT     */
/* within one DefCore Timer cycle (Timer=210), a dry pan yields none,*/
/* and SALT prices via the TradeLib market model.                    */

#strict 2

static g_iStep;
static pWetPan, pDryPan;

/* First solid pixel at column iX, scanning down from y=20. */
global func SurfaceY(int iX) {
	var iY = 20;
	while (iY < 380 && !GBackSolid(iX, iY)) iY++;
	return iY;
}

/* Dig a water pocket and fill it bottom-up (FirstLightClimate recipe). */
global func CarveSeawater(int iX, int iY, int iW, int iH) {
	DigFreeRect(iX, iY, iW, iH);
	var iWater = Material("Water");
	var px, py;
	for (py = iY + iH - 2; py > iY; py--)
		for (px = iX + 1; px < iX + iW - 1; px++)
			InsertMaterial(iWater, px, py);
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
		// Step 0b: two SaltPans -- the wet one gets a seawater pocket
		// carved flush against its right flank; the other stays dry.
		var iYWet = SurfaceY(30), iYDry = SurfaceY(100);
		pWetPan = CreateObject(SLTP, 30, iYWet - 6, NO_OWNER);
		pDryPan = CreateObject(SLTP, 100, iYDry - 6, NO_OWNER);
		if (!pWetPan || !pDryPan)
			FatalError("SaltChainSmoke FAIL step 0: SLTP not spawned");
		CarveSeawater(46, iYWet - 12, 18, 22);
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
