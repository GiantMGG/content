/*-- PXSPerfSmoke.c4s — headless PXS frame-budget perf gate (cycle 88). --*/
/*                                                                       */
/* Pins the Epic 2 physics features (PXS pipeline + material reactions    */
/* + saltation + mass movers) at a playable frame budget: a saturated    */
/* five-material load driven toward the 10k PXS cap, with per-window     */
/* wall-clock asserts and [CAL] logging for baseline trending.           */
/*                                                                       */
/* Apparatus (flat map 1000x400, ground top y=200):                      */
/*   A lava field x[100,500] (static lava fill x[150,450] y[135,150])    */
/*   B oil pan x[540,720] (pre-filled Oil x[550,710] y[165,190])         */
/*   C acid pool x[760,940] (pre-filled Acid x[770,930] y[165,190])      */
/*   D sand bed x[20,90] y[150,200] + gale Wind=80 (saltation)           */
/*                                                                       */
/* Load recipe (PXS cast per frame): water 50, lava 10, oil 30,          */
/* acid 20, sand 40 (= 150 PXS/frame toward the 10k cap).                */
/*                                                                       */
/* Gate: GATE_WINDOW_MS = 1500 per 35-tick window (= 35 x 28 ms budget   */
/* x ~1.5 headroom); 2-strike breach counter absorbs single-window CI    */
/* transients. PXS_FLOOR asserts non-vacuity from window 3.              */
/*                                                                       */
/* Driver note: the timers are GLOBAL effects with GLOBAL callbacks      */
/* (AddEffect(..., 0, ...) + global func Fx...Timer); per-object         */
/* AddEffect timers do not fire in the harness.                          */
/*                                                                       */
/* On any assertion failure, FatalError produces a non-zero exit code,   */
/* failing the CTest entry.                                              */

#strict 2

// ---- Frozen gate constants (calibrate, then freeze) -------------------
static const GATE_WINDOW_MS = 1500;  // 35 ticks x 28 ms budget x ~1.5
static const PXS_FLOOR      = 1361;  // non-vacuity floor (<= 1/3 steady state)
static const PXS_CAP        = 10000; // C4PXS.h:40 hard-cap tripwire

static g_iStep;
static g_iBreaches;
static g_iPeakPXS;
static g_tWin;

protected func Initialize()
{
	// Zone A: rock box x[100,500] y[60,160] + static lava fill.
	DrawMaterialQuad("Rock", 100, 60, 110, 60, 110, 160, 100, 160);
	DrawMaterialQuad("Rock", 490, 60, 500, 60, 500, 160, 490, 160);
	DrawMaterialQuad("Rock", 100, 150, 500, 150, 500, 160, 100, 160);
	DrawMaterialQuad("Lava", 150, 135, 450, 135, 450, 150, 150, 150);

	// Zone B: oil pan (rock basin + static oil fill).
	DrawMaterialQuad("Rock", 540, 60, 550, 60, 550, 200, 540, 200);
	DrawMaterialQuad("Rock", 710, 60, 720, 60, 720, 200, 710, 200);
	DrawMaterialQuad("Rock", 540, 190, 720, 190, 720, 200, 540, 200);
	DrawMaterialQuad("Oil", 550, 165, 710, 165, 710, 190, 550, 190);

	// Zone C: acid pool (rock basin + static acid fill).
	DrawMaterialQuad("Rock", 760, 60, 770, 60, 770, 200, 760, 200);
	DrawMaterialQuad("Rock", 930, 60, 940, 60, 940, 200, 930, 200);
	DrawMaterialQuad("Rock", 760, 190, 940, 190, 940, 200, 760, 200);
	DrawMaterialQuad("Acid", 770, 165, 930, 165, 930, 190, 770, 190);

	// Zone D: sand bed (static; churns via mass movers + saltation).
	DrawMaterialQuad("Sand", 20, 150, 90, 150, 90, 200, 20, 200);

	// Mass-mover kickstarts (CascadeSmoke Script.c:76-77 breach pattern:
	// ExtractMaterialAmount fires CheckInstabilityRange, C4Landscape.cpp:1321)
	// - one per liquid zone.
	ExtractMaterialAmount(300, 140, Material("Lava"), 1);
	ExtractMaterialAmount(620, 180, Material("Oil"), 1);
	ExtractMaterialAmount(840, 180, Material("Acid"), 1);

	Log("[CAL] recipe: water 50 lava 10 oil 30 acid 20 sand 40 per frame, wind 80");

	g_iStep = 0;
	g_iBreaches = 0;
	g_iPeakPXS = 0;
	g_tWin = GetTime();
	AddEffect("RunTest", 0, 1, 35, 0, 0);
	AddEffect("PxsSpam", 0, 1, 1, 0, 0);
	return true;
}

