/*-- DammbruchSmoke.c4s -- cycle-124 acceptance twin (liquid-body-dynamics). --*/
/*                                                                            */
/* Apparatus: static-water reservoir (rock tank on the flat map's ground      */
/* top y=200), a Sandstone keystone band in the dam base (x[272,280]          */
/* y[190,196]), and the open floodplain right of the dam, bounded by a        */
/* rock rim at x[940,948]. The breach is cut programmatically (no player      */
/* input): ExtractMaterialAmount on the keystone cells. The flood is 100%     */
/* the cycle-124 physics: Water FlowRate=4 multi-hop transfer                 */
/* (C4MassMover.cpp hop loop) + JetFall=3 ballistic handoff at the dam-face   */
/* lip (budgeted create-or-deposit caster, C4PXS cap fallback).               */
/*                                                                            */
/* Assert classes (each a distinct FatalError):                               */
/*   drain   -- reservoir surface at the centre column sinks >= DRAIN_PER_WIN */
/*              rows per flood window while the tank still drains (mutation   */
/*              M1: Water FlowRate=1 must go RED here)                        */
/*   mm      -- GetMassMoverCount() >= MM_FLOOR while flooding (transfer-path */
/*              non-vacuity; the cycle-118 instrument gap closed)             */
/*   pxs     -- GetPXSCount() >= JET_FLOOR during the breach-jet phase        */
/*              (mutation M2: Water JetFall=0 must go RED here)               */
/*   conserv -- |(static Water + PXS) - baseline| <= CONSERVE_BAND: the       */
/*              budgeted caster's standing tripwire (mutation M3)             */
/*   fill    -- static Water in the floodplain span >= VALLEY_FLOOR           */
/*   flat    -- settled lake per-column top-row spread <= FLAT_MAX over the   */
/*              lake interior span (the flat-at-rest acceptance)              */
/*   quiet   -- GetMassMoverCount() <= SETTLE_MM after settling (quiescence)  */
/*                                                                            */
/* [CAL] lines are tick-keyed sim state ONLY (no GetTime) so the x3           */
/* determinism ritual can diff transcripts byte-exact; the seed is pinned     */
/* via SMOKE_SEED_PIN_DammbruchSmoke (tests/CMakeLists.txt).                   */

#strict 2

// ---- Gate constants (frozen 2026-09-14; calibration history in scratch) ---
static const DRAIN_PER_WIN = 4;    // observed lvl drops/win: 6,6,4,5,5 @ FlowRate=4;
                                   // FlowRate=1 -> ~1.5 -> M1 trips RED
static const DRAINED_ROW   = 158;  // assert off at/above settle equilibrium lvl 161
static const MM_FLOOR      = 25;   // observed flood-phase min: 34 (win 3), peak 56
static const JET_FLOOR     = 3;    // bursty jet profile: crests 211 (win 2) & 64
                                   // (win 5), trough 4 (win 3); M2 -> 0 -> RED
static const CONSERVE_BAND = 20;   // observed max |total-baseline| = 7 (win 2);
                                   // settle deficit -2 = legacy InsertMaterial-fail
                                   // path (not the caster); ~3x margin
static const VALLEY_FLOOR  = 3000; // static water in floodplain after flood
static const FLAT_MAX      = 3;    // settled lake surface variance (px)
static const SETTLE_MM     = 50;   // mover ceiling after settling

// ---- Apparatus coordinates (landscape 1000x400, ground top y=200) ---------
static const RESC_X  = 180;  // reservoir centre column (level probe)
static const LAKE_X1 = 380;  // flat-at-rest scan span (lake interior)
static const LAKE_X2 = 850;
static const LAKE_Y1 = 110;  // scan rows: sky down to just above ground
static const LAKE_Y2 = 196;

static g_iStep;
static g_iWater0;
static g_iLvlMark;

protected func Initialize()
{
	// Reservoir (left): rock tank on the flat ground, static water fill.
	DrawMaterialQuad("Rock", 80, 110, 88, 110, 88, 200, 80, 200);
	DrawMaterialQuad("Water", 89, 130, 271, 130, 271, 199, 89, 199);

	// Dam (reservoir right wall): Rock above and below the keystone band.
	DrawMaterialQuad("Rock", 272, 110, 280, 110, 280, 190, 272, 190);
	DrawMaterialQuad("Sandstone", 272, 190, 280, 190, 280, 196, 272, 196);
	DrawMaterialQuad("Rock", 272, 196, 280, 196, 280, 200, 272, 200);

	// Floodplain right rim: bounds the settling lake.
	DrawMaterialQuad("Rock", 940, 150, 948, 150, 948, 200, 940, 200);

	// Paint asserts (CascadeSmoke discipline): every ROI filled.
	var n = CountMatRegion(Material("Water"), 89, 130, 271, 199);
	if (n < 10000)
		FatalError(Format("DammbruchSmoke FAIL: reservoir short (%d)", n));
	n = CountMatRegion(Material("Sandstone"), 272, 190, 280, 196);
	if (n < 40)
		FatalError(Format("DammbruchSmoke FAIL: keystone short (%d)", n));

	// Conservation baseline (closed apparatus: no Lava, climate 50 keeps
	// BelowTempConvert off; splash embed conserves; jets conserve by caster).
	g_iWater0 = GetMaterialCount(Material("Water")) + GetPXSCount();

	g_iStep = 0;
	g_iLvlMark = ReservoirLevel();
	AddEffect("RunTest", 0, 1, 35, 0, 0);
	return true;
}

