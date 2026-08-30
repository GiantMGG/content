/*-- LavaWaterSmoke.c4s — headless lava/water reaction smoke. --*/
/*                                                              */
/* Pins U1 (cycle 75 spec §1) both directions:                  */
/*   A: water PXS onto static lava -> Rock crust + rising Steam */
/*   B: lava PXS onto static water -> Rock crust + rising Steam */
/* plus the rain-loop tail: condensed steam re-deposits static  */
/* water above the crusted lava surface line.                   */
/*                                                              */
/* Landscape contract: default flat map (Material=Earth, no     */
/* Amplitude/Random overrides) has its ground top at y=200;     */
/* both basins are drawn in the open-sky band y in [60,160].    */
/*                                                              */
/* Driver note: the per-step timer is a GLOBAL effect with      */
/* GLOBAL callbacks (AddEffect(..., 0, ...) + global func       */
/* Fx...Timer) - the plain AddEffect("...", this, ..., this)    */
/* pattern does not resolve callbacks in scenario scripts       */
/* (see MinorMelee.c4s for the shipped global-func precedent).  */
/* Objects.c4d is loaded so System.c4g's LaunchRain/            */
/* LaunchVolcano/ObjectInsertMaterial link cleanly.             */
/*                                                              */
/* On any assertion failure, FatalError produces a non-zero     */
/* exit code, failing the CTest entry.                          */

#strict 2

static g_iStep;
static g_iRockA0, g_iLavaA0, g_iRockB0;

protected func Initialize()
{
	// Basin A: rock box x[100,300] y[60,160] with a static lava fill
	// (x[110,290] y[130,150] = 3600 lava pixels).
	DrawMaterialQuad("Rock", 100, 60, 110, 60, 110, 160, 100, 160);
	DrawMaterialQuad("Rock", 290, 60, 300, 60, 300, 160, 290, 160);
	DrawMaterialQuad("Rock", 100, 150, 300, 150, 300, 160, 100, 160);
	DrawMaterialQuad("Lava", 110, 130, 290, 130, 290, 150, 110, 150);
	// Basin B: same box at x+400 with a static water fill.
	DrawMaterialQuad("Rock", 500, 60, 510, 60, 510, 160, 500, 160);
	DrawMaterialQuad("Rock", 690, 60, 700, 60, 700, 160, 690, 160);
	DrawMaterialQuad("Rock", 500, 150, 700, 150, 700, 160, 500, 160);
	DrawMaterialQuad("Water", 510, 130, 690, 130, 690, 150, 510, 150);

	g_iRockA0 = CountMatRegion(Material("Rock"), 100, 60, 300, 160);
	g_iLavaA0 = CountMatRegion(Material("Lava"), 100, 60, 300, 160);
	g_iRockB0 = CountMatRegion(Material("Rock"), 500, 60, 700, 160);

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
		// Direction A: water PXS onto basin A's static lava.
		// level=20 keeps the impact zone at x in [175,225], inside the
		// lava span x[110,290] - every PXS reaches lava at Rate=100.
		CastPXS("Water", 400, 20, 200, 100);
		// Direction B: lava PXS onto basin B's static water.
		CastPXS("Lava", 400, 20, 600, 100);
	}
	else
	{
		// Steam is PXS-only: it must never insert statically.
		if (GetMaterialCount(Material("Steam")) != 0)
			FatalError(Format("LavaWaterSmoke FAIL step %d: static steam appeared (%d)",
			                  g_iStep, GetMaterialCount(Material("Steam"))));
		// PXS budget guard (10k hard cap).
		if (GetPXSCount() > 10000)
			FatalError(Format("LavaWaterSmoke FAIL step %d: PXS budget exceeded (%d)",
			                  g_iStep, GetPXSCount()));
	}

	if (g_iStep == 2)
	{
		// Mid-run liveness: the reaction web keeps PXS in flight
		// (airborne water/steam/lava cycling through conversions).
		if (GetPXSCount() <= 0)
			FatalError("LavaWaterSmoke FAIL step 2: no PXS in flight after the casts");
	}

	if (g_iStep >= 9)
	{
		// Direction A: the lava surface quenched to rock; lava strictly
		// consumed. Expected rock delta ~400 (threshold 40 = 10x margin).
		var iRockA = CountMatRegion(Material("Rock"), 100, 60, 300, 160);
		var iLavaA = CountMatRegion(Material("Lava"), 100, 60, 300, 160);
		if (iRockA - g_iRockA0 < 40)
			FatalError(Format("LavaWaterSmoke FAIL: basin A rock delta too small (%d)",
			                  iRockA - g_iRockA0));
		if (iLavaA >= g_iLavaA0)
			FatalError(Format("LavaWaterSmoke FAIL: basin A lava not consumed (%d -> %d)",
			                  g_iLavaA0, iLavaA));

		// Direction B: lava PXS quenched basin B's static water to rock.
		var iRockB = CountMatRegion(Material("Rock"), 500, 60, 700, 160);
		if (iRockB - g_iRockB0 < 40)
			FatalError(Format("LavaWaterSmoke FAIL: basin B rock delta too small (%d)",
			                  iRockB - g_iRockB0));

		// Rain-loop tail: static water re-deposited above the former
		// lava line (y < 130) in basin A's region - on the crust top,
		// rim tops, or inner wall faces. Expected ~100+ (threshold 10).
		var iRainA = CountMatRegion(Material("Water"), 100, 50, 300, 129);
		if (iRainA < 10)
			FatalError(Format("LavaWaterSmoke FAIL: no rain deposits above basin A lava line (%d)",
			                  iRainA));

		Log("LavaWaterSmoke PASS");
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
