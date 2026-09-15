/*-- ReactionLabSmoke.c4s - headless reaction-web smoke (cycle 128). --*/
/*   Beat 0 Quench    : Water PXS onto static Lava -> Rock + Steam      */
/*   Beat 1 Pillow    : Lava PXS onto static Water -> Rock crust        */
/*   Beat 2 Oilburn   : Lava PXS ignites static Oil; Water kills blaze  */
/*   Beat 3 Dissolve  : Acid corrodes Sulphur > Ice > Sand; Granite 0   */
/*   Beat 4 Duneworks : Wind 80 saltates Sand into lee deposition zone  */
/*                                                                     */
/* Global-effect driver (PXSPerfSmoke:76-77 precedent). FatalError in   */
/* an effect timer exits 0 - RED detection greps the log for FAIL:.     */
/* Seed pinned via SMOKE_SEED_PIN_ReactionLabSmoke (saltation beat).    */

#strict 2

// ---- Tunable thresholds (calibrate on the pinned seed, then freeze) ----
static const SMK_QUENCH_ROCK_MIN = 40;
static const SMK_PILLOW_ROCK_MIN = 40;
static const SMK_OIL_BURN_MIN    = 500;
static const SMK_DISSOLVE_MIN    = 7;
static const SMK_DUNE_SAND_MIN   = 40;

static g_iStep;
static g_iRockQ0, g_iLavaQ0;
static g_iRockP0;
static g_iOilO0;
static g_iS0, g_iI0, g_iSa0, g_iG0;
static g_fFlamFound, g_fOilExtinguished;

protected func Initialize()
{
	// Beat 0 - Quench band x[40,200]: rock box, lava fill x[50,190] y[130,150].
	DrawMaterialQuad("Rock", 40, 60, 50, 60, 50, 160, 40, 160);
	DrawMaterialQuad("Rock", 190, 60, 200, 60, 200, 160, 190, 160);
	DrawMaterialQuad("Rock", 40, 150, 200, 150, 200, 160, 40, 160);
	DrawMaterialQuad("Lava", 50, 130, 190, 130, 190, 150, 50, 150);

	// Beat 1 - Pillow band x[240,400]: rock box, water fill.
	DrawMaterialQuad("Rock", 240, 60, 250, 60, 250, 160, 240, 160);
	DrawMaterialQuad("Rock", 390, 60, 400, 60, 400, 160, 390, 160);
	DrawMaterialQuad("Rock", 240, 150, 400, 150, 400, 160, 240, 160);
	DrawMaterialQuad("Water", 250, 130, 390, 130, 390, 150, 250, 150);

	// Beat 2 - Oilburn band x[440,600]: rock shelf, oil fill.
	DrawMaterialQuad("Rock", 440, 60, 450, 60, 450, 160, 440, 160);
	DrawMaterialQuad("Rock", 590, 60, 600, 60, 600, 160, 590, 160);
	DrawMaterialQuad("Rock", 440, 150, 600, 150, 600, 160, 440, 160);
	DrawMaterialQuad("Oil", 450, 130, 590, 130, 590, 150, 450, 150);

	// Beat 3 - Dissolve band x[640,800]: bench + four sample plates.
	DrawMaterialQuad("Rock", 640, 150, 800, 150, 800, 160, 640, 160);
	DrawMaterialQuad("Sulphur", 650, 140, 670, 140, 670, 150, 650, 150);
	DrawMaterialQuad("Ice", 680, 140, 700, 140, 700, 150, 680, 150);
	DrawMaterialQuad("Sand", 710, 140, 730, 140, 730, 150, 710, 150);
	DrawMaterialQuad("Granite", 745, 140, 765, 140, 765, 150, 745, 150);

	// Beat 4 - Duneworks band: sand slab runway x[801,899] top y150; the
	// lee deposition zone lies east x[900,999] behind the slab's open east
	// edge. Sunken well below beat-3's probe band (x[640,800]), the slab
	// never pollutes the Dissolve baselines. (DuneBurial runway precedent.)
	DrawMaterialQuad("Sand", 801, 150, 899, 150, 899, 200, 801, 200, false);

	// Baselines.
	g_iRockQ0 = CountMatRegion(Material("Rock"), 40, 60, 200, 160);
	g_iLavaQ0 = CountMatRegion(Material("Lava"), 40, 60, 200, 160);
	g_iRockP0 = CountMatRegion(Material("Rock"), 240, 60, 400, 160);
	g_iOilO0  = CountMatRegion(Material("Oil"), 440, 60, 600, 160);
	g_iS0  = CountMatRegion(Material("Sulphur"), 640, 130, 800, 160);
	g_iI0  = CountMatRegion(Material("Ice"), 640, 130, 800, 160);
	g_iSa0 = CountMatRegion(Material("Sand"), 640, 130, 800, 160);
	g_iG0  = CountMatRegion(Material("Granite"), 640, 130, 800, 160);

	g_iStep = 0;
	g_fFlamFound = false;
	g_fOilExtinguished = false;
	AddEffect("RunTest", 0, 1, 35, 0, 0);
	AddEffect("WindPin", 0, 1, 1, 0, 0);
	return true;
}

global func FxRunTestStart(target, effect, temp) { return 1; }
global func FxWindPinStart(target, effect, temp) { return 1; }

global func FxWindPinTimer(object target, int effect, int timer)
{
	// Hold the saltation gale steady for the Duneworks beat.
	SetWind(80);
	return 1;
}

