/*-- EscapeUndertowSmoke.c4s — headless sand-cascade smoke. --*/
/*                                                              */
/* Pins the cycle-147 "The Undertow" keystone-undercut          */
/* contract end-to-end: the sandstone keystone shelf under a    */
/* static sand column is blasted; CheckInstabilityRange fires   */
/* for the urbasted sand (blast path, C4Landscape.cpp:1146)     */
/* and the instable column (Sand.c4m Instable=1, MaxSlide=1)    */
/* cascades through the gap into the sump cavity below.         */
/*                                                              */
/* Sand collapse is disturbance-driven only (no autonomous      */
/* landslide — Epic 5 unbuilt): the assert is the END STATE of  */
/* the painted geometry — the choke ROI is empty of sand.       */
/*                                                              */
/* Landscape contract: the flat default map (Material=Earth) is */
/* 1000x400 px, ground top at y=200 (=G). The painted apparatus */
/* straddles the sky/ground seam: rock shaft walls x[400,540],  */
/* a sandstone keystone shelf at y[190,198] just above ground,  */
/* the sand column above it, and a DigFreeRect sump carved      */
/* below the keystone into the ground.                          */
/*                                                              */
/* Driver note: the step timer is a GLOBAL effect with GLOBAL   */
/* callbacks (SnowMeltSmoke pattern); Objects.c4d loaded so     */
/* System links cleanly. On any assertion failure, FatalError   */
/* produces a non-zero exit, failing the CTest entry.           */
/*                                                              */
/* Calibration: set g_fCalibrate=1 to log per-step sand in the  */
/* choke ROI and the mover count; freeze UNDERTOW_CLEAR_MAX     */
/* with >=2x margin over the worst observed green residual,     */
/* then flip back to 0 (SandFlowSmoke discipline). FIX THE      */
/* GEOMETRY, never the threshold, if the column hangs.          */

#strict 2

static const UNDERTOW_CLEAR_MAX = 50;   // frozen: drained choke residual 12 px was
                                        // the worst green observation (>=4x margin);
                                        // a hung column reads >= 1700 (R2 geometry
                                        // fix, never threshold relaxation).
static const COL_WDT = 80;              // painted column 80x80 px
static const COL_HGT = 80;
static const COL_X1  = 430;             // column footprint
static const COL_Y1  = 110;
static const COL_X2  = 510;
static const COL_Y2  = 190;
static const KEY_X1  = 420;             // keystone shelf footprint
static const KEY_Y1  = 190;
static const KEY_X2  = 520;
static const KEY_Y2  = 198;
static const SUM_X1  = 420;             // sump cavity below the keystone
static const SUM_Y1  = 198;
static const SUM_X2  = 520;
static const SUM_Y2  = 288;
static const CHK_X1  = 420;             // choke ROI = where the column stood
static const CHK_Y1  = 110;
static const CHK_X2  = 520;
static const CHK_Y2  = 190;

static g_iStep;
static g_iPeakMovers;
static g_fCalibrate;

