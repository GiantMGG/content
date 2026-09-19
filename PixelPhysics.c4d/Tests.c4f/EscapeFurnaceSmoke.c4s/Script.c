/*-- EscapeFurnaceSmoke.c4s — headless furnace smoke. --*/
/*                                                       */
/* Cycle-147 RC: the temp-scan ice melt (4 px per column */
/* per sweep, t3-furnace-smoke.txt: ice 880->836) could  */
/* never consume a fat doorway plug in play time, so the */
/* mechanism was swapped to the Snow<->lava Convert      */
/* chain (Snow.c4m [Reaction] Type=Convert               */
/* TargetSpec=Incindiary, proven headless by             */
/* SnowMeltSmoke). The probe findings (rc-probe-*.txt):  */
/*   - static Snow directly on lava self-starts the melt */
/*     but pre-lever contact is unplayable (melts during */
/*     the intro);                                       */
/*   - a lid gap of ANY height is a stall: static snow   */
/*     never falls through air, and an exposed lava pool */
/*     bleeds away on its own (movers/ashes, probes      */
/*     B/D/E);                                           */
/*   - the trigger "FreeRect the lid + BlastFree the     */
/*     plug base" (probe F) converts the plug base to    */
/*     mass movers, the column avalanches into the pool, */
/*     and the Convert chain consumes it: snowDoor       */
/*     750 -> ~15, lava 380 -> 0 in ~2 steps. Probe G    */
/*     (trigger at step 4) shows the door still clears   */
/*     by avalanche collapse even with a dead pool.      */
/* The smoke scripts the room's lever action (throw      */
/* while the furnace is alive) and asserts the doorway   */
/* clears + the melt byproduct (quench rock crust) is    */
/* observable.                                           */

#strict 2

static const PXS_BUDGET      = 10000; // hard-cap tripwire (SnowMeltSmoke:62-64)
static const SNOW_X1         = 185;   // snow plug column span (the doorway)
static const SNOW_X2         = 194;
static const SNOW_Y1         = 62;    // snow plug vertical span (corridor height)
static const SNOW_Y2         = 137;
static const FURNACE_REMNANT_MAX = 40; // CAL-FREEZE: doorway-band snow end-state
                                       // measured at 6..13 across seeds (1472: 13,
                                       // 1: 7, 2: 6) -> 3.1x margin at 40.
static const ROCK_DELTA_MIN      = 8;  // CAL-FREEZE: trough-rock crust delta measured
                                       // at 18..29 across seeds (1472: 27, 1: 26,
                                       // 2: 18) -> 2.25x margin at 8. The furnace
                                       // charge is additionally asserted fully spent
                                       // (lava == 0, deterministic in every run).

static g_iStep;
static g_iSnow0;
static g_iRockTrough0;

protected func Initialize()
{
	// Cold chamber pin (room-02 pattern): the snow plug must remain
	// stable while temp <= 5 (Snow.c4m AboveTempConvert=5). The lava
	// charge is sealed under the lid by the room design.
	SetTemperature(0);
	SetClimate(-50);

	// Corridor: rock shell x[100,300] y[60,180] on the default
	// 500x200 earth map, interior carved to air.
	DrawMaterialQuad("Rock", 100, 60, 105, 60, 105, 180, 100, 180);   // left wall
	DrawMaterialQuad("Rock", 295, 60, 300, 60, 300, 180, 295, 180);   // right wall
	DrawMaterialQuad("Rock", 100, 60, 300, 60, 300, 62, 100, 62);     // ceiling
	DrawMaterialQuad("Rock", 100, 138, 300, 138, 300, 180, 100, 180); // floor band (hosts the trough)
	FreeRect(105, 62, 190, 76);                                        // carve interior to air

	// The doorway plug: 10 wide x 76 tall snow column (the room's plug
	// is the same shape at EscapeRoom02's exit, Ice->Snow swap).
	DrawMaterialQuad("Snow", SNOW_X1, SNOW_Y1, SNOW_X2, SNOW_Y1,
	                 SNOW_X2, SNOW_Y2, SNOW_X1, SNOW_Y2);
	// The lid: 4 px rock separating snow from the furnace charge.
	DrawMaterialQuad("Rock", 185, 138, 194, 138, 194, 141, 185, 141);
	// The furnace charge: a static lava pool in the rock trough beneath
	// the lid (10 x 39 px). Sealed: the smoke spends ~2 steps pre-lever.
	DrawMaterialQuad("Lava", 185, 142, 194, 142, 194, 180, 185, 180);

	// Paint-baseline asserts (CascadeSmoke:39-53 pattern).
	g_iSnow0 = CountMatRegion(Material("Snow"), 180, 60, 200, 137);
	if (g_iSnow0 < 700)
		FatalError(Format("EscapeFurnaceSmoke FAIL: snow plug paint short (%d, expect ~750)", g_iSnow0));
	if (CountMatRegion(Material("Lava"), 180, 138, 200, 185) < 300)
		FatalError(Format("EscapeFurnaceSmoke FAIL: furnace charge paint short (%d, expect ~380)",
		                  CountMatRegion(Material("Lava"), 180, 138, 200, 185)));
	// Trough-band rock baseline: the crust delta is measured over the
	// trough band only, so the lid-Free (which removes 40 rock cells
	// from the corridor ROI) never pollutes the crust measurement; the
	// melt converts the pool's lava -> rock crust in this band.
	g_iRockTrough0 = CountMatRegion(Material("Rock"), 180, 142, 200, 185);
	Log(Format("[CAL] snow0 %d rockTrough0 %d", g_iSnow0, g_iRockTrough0));

	g_iStep = 0;
	AddEffect("RunTest", 0, 1, 35, 0, 0);
	return true;
}