global func FxRunTestStart(target, effect, temp) { return 1; }

global func FxRunTestTimer(object target, int effect, int timer)
{
	++g_iStep;

	// ---- Step 1: equilibrium check + programmed breach ------------------
	if (g_iStep == 1)
	{
		if (GetMassMoverCount() > 20)
			FatalError(Format("DammbruchSmoke FAIL win 1: painted water already moving (mm %d)", GetMassMoverCount()));
		// Breach: extract the keystone band cells (deterministic, no player
		// input). The extraction fires CheckInstabilityRange -> movers.
		// (Amendment 1, orchestrator-approved: FnExtractMaterialAmount
		// re-tests the fixed (x,y); a single amount=48 call yields 1.
		// Per-cell loop, by-major: sweep rows first so each call's
		// FindMatTop climb finds its column's top already cleared and
		// clears exactly its own cell -- cut=54 (all 6 painted rows;
		// row y=196 is Rock, overwritten by the rock-below quad).)
		var cut = 0, bx, by;
		for (by = 190; by <= 196; ++by)
			for (bx = 272; bx <= 280; ++bx)
				cut += ExtractMaterialAmount(bx, by, Material("Sandstone"), 1);
		if (cut < 40)
			FatalError(Format("DammbruchSmoke FAIL win 1: keystone extraction short (%d)", cut));
	}

	// ---- Per-window standing guards (steps >= 2) ------------------------
	if (g_iStep >= 2)
	{
		// Conservation: the budgeted caster's tripwire (band 0).
		var total = GetMaterialCount(Material("Water")) + GetPXSCount();
		if (Abs(total - g_iWater0) > CONSERVE_BAND)
			FatalError(Format("DammbruchSmoke FAIL win %d: water %d != %d (caster/deletion)", g_iStep, total, g_iWater0));

		// Flood-phase floors (steps 2..6; calibrated: flow completes by
		// win 6 -- mm drops to 0 at win 7, so the floor window ends at 6).
		if (g_iStep <= 6)
		{
			var lvl = ReservoirLevel();
			if (lvl >= 0 && lvl < DRAINED_ROW && lvl - g_iLvlMark < DRAIN_PER_WIN)
				FatalError(Format("DammbruchSmoke FAIL win %d: drain %d < %d (FlowRate regression)", g_iStep, lvl - g_iLvlMark, DRAIN_PER_WIN));
			if (GetMassMoverCount() < MM_FLOOR)
				FatalError(Format("DammbruchSmoke FAIL win %d: mm %d < MM_FLOOR (transfer path vacuous)", g_iStep, GetMassMoverCount()));
			if (g_iStep <= 5 && GetPXSCount() < JET_FLOOR)
				FatalError(Format("DammbruchSmoke FAIL win %d: pxs %d < JET_FLOOR (jet path vacuous)", g_iStep, GetPXSCount()));
		}
	}

	// ---- [CAL] telemetry (tick-keyed sim state only) ---------------------
	Log(Format("[CAL] breach win %d: lvl %d, mm %d, pxs %d, water %d",
		g_iStep, ReservoirLevel(), GetMassMoverCount(), GetPXSCount(),
		GetMaterialCount(Material("Water"))));
	g_iLvlMark = ReservoirLevel();

	// ---- Final windows ----------------------------------------------------
	if (g_iStep == 8)
	{
		var fill = CountMatRegion(Material("Water"), 281, 130, 939, 199);
		if (fill < VALLEY_FLOOR)
			FatalError(Format("DammbruchSmoke FAIL win 8: valley fill %d < %d", fill, VALLEY_FLOOR));
	}
	if (g_iStep == 9)
	{
		var flat = LakeVariance(LAKE_X1, LAKE_X2, LAKE_Y1, LAKE_Y2);
		if (flat < 0 || flat > FLAT_MAX)
			FatalError(Format("DammbruchSmoke FAIL win 9: lake surface spread %d > %d", flat, FLAT_MAX));
		if (GetMassMoverCount() > SETTLE_MM)
			FatalError(Format("DammbruchSmoke FAIL win 9: not quiescent (mm %d > %d)", GetMassMoverCount(), SETTLE_MM));
		Log("DammbruchSmoke PASS");
		GameOver();
		return -1;
	}
	return 1;
}

/* Topmost static-Water row at the reservoir centre column; -1 if empty. */
global func ReservoirLevel()
{
	var y;
	for (y = 60; y < 200; y++)
		if (GetMaterial(RESC_X, y) == Material("Water"))
			return y;
	return -1;
}

/* Per-column topmost-water-row spread over [x1,x2]; -1 if no water found. */
global func LakeVariance(x1, x2, y1, y2)
{
	var min = 32767, max = -32767, cols = 0, x, y, top;
	for (x = x1; x <= x2; x++)
	{
		top = -1;
		for (y = y1; y <= y2; y++)
			if (GetMaterial(x, y) == Material("Water")) { top = y; break; }
		if (top >= 0)
		{
			++cols;
			if (top < min) min = top;
			if (top > max) max = top;
		}
	}
	if (cols == 0) return -1;
	return max - min;
}

/* Static material cells in the rectangle [x1,x2] x [y1,y2]. */
global func CountMatRegion(mat, x1, y1, x2, y2)
{
	var count = 0, x, y;
	for (x = x1; x <= x2; x++)
		for (y = y1; y <= y2; y++)
			if (GetMaterial(x, y) == mat)
				++count;
	return count;
}
