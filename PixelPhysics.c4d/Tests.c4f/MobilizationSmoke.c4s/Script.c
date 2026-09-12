/*-- MobilizationSmoke.c4s — mobilization perf gate (cycle 118). -------*/
/*                                                                       */
/* Generalizes the pxs_perf_gate contract (PXSPerfSmoke, cycle 88) from */
/* saturated PXS load to the mobilization paths:                        */
/*   breach churn  — ExtractMaterialAmount clears cells and fires       */
/*                   CheckInstabilityRange (C4Landscape.cpp:1344)       */
/*                   -> mass-mover chains;                              */
/*   blast churn   — BlastFree scrapes the sand terraces, exposing      */
/*                   instable neighbours -> cascade (movers chain-create*/
/*                   at transfer targets, C4MassMover.cpp:154);         */
/*   deposit churn — CastPXS rains mobile PXS back into the basin,      */
/*                   re-embedding static via the contact-reaction       */
/*                   InsertMaterial path (C4Material.cpp:845).          */
/*                                                                       */
/* Apparatus (flat map 1000x400, ground top y=200):                      */
/*   Zone A  water basin      x[100,400]: rock walls y[120,200],        */
/*                             static water fill y[150,199]             */
/*   Zone B  sand slope       x[450,700]: three sand terraces against   */
/*                             a rock backstop x[690,700] y[120,200]    */
/*   Zone C  collection sump  x[400,690] y[330,398]: rock-lined pit at  */
/*                             the map bottom, static-water-seeded via  */
/*                             InsertMaterial (200 cells)               */
/*                                                                       */
/* Per-frame churn recipe (FxMobChurnTimer, period 1):                   */
/*   SetWind(80); ExtractMaterialAmount(BREACH, Water, 3);               */
/*   BlastFree(roving terrace position, 8);                              */
/*   CastPXS("Water", 60, 100, CAST_X, CAST_Y).                          */
/*                                                                       */
/* Gate: GATE_WINDOW_MS = 1500 per 35-tick window (= 35 x 28 ms x 1.53   */
/* CI slack); 2-strike breach counter. MOB_FLOOR / EXT_FLOOR / W_FLOOR   */
/* non-vacuity + decay asserts from window 3 — each a distinct          */
/* FatalError class so a RED transcript names its own cause.            */
/*                                                                       */
/* Driver note: GLOBAL effects with GLOBAL callbacks (AddEffect(..., 0,  */
/* ...) + global func Fx...Timer); per-object timers do not fire in the  */
/* harness.                                                              */
/*                                                                       */
/* On any assertion failure, FatalError produces a non-zero exit code,   */
/* failing the CTest entry.                                              */

#strict 2

// ---- Gate constants (CAL-TUNE draft; frozen after the 3-run ----------
// ---- calibration, Task 3 of the cycle-118 plan) -----------------------
static const GATE_WINDOW_MS = 1500;  // 35 ticks x 28 ms budget x 1.53
static const PXS_CAP        = 10000; // C4PXS.h:40 hard-cap tripwire
static const MOB_FLOOR      = 300;   // CAL-TUNE: mobile-pxs floor (win >= 3)
static const EXT_FLOOR      = 60;    // CAL-TUNE: per-window extraction floor (win >= 3)
static const W_FLOOR        = 12000; // CAL-TUNE: static-water floor (win >= 3)

// ---- Apparatus coordinates (CAL-TUNE; adjust if a zone misbehaves) ----
static const BREACH_X   = 250; // basin-centre extraction bore
static const BREACH_Y   = 155; // near the top of the static water fill
static const BLAST_X0   = 460; // roving blast scrape, left edge
static const BLAST_SPAN = 220; // rove width across the Zone-B terraces
static const BLAST_Y    = 170; // mid-height of the terraces

static g_iStep;
static g_iFrame;
static g_iBreaches;
static g_iExtracted;  // cumulative cells extracted at the breach bore
static g_iExtMark;    // last window's cumulative extraction
static g_iBlasted;    // cumulative blast scrapes
static g_iBlastMark;  // last window's cumulative blast count
static g_iPeakPXS;
static g_tWin;

protected func Initialize()
{
	// --- Zone A: water basin (rock walls + static water fill) -----------
	DrawMaterialQuad("Rock", 100, 120, 110, 120, 110, 200, 100, 200);
	DrawMaterialQuad("Rock", 390, 120, 400, 120, 400, 200, 390, 200);
	DrawMaterialQuad("Water", 110, 150, 390, 150, 390, 199, 110, 199);

	// --- Zone B: sand slope (terraced wedge against a rock backstop) ----
	DrawMaterialQuad("Rock", 690, 120, 700, 120, 700, 200, 690, 200);
	DrawMaterialQuad("Sand", 450, 180, 690, 180, 690, 200, 450, 200);
	DrawMaterialQuad("Sand", 470, 160, 690, 160, 690, 180, 470, 180);
	DrawMaterialQuad("Sand", 490, 140, 690, 140, 690, 160, 490, 160);

	// --- Zone C: collection sump (rock-lined pit at the map bottom) -----
	DrawMaterialQuad("Rock", 400, 330, 410, 330, 410, 398, 400, 398);
	DrawMaterialQuad("Rock", 680, 330, 690, 330, 690, 398, 680, 398);
	DrawMaterialQuad("Rock", 400, 390, 690, 390, 690, 398, 400, 398);

	// Static-water seed via the registered InsertMaterial driver
	// (C4Script.cpp:7559): 200 direct static deposits into the sump —
	// the transport-creep baseline the [CAL] mob static lines track.
	for (var i = 0; i < 200; i++)
		InsertMaterial(Material("Water"), 420 + Random(240), 380);

	g_iStep = 0;
	g_iFrame = 0;
	g_iBreaches = 0;
	g_iExtracted = 0;
	g_iExtMark = 0;
	g_iBlasted = 0;
	g_iBlastMark = 0;
	g_iPeakPXS = 0;
	g_tWin = GetTime();
	AddEffect("MobWin", 0, 1, 35, 0, 0);
	AddEffect("MobChurn", 0, 1, 1, 0, 0);
	Log("[CAL] mob recipe: breach 3/frame, blast 8 roving, cast Water 60/frame, wind 80");
	return true;
}