global func FxRunTestTimer(object target, int effect, int timer)
{
	++g_iStep;

	if (g_iStep == 1)
	{
		// Beat 0: water onto the lava pool.
		CastPXS("Water", 400, 20, 120, 100);
		// Beat 1: lava onto the water basin.
		CastPXS("Lava", 400, 20, 320, 100);
		// Beat 2: lava PXS onto the oil slick -> ignite.
		CastPXS("Lava", 100, 50, 520, 100);
		// Beat 3: acid over the sample plates (calibrated: flush burst at
		// plate-top height, ~±2 px/tick spread accretes on all three plates).
		CastPXS("Acid", 6000, 42, 660, 140);
	}

	// Sticky FLAM detection for the Oilburn beat.
	if (FindObject(FLAM)) g_fFlamFound = true;

	// Beat 4: thin sand grain layer on the slab's upwind end. The gale
	// saltates the resting grains east along the runway top (rows 140-149)
	// and off the east edge x899 into the lee deposition zone; at wind 0
	// the grains stay parked on the slab (dune stays ~0). Cast at step 8
	// (after the quench flush steps 4-7 and the step-6 poll) so the
	// wind-sensitivity of the sand never perturbs the RNG behind beats 0-3.
	if (g_iStep == 8)
	{
		var ctw;
		for (ctw = 880; ctw <= 899; ctw++)
			CastPXS("Sand", 4, 10, ctw, 146);
	}

	if (g_iStep >= 4 && g_iStep <= 7)
	{
		// Beat 2 act (b): quench flush along the oil surface.
		CastPXS("Water", 1000, 30, 450, 130);
		CastPXS("Water", 1000, 30, 490, 130);
		CastPXS("Water", 1000, 30, 530, 130);
		CastPXS("Water", 1000, 30, 570, 130);
	}
	if (g_iStep == 6)
	{
		if (FindObject(FLAM)) g_fFlamFound = true; // already sticky; keep polling
		if (!FindObject(FLAM)) g_fOilExtinguished = true;
	}

	if (g_iStep >= 9)
	{
		// ---- Beat 0: Quench ----
		var iRockQ = CountMatRegion(Material("Rock"), 40, 60, 200, 160);
		var iLavaQ = CountMatRegion(Material("Lava"), 40, 60, 200, 160);
		if (iRockQ - g_iRockQ0 < SMK_QUENCH_ROCK_MIN)
			FatalError(Format("ReactionLabSmoke FAIL beat 0: rock delta %d < %d",
			                  iRockQ - g_iRockQ0, SMK_QUENCH_ROCK_MIN));
		if (iLavaQ >= g_iLavaQ0)
			FatalError(Format("ReactionLabSmoke FAIL beat 0: lava not consumed (%d -> %d)",
			                  g_iLavaQ0, iLavaQ));
		// Steam stays PXS-only: never a static deposit (LavaWaterSmoke invariant).
		if (GetMaterialCount(Material("Steam")) != 0)
			FatalError(Format("ReactionLabSmoke FAIL beat 0: static steam appeared (%d)",
			                  GetMaterialCount(Material("Steam"))));

		// ---- Beat 1: Pillow ----
		var iRockP = CountMatRegion(Material("Rock"), 240, 60, 400, 160);
		if (iRockP - g_iRockP0 < SMK_PILLOW_ROCK_MIN)
			FatalError(Format("ReactionLabSmoke FAIL beat 1: pillow rock delta %d < %d",
			                  iRockP - g_iRockP0, SMK_PILLOW_ROCK_MIN));

		// ---- Beat 2: Oilburn ----
		var iOil = CountMatRegion(Material("Oil"), 440, 60, 600, 160);
		if (g_iOilO0 - iOil < SMK_OIL_BURN_MIN)
			FatalError(Format("ReactionLabSmoke FAIL beat 2: oil consumed only %d of %d",
			                  g_iOilO0 - iOil, g_iOilO0));
		if (!g_fFlamFound)
			FatalError("ReactionLabSmoke FAIL beat 2: slick never ignited (no FLAM)");
		if (!g_fOilExtinguished)
			FatalError("ReactionLabSmoke FAIL beat 2: blaze not extinguished by water");

		// ---- Beat 3: Dissolve ----
		var dS  = g_iS0  - CountMatRegion(Material("Sulphur"), 640, 130, 800, 160);
		var dI  = g_iI0  - CountMatRegion(Material("Ice"), 640, 130, 800, 160);
		var dSa = g_iSa0 - CountMatRegion(Material("Sand"), 640, 130, 800, 160);
		var dG  = g_iG0  - CountMatRegion(Material("Granite"), 640, 130, 800, 160);
		if (dS < SMK_DISSOLVE_MIN)
			FatalError(Format("ReactionLabSmoke FAIL beat 3: sulphur dissolved %d < %d", dS, SMK_DISSOLVE_MIN));
		if (dI < SMK_DISSOLVE_MIN)
			FatalError(Format("ReactionLabSmoke FAIL beat 3: ice dissolved %d < %d", dI, SMK_DISSOLVE_MIN));
		if (dSa < SMK_DISSOLVE_MIN)
			FatalError(Format("ReactionLabSmoke FAIL beat 3: sand dissolved %d < %d", dSa, SMK_DISSOLVE_MIN));
		if (dG != 0)
			FatalError(Format("ReactionLabSmoke FAIL beat 3: granite control corroded (%d)", dG));

		// ---- Beat 4: Duneworks ----
		var iDune = CountMatRegion(Material("Sand"), 900, 150, 999, 200);
		if (iDune < SMK_DUNE_SAND_MIN)
			FatalError(Format("ReactionLabSmoke FAIL beat 4: deposition zone holds %d < %d",
			                  iDune, SMK_DUNE_SAND_MIN));

		Log("ReactionLabSmoke PASS");
		GameOver();
		return -1;
	}
	return 1;
}

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