global func FxRunTestStart(target, effect, temp) { return 1; }

global func FxPxsSpamTimer(object target, int effect, int timer)
{
	// Pin the saltation gale, then cast the per-frame load recipe.
	SetWind(80);
	CastPXS("Water", 50, 100, 300, 100);  // onto the lava field (zone A)
	CastPXS("Lava", 10, 20, 300, 100);    // lava PXS into the water spray: mid-air reactions
	CastPXS("Oil", 30, 60, 620, 120);     // onto the oil pan (zone B)
	CastPXS("Acid", 20, 60, 840, 120);    // onto the acid pool (zone C)
	CastPXS("Sand", 40, 20, 460, 60);     // upwind cast; wind 80 carries grains
	return 1;
}

global func FxRunTestTimer(object target, int effect, int timer)
{
	++g_iStep;

	// Per-window wall-clock (THE GATE). 2-strike counter: a single-window
	// CI transient is absorbed; a sustained >= ~42 ms/tick slowdown fails.
	var t = GetTime();
	var elapsed = t - g_tWin;
	g_tWin = t;
	if (elapsed > GATE_WINDOW_MS)
	{
		++g_iBreaches;
		if (g_iBreaches >= 2)
			FatalError(Format("PXSPerfSmoke FAIL win %d: %d ms > GATE_WINDOW_MS (pxs %d)",
			                  g_iStep, elapsed, GetPXSCount()));
	}

	// Belt-and-braces engine-bug tripwire (C4PXS.h:40 hard cap).
	if (GetPXSCount() > PXS_CAP)
		FatalError(Format("PXSPerfSmoke FAIL win %d: PXS cap exceeded (%d > %d)",
		                  g_iStep, GetPXSCount(), PXS_CAP));

	// Steam stays PXS-only (PXSBudgetSmoke:61-63 pattern).
	if (GetMaterialCount(Material("Steam")) != 0)
		FatalError(Format("PXSPerfSmoke FAIL win %d: static steam appeared (%d)",
		                  g_iStep, GetMaterialCount(Material("Steam"))));

	// Non-vacuity: the saturated load must hold from window 3 on.
	if (g_iStep >= 3 && GetPXSCount() < PXS_FLOOR)
		FatalError(Format("PXSPerfSmoke FAIL win %d: pxs %d < PXS_FLOOR (load collapsed)",
		                  g_iStep, GetPXSCount()));

	if (GetPXSCount() > g_iPeakPXS)
		g_iPeakPXS = GetPXSCount();

	// [CAL] baseline trend lines (read from LastTest.log).
	Log(Format("[CAL] win %d: %d ms, pxs %d, peak %d",
	            g_iStep, elapsed, GetPXSCount(), g_iPeakPXS));
	Log(Format("[CAL] static W=%d O=%d L=%d A=%d S=%d",
	            GetMaterialCount(Material("Water")),
	            GetMaterialCount(Material("Oil")),
	            GetMaterialCount(Material("Lava")),
	            GetMaterialCount(Material("Acid")),
	            GetMaterialCount(Material("Sand"))));

	if (g_iStep >= 9)
	{
		Log(Format("[CAL] total windows %d, breaches %d, peak pxs %d",
		           g_iStep, g_iBreaches, g_iPeakPXS));
		Log("PXSPerfSmoke PASS");
		GameOver();
		return -1;
	}
	return 1;
}
