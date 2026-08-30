/*-- SteamBuoyancySmoke.c4s — headless steam buoyancy smoke. --*/
/*                                                             */
/* Pins U2 (cycle 75 spec §1): Steam PXS (Buoyancy=150,        */
/* Density=1) rise out of an open-top rock chimney, condense   */
/* against the open sky (TargetSpec=Sky, Rate=5) into falling  */
/* Water PXS, and insert as static water on the open ground    */
/* OUTSIDE the chimney. Without the buoyant acceleration the   */
/* steam falls onto the chimney floor and the outside deposit  */
/* stays empty - this is the mutation hatch negative-verified  */
/* in the plan's verification phase.                           */
/*                                                             */
/* Geometry note: the chimney interior is 60px wide (walls at  */
/* x[160,170] and x[230,240], top at y=140) and the steam is   */
/* cast 10px below the top, so the rising steam escapes        */
/* before the 5%/check sky condensation or the 20%/check wall  */
/* condensation can absorb it. Measured escape quota: ~65 of   */
/* 300 cast PXS rain onto the ground outside the footprint     */
/* (threshold 10 = 6.5x margin); without buoyancy the outside  */
/* count collapses to ~0.                                      */
/*                                                             */
/* Driver note: the per-step timer is a GLOBAL effect with     */
/* GLOBAL callbacks (AddEffect(..., 0, ...) + global func      */
/* Fx...Timer) - the plain AddEffect("...", this, ..., this)   */
/* pattern does not resolve callbacks in scenario scripts.     */
/* Objects.c4d is loaded so System.c4g's LaunchRain/           */
/* LaunchVolcano/ObjectInsertMaterial link cleanly.            */
/*                                                             */
/* On any assertion failure, FatalError produces a non-zero    */
/* exit code, failing the CTest entry.                         */

#strict 2

static g_iStep;
static g_iPxsBase;

protected func Initialize()
{
	// Open-top rock chimney standing on the flat ground (top at y=200):
	// walls x[160,170] and x[230,240], y[140,200]; interior x[170,230]
	// with the Earth ground as its floor. The cast point (200,150) is
	// inside, 10px below the open top; only buoyant steam can leave.
	DrawMaterialQuad("Rock", 160, 140, 170, 140, 170, 200, 160, 200);
	DrawMaterialQuad("Rock", 230, 140, 240, 140, 240, 200, 230, 200);

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
		g_iPxsBase = GetPXSCount();
		CastPXS("Steam", 300, 20, 200, 150);
	}
	else
	{
		// Steam is PXS-only: it must never insert statically.
		if (GetMaterialCount(Material("Steam")) != 0)
			FatalError(Format("SteamBuoyancySmoke FAIL step %d: static steam appeared (%d)",
			                  g_iStep, GetMaterialCount(Material("Steam"))));
		if (GetPXSCount() > 10000)
			FatalError(Format("SteamBuoyancySmoke FAIL step %d: PXS budget exceeded (%d)",
			                  g_iStep, GetPXSCount()));
	}

	if (g_iStep >= 9)
	{
		// Rain deposits: static water on the open ground inside the
		// scan strip x[100,300] y[190,199], EXCLUDING the chimney
		// footprint x[160,240] (only steam that rose out of the open
		// top can rain outside the chimney). Expected ~65 of 300
		// cast steam resolving as outside deposits; threshold 10.
		var iRain = 0;
		var x, y;
		for (x = 100; x <= 300; x++)
			if (x < 160 || x > 240)
				for (y = 190; y <= 199; y++)
					if (GetMaterial(x, y) == Material("Water"))
						iRain++;
		if (iRain < 10)
			FatalError(Format("SteamBuoyancySmoke FAIL: no rain deposits outside the chimney (%d)",
			                  iRain));

		// All cast PXS must have resolved: condensed water inserted
		// statically or left the map top (y < -10 deactivates; no
		// leak). P(airborne at tick 315) ~ 0.95^280 * 300 ~ 2e-4.
		if (GetPXSCount() != g_iPxsBase)
			FatalError(Format("SteamBuoyancySmoke FAIL: PXS count did not return to baseline (%d != %d)",
			                  GetPXSCount(), g_iPxsBase));

		Log("SteamBuoyancySmoke PASS");
		GameOver();
		return -1;
	}
	return 1;
}