global func FxMobWinStart(target, effect, temp) { return 1; }

global func FxMobChurnTimer(object target, int effect, int timer)
{
	// 1. Saltation gale (PXSPerfSmoke zone-D pattern).
	SetWind(80);

	// 2. Breach churn: carve the basin water column top-down; each
	//    extraction fires CheckInstabilityRange (C4Landscape.cpp:1344)
	//    -> mass-mover chains.
	g_iExtracted += ExtractMaterialAmount(BREACH_X, BREACH_Y, Material("Water"), 3);

	// 3. Blast scrape: one roving blast per frame on the Zone-B terraces;
	//    freed sand saltates and cascades (instability chain at the
	//    transfer target, C4MassMover.cpp:154).
	BlastFree(BLAST_X0 + (g_iFrame * 13) % BLAST_SPAN, BLAST_Y, 8);
	++g_iBlasted;

	// 4. Deposit churn: PXS rain into the basin (mobile -> static embed
	//    via the contact-reaction InsertMaterial path, C4Material.cpp:845).
	CastPXS("Water", 60, 100, 250, 100);

	++g_iFrame;
	return 1;
}

global func FxMobWinTimer(object target, int effect, int timer)
{
	++g_iStep;

	// Per-window wall-clock (THE GATE). 2-strike counter: a single-window
	// CI transient is absorbed; a sustained >= ~42.9 ms/tick slowdown
	// fails.
	var t = GetTime();
	var elapsed = t - g_tWin;
	g_tWin = t;
	if (elapsed > GATE_WINDOW_MS)
	{
		++g_iBreaches;
		if (g_iBreaches >= 2)
			FatalError(Format("MobilizationSmoke FAIL win %d: %d ms > GATE_WINDOW_MS (ext %d, pxs %d)",
			                  g_iStep, elapsed, g_iExtracted, GetPXSCount()));
	}

	// Belt-and-braces engine-bug tripwire (C4PXS.h:40 hard cap).
	if (GetPXSCount() > PXS_CAP)
		FatalError(Format("MobilizationSmoke FAIL win %d: PXS cap exceeded (%d > %d)",
		                  g_iStep, GetPXSCount(), PXS_CAP));

	// Non-vacuity (window >= 3): the mobilized load must hold.
	if (g_iStep >= 3 && GetPXSCount() < MOB_FLOOR)
		FatalError(Format("MobilizationSmoke FAIL win %d: pxs %d < MOB_FLOOR (load collapsed)",
		                  g_iStep, GetPXSCount()));

	// Breach churn must actually run (window >= 3).
	if (g_iStep >= 3 && g_iExtracted - g_iExtMark < EXT_FLOOR)
		FatalError(Format("MobilizationSmoke FAIL win %d: ext %d < EXT_FLOOR (breach churn vacuous)",
		                  g_iStep, g_iExtracted - g_iExtMark));

	// Decay property (window >= 3): rain replenishment must keep the
	// static water above the floor (extraction outpacing rain -> red).
	if (g_iStep >= 3 && GetMaterialCount(Material("Water")) < W_FLOOR)
		FatalError(Format("MobilizationSmoke FAIL win %d: basin drained (W %d < W_floor %d)",
		                  g_iStep, GetMaterialCount(Material("Water")), W_FLOOR));

	if (GetPXSCount() > g_iPeakPXS)
		g_iPeakPXS = GetPXSCount();

	// [CAL] per-window trend lines: per-path churn decomposition (the
	// per-path regression baseline the L cycle inherits) + statics
	// (the mover-transport signature: water shifts basin->sump at
	// ~constant total). Format arg count <= 9 (cycle-88 constraint).
	Log(Format("[CAL] mob win %d: %d ms, ext %d, blast %d, pxs %d",
	            g_iStep, elapsed, g_iExtracted - g_iExtMark,
	            g_iBlasted - g_iBlastMark, GetPXSCount()));
	Log(Format("[CAL] mob static W=%d S=%d",
	            GetMaterialCount(Material("Water")),
	            GetMaterialCount(Material("Sand"))));
	g_iExtMark = g_iExtracted;
	g_iBlastMark = g_iBlasted;

	if (g_iStep >= 9)
	{
		Log(Format("[CAL] mob total windows %d, breaches %d, ext %d, peak pxs %d",
		           g_iStep, g_iBreaches, g_iExtracted, g_iPeakPXS));
		Log("MobilizationSmoke PASS");
		GameOver();
		return -1;
	}
	return 1;
}
