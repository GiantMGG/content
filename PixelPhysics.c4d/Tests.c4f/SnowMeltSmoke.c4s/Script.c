/*-- SnowMeltSmoke.c4s — headless snow-melt chain smoke. --*/
/*                                                          */
/* Pins U3 (cycle 75 spec §1): snow PXS melt on the static  */
/* lava via the SHIPPED [Reaction] Type=Convert              */
/* TargetSpec=Incindiary (Snow.c4m:30-33), and the resulting */
/* water PXS quench the lava via the NEW React web (rock     */
/* crust + steam). The composed chain is:                    */
/*   snow-PXS ->(Convert)  water-PXS                         */
/*             ->(React)   static Rock + rising Steam PXS    */
/*                                                          */
/* NOTE (spec deviation, documented in the plan): the spec's */
/* literal "no static Snow ever" invariant is unsatisfiable  */
/* here - once the impact zone crusts to Rock, later snow    */
/* arrivals legitimately insert as static snow on the crust  */
/* (mrfInsert default path). The U3 pin is the regional      */
/* rock-delta assertion below, which reads ~0 if either      */
/* chain link breaks.                                        */
/*                                                          */
/* Driver note: the per-step timer is a GLOBAL effect with   */
/* GLOBAL callbacks - the plain AddEffect(this) pattern does */
/* not resolve callbacks in scenario scripts. Objects.c4d is */
/* loaded so System.c4g links cleanly.                       */
/*                                                          */
/* On any assertion failure, FatalError produces a non-zero  */
/* exit code, failing the CTest entry.                       */

#strict 2

static g_iStep;
static g_iRock0;

protected func Initialize()
{
	// Basin: rock box x[100,300] y[60,160] with a static lava fill
	// (x[110,290] y[130,150] = 3600 lava pixels).
	DrawMaterialQuad("Rock", 100, 60, 110, 60, 110, 160, 100, 160);
	DrawMaterialQuad("Rock", 290, 60, 300, 60, 300, 160, 290, 160);
	DrawMaterialQuad("Rock", 100, 150, 300, 150, 300, 160, 100, 160);
	DrawMaterialQuad("Lava", 110, 130, 290, 130, 290, 150, 110, 150);

	g_iRock0 = CountMatRegion(Material("Rock"), 100, 60, 300, 160);

	g_iStep = 0;
	AddEffect("RunTest", 0, 1, 35, 0, 0);
	return true;
}

global func FxRunTestStart(target, effect, temp) { return 1; }

global func FxRunTestTimer(object target, int effect, int timer)
{
	++g_iStep;

	if (g_iStep == 1)
	{
		// level=20 keeps the impact zone at x in [165,235], inside the
		// lava span x[110,290]: all 300 snow PXS contact lava first.
		CastPXS("Snow", 300, 20, 200, 115);
	}
	else
	{
		if (GetPXSCount() > 10000)
			FatalError(Format("SnowMeltSmoke FAIL step %d: PXS budget exceeded (%d)",
			                  g_iStep, GetPXSCount()));
	}

	if (g_iStep == 2)
	{
		// Mid-run liveness: the melt chain keeps PXS in flight
		// (airborne snow/water/steam cycling through conversions).
		if (GetPXSCount() <= 0)
			FatalError("SnowMeltSmoke FAIL step 2: no PXS in flight after the snow cast");
	}

	if (g_iStep >= 9)
	{
		// The composed chain quenched lava surface pixels to rock.
		// Expected delta ~50 (first-per-column conversions) plus the
		// steam->rain->lava spread over 280 ticks; threshold 25 has
		// >= 2x margin over the guaranteed minimum alone.
		var iRock = CountMatRegion(Material("Rock"), 100, 60, 300, 160);
		if (iRock - g_iRock0 < 25)
			FatalError(Format("SnowMeltSmoke FAIL: rock delta too small (%d)",
			                  iRock - g_iRock0));

		Log("SnowMeltSmoke PASS");
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
