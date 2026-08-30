/*-- OilFireSmoke.c4s — headless oil-ignition default smoke. --*/
/*                                                              */
/* Pins U4 (cycle 75 spec §1): lava PXS (Incendiary) landing on */
/* static oil (Inflammable) trigger the shipped default          */
/* Incendiary x Inflammable -> Incinerate wiring                 */
/* (C4Material.cpp:331-333, mrfIncinerate ->                     */
/* Landscape.Incinerate -> FLAM object at the contact site).     */
/* No content change - additive pin only.                        */
/*                                                              */
/* Driver note: the per-step timer is a GLOBAL effect with       */
/* GLOBAL callbacks - the plain AddEffect(this) pattern does     */
/* not resolve callbacks in scenario scripts. Objects.c4d ships  */
/* FLAM and lets System.c4g link cleanly.                        */
/*                                                              */
/* On any assertion failure, FatalError produces a non-zero      */
/* exit code, failing the CTest entry.                           */

#strict 2

static g_iStep;
static g_fFlamFound;

protected func Initialize()
{
	// Basin: rock box x[100,300] y[60,160] with a static oil fill
	// (x[110,290] y[130,150] = 3600 oil pixels).
	DrawMaterialQuad("Rock", 100, 60, 110, 60, 110, 160, 100, 160);
	DrawMaterialQuad("Rock", 290, 60, 300, 60, 300, 160, 290, 160);
	DrawMaterialQuad("Rock", 100, 150, 300, 150, 300, 160, 100, 160);
	DrawMaterialQuad("Oil", 110, 130, 290, 130, 290, 150, 110, 150);

	g_iStep = 0;
	g_fFlamFound = false;
	AddEffect("RunTest", 0, 1, 35, 0, 0);
	return true;
}

global func FxRunTestStart(target, effect, temp) { return 1; }

global func FxRunTestTimer(object target, int effect, int timer)
{
	++g_iStep;

	if (g_iStep == 1)
	{
		// level=50: impact zone x in [150,250], inside the oil span.
		CastPXS("Lava", 100, 50, 200, 100);
	}
	else
	{
		// Catch FLAM at any checkpoint after the cast (FLAM objects
		// may burn out; the flag is sticky).
		if (FindObject(FLAM)) g_fFlamFound = true;
		if (GetPXSCount() > 10000)
			FatalError(Format("OilFireSmoke FAIL step %d: PXS budget exceeded (%d)",
			                  g_iStep, GetPXSCount()));
	}

	if (g_iStep >= 9)
	{
		if (!g_fFlamFound)
			FatalError("OilFireSmoke FAIL: lava PXS on oil never ignited (no FLAM found)");

		Log("OilFireSmoke PASS");
		GameOver();
		return -1;
	}
	return 1;
}
