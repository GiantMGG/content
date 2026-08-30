/*-- DuneBurialSmoke.c4s -- headless sand-drift + saltation smoke. --*/
/*                                                                   */
/* Pins the cycle-80 contract (spec 2026-08-30-2345):                */
/*   U1 saltation: resting Sand PXS hop downwind at |wind| >= 30     */
/*   U2/U3 drift: the SDRF director moves budget px per cadence,     */
/*                ramping to 2 + intensity/10 under SNDT             */
/*   U4 burial: walkers in a deposit footprint get SandBuried        */
/*                (Walk cut; escape restores)                        */
/*                                                                   */
/* Landscape contract: the flat default map (no Landscape= key,      */
/* Material=Earth, cf. the PixelPhysics smokes) has open sky in      */
/* rows [0,200) and a flat earth top at row 200. Every assertion     */
/* probes the smoke's own painted platform, so the run is            */
/* frame-order-deterministic and independent of the map seed.        */
/*                                                                   */
/* Phase 1 (synchronous, zero frames): director budget math, mass    */
/* conservation, storm ramp, walker burial/escape.                   */
/* Phase 2 (frame-driven, 350-tick window): cast grains saltate off  */
/* the platform's upwind end and embed downwind.                     */
/*                                                                   */
/* On any assertion failure, FatalError produces a non-zero exit     */
/* code, failing the CTest entry.                                    */

#strict 3

static g_iStep;
static g_iProbeSand0;

protected func Initialize()
{
	RunSmokeSteps();
	// Frame-driven step driver (PXSBudgetSmoke pattern: a GLOBAL
	// effect with GLOBAL callbacks; per-object AddEffect timers do
	// not fire reliably in the smoke harness).
	AddEffect("RunTest", nil, 1, 35, nil, nil);
	return true;
}

