/*-- CascadeSmoke.c4s -- headless dam-break cascade smoke. --*/
/*                                                              */
/* Pins the cycle-86 cascade contract end-to-end: dig the       */
/* sandstone keystone under the lava lake, the lava drains      */
/* into the oil pan (FLAM), the burning mix tops the pan rim    */
/* and spills into the sea (Rock crust + Steam), the steam      */
/* condenses against the open sky and rains out right of the    */
/* sea tank.                                                    */
/*                                                              */
/* Landscape contract: the flat default map (Material=Earth,    */
/* no overrides) is 1000x400 px, ground top at y=200 (=G).      */
/*                                                              */
/* Driver note: the step timer is a GLOBAL effect with GLOBAL   */
/* callbacks (PXSBudgetSmoke pattern); per-object AddEffect     */
/* timers do not fire in the harness. Objects.c4d is loaded     */
/* so FLAM links cleanly.                                       */
/*                                                              */
/* Calibration: set g_fCalibrate=1 to log per-step rock/rain    */
/* deltas; freeze thresholds with >=3x margin, then flip        */
/* back to 0 (SteamBuoyancy discipline).                        */

#strict 2

static g_iStep;
static g_pDir;
static g_fCalibrate;
static g_iRockSea0, g_iWaterStrip0;

protected func Initialize()
{
	g_pDir = CreateObject(CDIR, 0, 0, NO_OWNER);
	g_pDir->SetGroundRow(200);
	g_pDir->SetLaunchStorm(false);
	g_pDir->SetSeedSea(true);
	g_pDir->PaintCascade(200);

	// Paint-baseline asserts: every apparatus ROI is filled.
	var n;
	n = CountMatRegion(Material("Lava"), 120, 70, 290, 100);
	if (n < 4500)
		FatalError(Format("CascadeSmoke FAIL: lava reservoir short (%d)", n));
	n = CountMatRegion(Material("Oil"), 120, 115, 440, 199);
	if (n < 25000)
		FatalError(Format("CascadeSmoke FAIL: oil pan short (%d)", n));
	n = CountMatRegion(Material("Water"), 470, 150, 880, 199);
	if (n < 18000)
		FatalError(Format("CascadeSmoke FAIL: sea short (%d)", n));
	n = CountMatRegion(Material("Sand"), 20, 150, 90, 199);
	if (n < 3000)
		FatalError(Format("CascadeSmoke FAIL: dune slab short (%d)", n));
	n = CountMatRegion(Material("Sandstone"), 110, 100, 300, 110);
	if (n < 1800)
		FatalError(Format("CascadeSmoke FAIL: keystone short (%d)", n));

	// Smoke-side baselines for the calibration logs.
	g_iRockSea0    = CountMatRegion(Material("Rock"),  460, 150, 890, 199);
	g_iWaterStrip0 = CountMatRegion(Material("Water"), 881, 190, 889, 199);

	g_iStep = 0;
	g_fCalibrate = 0;
	AddEffect("RunTest", 0, 1, 35, 0, 0);
	return true;
}

global func FxRunTestStart(target, effect, temp) { return 1; }

global func FxRunTestTimer(object target, int effect, int timer)
{
	++g_iStep;

	if (g_iStep == 1)
	{
		// Breach: open a slot through the keystone under the
		// lava lake, then kick instability so the lava starts
		// flowing via MassMover (approved dig fix, cycle 86).
		FreeRect(200, 100, 8, 10);                                        // open slot in place
		ExtractMaterialAmount(199, 100, Material("Sandstone"), 1);        // kick instability
	}
	else
	{
		// Per-step guards (steps >= 2).
		if (GetPXSCount() > 10000)
			FatalError(Format("CascadeSmoke FAIL step %d: PXS budget exceeded (%d)",
			                  g_iStep, GetPXSCount()));
		if (GetMaterialCount(Material("Steam")) != 0)
			FatalError(Format("CascadeSmoke FAIL step %d: static steam appeared (%d)",
			                  g_iStep, GetMaterialCount(Material("Steam"))));
		if (g_fCalibrate)
			Log(Format("[CAL] step %d rock %d rain %d pxs %d",
			           g_iStep,
			           CountMatRegion(Material("Rock"), 460, 150, 890, 199) - g_iRockSea0,
			           CountMatRegion(Material("Water"), 881, 190, 889, 199) - g_iWaterStrip0,
			           GetPXSCount()));
	}

	if (g_iStep >= 9)
	{
		var i_rock = CountMatRegion(Material("Rock"), 460, 150, 890, 199) - g_iRockSea0;
		var i_water = CountMatRegion(Material("Water"), 881, 190, 889, 199) - g_iWaterStrip0;
		if (!g_pDir->IsBreached())
			FatalError("CascadeSmoke FAIL: breach never detected");
		if (!g_pDir->HasFlam())
			FatalError("CascadeSmoke FAIL: oil never ignited (no FLAM beat)");
		if (!g_pDir->HasRock())
			FatalError(Format("CascadeSmoke FAIL: no rock crust in the sea band (rock delta %d, rain %d)",
			                  i_rock, i_water));
		if (!g_pDir->HasRain())
			FatalError(Format("CascadeSmoke FAIL: no rain deposits in the strip (rain %d, rock delta %d)",
			                  i_water, i_rock));
		Log(Format("[CAL] rock delta %d, rain %d", i_rock, i_water));
		Log("CascadeSmoke PASS");
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