protected func Initialize()
{
	// Rock shaft walls (painted over the sky/ground seam; below y=200
	// they replace the default Earth fill).
	DrawMaterialQuad("Rock", 400, 100, 420, 100, 420, 300, 400, 300);
	DrawMaterialQuad("Rock", 520, 100, 540, 100, 540, 300, 520, 300);

	// Sandstone keystone shelf spanning the shaft at ground level.
	DrawMaterialQuad("Sandstone", KEY_X1, KEY_Y1, KEY_X2, KEY_Y1, KEY_X2, KEY_Y2, KEY_X1, KEY_Y2);

	// Static sand column resting on the keystone (the choke).
	DrawMaterialQuad("Sand", COL_X1, COL_Y1, COL_X2, COL_Y1, COL_X2, COL_Y2, COL_X1, COL_Y2);

	// Sump cavity directly below the keystone (carved into the Earth).
	// Deep enough to hold the whole 80x80 column with margin (observed
	// 12 px residual on the drained run; capacity 100x90 = 9000 px).
	DigFreeRect(SUM_X1, SUM_Y1, SUM_X2 - SUM_X1, SUM_Y2 - SUM_Y1);

	// Paint-baseline asserts (CascadeSmoke:39-53 pattern): column >= 90%
	// of the painted footprint, keystone shelf present, sump empty.
	var n = CountMatRegion(Material("Sand"), COL_X1, COL_Y1, COL_X2, COL_Y2);
	if (n < COL_WDT * COL_HGT * 9 / 10)
		FatalError(Format("EscapeUndertowSmoke FAIL: column short (%d, expect ~%d)",
		                  n, COL_WDT * COL_HGT));
	n = CountMatRegion(Material("Sandstone"), KEY_X1, KEY_Y1, KEY_X2, KEY_Y2);
	if (n < (KEY_X2 - KEY_X1) * (KEY_Y2 - KEY_Y1) * 9 / 10)
		FatalError(Format("EscapeUndertowSmoke FAIL: keystone short (%d)", n));
	n = CountMatRegion(Material("Sand"), SUM_X1, SUM_Y1, SUM_X2, SUM_Y2);
	if (n != 0)
		FatalError(Format("EscapeUndertowSmoke FAIL: sump not empty (%d)", n));

	Log(Format("EscapeUndertowSmoke: baseline column %d keystone %d",
	           CountMatRegion(Material("Sand"), COL_X1, COL_Y1, COL_X2, COL_Y2),
	           CountMatRegion(Material("Sandstone"), KEY_X1, KEY_Y1, KEY_X2, KEY_Y2)));

	g_iStep = 0;
	g_iPeakMovers = 0;
	g_fCalibrate = 0;
	AddEffect("RunTest", 0, 1, 35, 0, 0);
	// Per-tick cascade watcher: the 35-tick step sampler misses the fast
	// mover spike right after the blast (the radius-40 undercut drains
	// the exposed column within ~35 ticks). Records the running max.
	AddEffect("UndertowWatch", 0, 1, 1, 0, 0);
	return true;
}

global func FxRunTestStart(target, effect, temp) { return 1; }

global func FxUndertowWatchStart(target, effect, temp) { return 1; }
global func FxUndertowWatchTimer(object target, int effect, int timer)
{
	var movers = GetMassMoverCount();
	if (movers > g_iPeakMovers) g_iPeakMovers = movers;
	return 1;
}

global func FxRunTestTimer(object target, int effect, int timer)
{
	++g_iStep;

	if (g_iStep == 1)
	{
		// Solution action: undermine the keystone. Calibration note
		// (R2): the plan's radius-10 undercut measured a ~1709 px hang at
		// the choke (sand pillars between the hole and the walls arch over
		// the 20px gap — granular sand MaxSlide=1 cannot pour sideways like
		// CascadeSmoke's liquid). The undercut is widened to span the whole
		// shaft so no sand pixel keeps support and the column cascades into
		// the sump. Geometry fix, threshold unchanged (R2 discipline).
		// BlastFree fires CheckInstabilityRange per pixel (C4Landscape:1146).
		BlastFree(470, 194, 40);
	}
	else
	{
		// Per-step guards (steps >= 2).
		if (GetPXSCount() > 10000)
			FatalError(Format("EscapeUndertowSmoke FAIL step %d: PXS budget exceeded (%d)",
			                  g_iStep, GetPXSCount()));
	}

	if (g_fCalibrate)
		Log(Format("[CAL] step %d: choke sand %d, movers %d, pxs %d",
		           g_iStep,
		           CountMatRegion(Material("Sand"), CHK_X1, CHK_Y1, CHK_X2, CHK_Y2),
		           GetMassMoverCount(), GetPXSCount()));

	if (g_iStep >= 9)
	{
		var iSand = CountMatRegion(Material("Sand"), CHK_X1, CHK_Y1, CHK_X2, CHK_Y2);
		Log(Format("[CAL] final: choke sand %d, peak movers %d, pxs %d",
		           iSand, g_iPeakMovers, GetPXSCount()));

		// The cascade end-state: the choke ROI holds <= the frozen
		// threshold (>=2x margin over the worst green residual), and
		// a sand-cascade actually ran while the column drained.
		if (iSand > UNDERTOW_CLEAR_MAX)
			FatalError(Format("EscapeUndertowSmoke FAIL: choke not cleared (sand %d > %d, peak movers %d)",
			                  iSand, UNDERTOW_CLEAR_MAX, g_iPeakMovers));
		if (g_iPeakMovers < 1)
			FatalError("EscapeUndertowSmoke FAIL: no cascade (peak movers 0)");

		Log("EscapeUndertowSmoke PASS");
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
