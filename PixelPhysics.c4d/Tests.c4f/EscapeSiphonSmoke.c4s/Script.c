/*-- EscapeSiphonSmoke.c4s -- headless siphon routing smoke (cycle 147 RB). --*/
/*                                                                          */
/* Pins the Room-01 "The Siphon" dig-breach physics end-to-end: digging     */
/* the cistern's sand floor plug (shovel primitive via DigFreeRect)         */
/* releases the static water, which drops down an enclosed ROCK chute and   */
/* pools in a rock-walled, rock-floored float basin beneath the breach.     */
/*                                                                          */
/* Why enclosed ROCK (probe .opencode/scratch/147/probe-c.txt): water PXS   */
/* landing on the default Material=Earth landscape are absorbed -- the      */
/* open-apron prototype lost the whole reservoir (map Water 4851 -> 1 in    */
/* 9 steps). Rock has no Corrode field, so a water path that touches only   */
/* Rock never corrodes; this apparatus caps the shaft with the cistern      */
/* floor (only opening = the dug plug), so no water can reach earth.        */
/*                                                                          */
/* Apparatus (flat 1000x400 map, ground top y=200):                         */
/*   Cistern: rock shell x[100,219] y[40,139] with a sand floor plug        */
/*            x[145,174] y[133,139]; static water fill x[106,213] y[46,132]. */
/*   Chute:   directly beneath the plug, a 120 px-wide shaft interior       */
/*            x[100,219] y[140,309] carved inside the rock block            */
/*            x[90,229] y[140,319]; the cistern floor caps it, so the only  */
/*            opening is the dug plug.                                      */
/*   Basin:   the lower half of that shaft x[100,219] y[240,309] (the goal  */
/*            ROI); rock floor y[310,319], rock walls x[90,99]/x[220,229].   */
/*                                                                          */
/* Driver: GLOBAL effect, step 1 = the room's solution action               */
/* (DigFreeRect + ExtractMaterialAmount kick, CascadeSmoke:76-77); steps    */
/* 2+ = standing guards; step 9 = basin assert with >=2x-margin threshold.  */
/* Seed pinned via SMOKE_SEED_PIN_EscapeSiphonSmoke.                        */

#strict 2

static g_iStep;
static g_fCalibrate;
static g_iCist0;

/* Basin fill floor, frozen after calibration (>=2x margin below the     */
/* worst observed green); evidence in .opencode/scratch/147/             */
/* rb-siphon-01.txt (cycle-147 RB): the sealed apparatus delivered        */
/* basin 8400 + shaft-mid 1010 of a 9396-cell cistern from step 3 on      */
/* (wall-loss 0), so 4000 keeps 2.1x margin below the 8400 settled floor. */
static const SIPHON_BASIN_MIN = 4000;

/* Basin ROI: lower half of the enclosed shaft (the float basin). */
static const BASIN_X1 = 100;
static const BASIN_Y1 = 240;
static const BASIN_X2 = 219;
static const BASIN_Y2 = 309;

/* Cistern water fill ROI (whole cistern interior). */
static const CIST_X1 = 106;
static const CIST_Y1 = 46;
static const CIST_X2 = 213;
static const CIST_Y2 = 132;