func RunSmokeSteps()
{
	var sand = Material("Sand");

	/* Step 0: paint the drift platform -- a 301-column sand slab  */
	/* in the open-sky band, top at row 150, sitting on the earth.  */
	DrawMaterialQuad("Sand", 100, 150, 400, 150, 400, 200, 100, 200, false);
	if (GetMaterial(250, 150) != sand)
		FatalError("DuneBurialSmoke FAIL step 0: platform top row is not Sand");
	if (GetMaterial(250, 149) != -1)
		FatalError("DuneBurialSmoke FAIL step 0: sky above platform top is not open");
	if (GetMaterial(250, 180) != sand)
		FatalError("DuneBurialSmoke FAIL step 0: platform interior is not Sand");

	/* Step 1: director idles below the saltation wind threshold.  */
	SetWind(10);
	var sdrf = CreateObject(SDRF, 250, 150, NO_OWNER);
	if (!sdrf) FatalError("DuneBurialSmoke FAIL step 1: SDRF not spawned");
	if (sdrf->Drift() != 0)
		FatalError("DuneBurialSmoke FAIL step 1: Drift() moved sand at wind 10");

	/* Step 2: base drift moves exactly the base budget (2 px) and */
	/* conserves static sand mass (extract+insert pair).           */
	SetWind(60);
	var before = GetMaterialCount(sand);
	var moved = sdrf->Drift();
	if (moved != 2)
		FatalError(Format("DuneBurialSmoke FAIL step 2: Drift() moved %d, want 2", moved));
	if (GetMaterialCount(sand) != before)
		FatalError(Format("DuneBurialSmoke FAIL step 2: sand mass not conserved (%d -> %d)",
			before, GetMaterialCount(sand)));

	/* Step 3: SNDT storm ramp -- budget = 2 + intensity/10 = 7.   */
	LaunchWeatherEvent(SNDT, 50, 100);
	if (GetActiveWeatherEvent() != SNDT)
		FatalError("DuneBurialSmoke FAIL step 3: SNDT not active");
	SetWind(60);
	var storm_moved = sdrf->Drift();
	if (storm_moved != 7)
		FatalError(Format("DuneBurialSmoke FAIL step 3: storm Drift() moved %d, want 7", storm_moved));
	StopWeatherEvent();
	if (GetActiveWeatherEvent() != nil)
		FatalError("DuneBurialSmoke FAIL step 3: SNDT not stopped");

	/* Step 4: structure burial -- a walker in the deposit         */
	/* footprint gets SandBuried (Walk cut); jumping clear ends     */
	/* the effect and restores Walk.                                */
	var victim = CreateObject(SCRP, 250, 150, NO_OWNER);
	if (!victim) FatalError("DuneBurialSmoke FAIL step 4: SCRP victim not spawned");
	var walk_base = GetPhysical("Walk", 0, victim);
	var i;
	for (i = 0; i < 400 && !GetEffect("SandBuried", victim); i++)
		sdrf->Drift();
	var buried = GetEffect("SandBuried", victim);
	if (!buried)
		FatalError("DuneBurialSmoke FAIL step 4: deposits never buried the walker");
	if (GetPhysical("Walk", 0, victim) != 20000)
		FatalError("DuneBurialSmoke FAIL step 4: SandBuried did not cut Walk");
	/* Escape: lift the victim clear of the deposit; the effect    */
	/* timer must then end the effect (FX_Execute_Kill = -1).       */
	SetPosition(GetX(victim), GetY(victim) - 30, victim);
	if (!sdrf->FxSandBuriedTimer(victim, buried, 1))
		FatalError("DuneBurialSmoke FAIL step 4: SandBuried timer did not end off-deposit");
	sdrf->FxSandBuriedStop(victim, buried, 0, false);
	if (GetPhysical("Walk", 0, victim) != walk_base)
		FatalError("DuneBurialSmoke FAIL step 4: Walk physical not restored after escape");

	/* Step 5: saltation setup -- pin gale wind, cast a thin grain   */
	/* layer onto the platform's upwind end, baseline the downwind   */
	/* probe region, and remove the director so the frame phase      */
	/* measures pure saltation. The cast is deliberately thin        */
	/* (2 grains per column): tall piles swallow their own hops      */
	/* (+30 px flights re-land inside the pile), while a thin layer  */
	/* launches grains from the platform top itself, so landings     */
	/* embed downwind of the layer edge.                             */
	SetWind(80);
	var cx;
	for (cx = 130; cx <= 149; cx++)
		CastPXS("Sand", 2, 10, cx, 146);
	/* Probe region: columns [160,400], rows [140,150). The cast    */
	/* layer embeds within [125,155] even without hops (M1), while  */
	/* hop flights (+~30 px) embed at [158,190] and chained bounces */
	/* further downwind.                                             */
	g_iProbeSand0 = CountSandRegion(160, 400, 140, 149);
	RemoveObject(sdrf);

	Log("DuneBurialSmoke sync phase done");
	return true;
}

/* Topmost open-sky Sand pixel of column x, else -1 (SDRF logic). */
global func SandSurfaceScan(int x)
{
	var y;
	for (y = 0; y < LandscapeHeight(); y++)
	{
		var mat = GetMaterial(x, y);
		if (mat == -1) continue;
		if (mat == Material("Sand")) return y;
		return -1;
	}
	return -1;
}

/* Static Sand cells in the rectangle [x1,x2] x [y1,y2]. */
global func CountSandRegion(int x1, int x2, int y1, int y2)
{
	var count = 0;
	var x, y;
	for (x = x1; x <= x2; x++)
		for (y = y1; y <= y2; y++)
			if (GetMaterial(x, y) == Material("Sand"))
				count++;
	return count;
}

global func FxRunTestStart(target, effect, temp) { return 1; }

global func FxRunTestTimer(object target, int effect, int timer)
{
	++g_iStep;

	// PXS budget guard (10k hard cap).
	if (GetPXSCount() > 10000)
		FatalError(Format("DuneBurialSmoke FAIL step %d: PXS budget exceeded (%d)",
			g_iStep, GetPXSCount()));

	if (g_iStep >= 9)
	{
		// Saltation probe: the frame phase must have embedded new
		// static Sand downwind of the cast zone. Without the hop
		// branch (negative mutation M1) the delta is exactly 0.
		var probe = CountSandRegion(160, 400, 140, 149);
		if (probe <= g_iProbeSand0)
			FatalError(Format("DuneBurialSmoke FAIL: no saltation embeds downwind (%d <= %d)",
				probe, g_iProbeSand0));

		Log("DuneBurialSmoke PASS");
		GameOver();
		return -1;
	}
	return 1;
}