global func FxRunTestStart(target, effect, temp) { return 1; }

global func FxRunTestTimer(object target, int effect, int timer)
{
	++g_iStep;

	// Cold re-pin (room-02 director pattern, replicated headless).
	SetTemperature(0);

	if (g_iStep == 2)
	{
		// The room's lever action (step 2: one step after the baselines
		// so the sealed charge is provably intact at the throw):
		//   1) FreeRect the lid -> the furnace mouth opens;
		//   2) BlastFree the plug base -> CheckInstabilityRange turns the
		//      instable snow base into mass movers; the column avalanches
		//      into the pool and the Snow<->lava Convert chain melts it
		//      bottom-up (probe F).
		FreeRect(185, 138, 10, 4);
		BlastFree(189, 140, 6);
	}

	if (g_iStep >= 3 && GetPXSCount() > PXS_BUDGET)
		FatalError(Format("EscapeFurnaceSmoke FAIL step %d: PXS budget exceeded (%d)",
		                  g_iStep, GetPXSCount()));

	// Doorway band (the plug's original location above the trough line)
	// vs. the pit (leftover debris below the doorway line).
	var iSnowDoor = CountMatRegion(Material("Snow"), 180, 60, 200, 137);
	var iSnowPit  = CountMatRegion(Material("Snow"), 180, 138, 200, 185);
	var iLava     = CountMatRegion(Material("Lava"),  180, 138, 200, 185);
	var iRockTrough = CountMatRegion(Material("Rock"), 180, 142, 200, 185);
	Log(Format("[CAL] step %d: snowDoor %d snowPit %d lava %d", g_iStep, iSnowDoor, iSnowPit, iLava));
	Log(Format("[CAL] step %d: rockTrough %d troughDelta %d pxs %d",
	           g_iStep, iRockTrough, iRockTrough - g_iRockTrough0, GetPXSCount()));

	if (g_iStep >= 9)
	{
		// Log the final readout BEFORE asserting so a failing calibration
		// run still prints the numbers.
		Log(Format("[CAL] final: snowDoor %d snowPit %d lava %d troughDelta %d pxs %d",
		           iSnowDoor, iSnowPit, iLava, iRockTrough - g_iRockTrough0, GetPXSCount()));
		// The doorway must have cleared (avalanche + melt).
		if (iSnowDoor > FURNACE_REMNANT_MAX)
			FatalError(Format("EscapeFurnaceSmoke FAIL: doorway not cleared (snowDoor %d > %d)",
			                  iSnowDoor, FURNACE_REMNANT_MAX));
		// The furnace charge must be spent (quenched to crust / dispersed).
		if (iLava > 0)
			FatalError(Format("EscapeFurnaceSmoke FAIL: furnace charge not consumed (lava %d)",
			                  iLava));
		// Melt/quench byproduct: the pool's lava must have been converted
		// to rock crust in the trough band (measured 18..29 across seeds).
		if (iRockTrough - g_iRockTrough0 < ROCK_DELTA_MIN)
			FatalError(Format("EscapeFurnaceSmoke FAIL: no quench crust (troughDelta %d < %d)",
			                  iRockTrough - g_iRockTrough0, ROCK_DELTA_MIN));

		Log("EscapeFurnaceSmoke PASS");
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