protected func Initialize()
{
	// --- Cistern: sealed rock shell with a diggable sand floor plug. ---
	DrawMaterialQuad("Rock", 100, 40, 106, 40, 106, 140, 100, 140);    // left wall (to y=139)
	DrawMaterialQuad("Rock", 214, 40, 220, 40, 220, 140, 214, 140);    // right wall (to y=139)
	DrawMaterialQuad("Rock", 100, 40, 220, 40, 220, 46, 100, 46);      // top lid
	DrawMaterialQuad("Rock", 106, 133, 145, 133, 145, 140, 106, 140);  // floor left of the plug
	DrawMaterialQuad("Sand", 145, 133, 175, 133, 175, 140, 145, 140);  // diggable floor plug
	DrawMaterialQuad("Rock", 175, 133, 214, 133, 214, 140, 175, 140);  // floor right of the plug
	DrawMaterialQuad("Water", 106, 46, 214, 46, 214, 133, 106, 133);   // static water fill

	// --- Enclosed chute + basin: solid rock block, carved to air. ------
	// The block caps the shaft with the cistern floor above, so the only
	// opening into the shaft is the dug plug hole (no earth contact).
	DrawMaterialQuad("Rock", 90, 140, 230, 140, 230, 320, 90, 320);    // block x[90,229] y[140,319]
	FreeRect(100, 140, 120, 170);                                      // shaft x[100,219] y[140,309]
	// (FreeRect, not DigFreeRect: Rock is not DigFree - DigFreeRect leaves
	//  it in place, which sealed the shaft and stranded the water.)
	// Remaining shell: walls x[90,99]/x[220,229], floor y[310,319].

	// --- Paint-baseline asserts (CascadeSmoke:39-53 style). -----------
	g_iCist0 = CountMatRegion(Material("Water"), CIST_X1, CIST_Y1, CIST_X2, CIST_Y2);
	if (g_iCist0 < 8500)
		FatalError(Format("EscapeSiphonSmoke FAIL: cistern fill short (%d)", g_iCist0));
	if (CountMatRegion(Material("Water"), BASIN_X1, BASIN_Y1, BASIN_X2, BASIN_Y2) != 0)
		FatalError("EscapeSiphonSmoke FAIL: basin not empty at start");

	g_iStep = 0;
	g_fCalibrate = 0;           // calibration transcript banked in scratch/147 (rb-siphon-01.txt)
	AddEffect("RunTest", 0, 1, 35, 0, 0);
	return true;
}

global func FxRunTestStart(target, effect, temp) { return 1; }

global func FxRunTestTimer(object target, int effect, int timer)
{
	++g_iStep;

	if (g_iStep == 1)
	{
		// The room's solution action: dig the cistern floor plug, then
		// kick one water cell to seed the flow (CascadeSmoke:76-77).
		DigFreeRect(145, 133, 30, 7);
		ExtractMaterialAmount(160, 130, Material("Water"), 1);
	}
	else
	{
		// Per-step guards (steps >= 2).
		if (GetPXSCount() > 10000)
			FatalError(Format("EscapeSiphonSmoke FAIL step %d: PXS budget exceeded (%d)",
			                  g_iStep, GetPXSCount()));
		if (GetMaterialCount(Material("Steam")) != 0)
			FatalError(Format("EscapeSiphonSmoke FAIL step %d: static steam appeared (%d)",
			                  g_iStep, GetMaterialCount(Material("Steam"))));
		// Routing liveness: water must actually arrive mid-run.
		if (g_iStep >= 5 && CountMatRegion(Material("Water"), BASIN_X1, BASIN_Y1, BASIN_X2, BASIN_Y2) <= 0)
			FatalError(Format("EscapeSiphonSmoke FAIL step %d: no water arrived in basin", g_iStep));
	}

	if (g_fCalibrate)
	{
		Log(Format("[CAL] step %d basin %d cist %d pxs %d",
		           g_iStep,
		           CountMatRegion(Material("Water"), BASIN_X1, BASIN_Y1, BASIN_X2, BASIN_Y2),
		           CountMatRegion(Material("Water"), CIST_X1, CIST_Y1, CIST_X2, CIST_Y2),
		           GetPXSCount()));
		Log(Format("[CAL]  shaft-mid %d wall-loss %d sky %d",
		           CountMatRegion(Material("Water"), BASIN_X1, 140, BASIN_X2, BASIN_Y1 - 1),
		           GetMaterialCount(Material("Water")) -
		               CountMatRegion(Material("Water"), 0, 0, 999, 399),
		           CountMatRegion(Material("Water"), 0, 0, 999, 199)));
	}

	if (g_iStep >= 9)
	{
		var basin = CountMatRegion(Material("Water"), BASIN_X1, BASIN_Y1, BASIN_X2, BASIN_Y2);
		if (basin < SIPHON_BASIN_MIN)
			FatalError(Format("EscapeSiphonSmoke FAIL: basin %d < %d (routing short)", basin, SIPHON_BASIN_MIN));
		Log(Format("EscapeSiphonSmoke: basin %d (of cistern %d, pxs %d)",
		           basin, g_iCist0, GetPXSCount()));
		Log("EscapeSiphonSmoke PASS");
		GameOver();
		return -1;
	}
	return 1;
}

/* Static material cells in the rectangle [x1,x2] x [y1,y2]. */
global func CountMatRegion(int mat, int x1, int y1, int x2, int y2)
{
	var count = 0;
	var x, y;
	for (x = x1; x <= x2; x++)
		for (y = y1; y <= y2; y++)
			if (GetMaterial(x, y) == mat)
				count++;
	return count;
}
