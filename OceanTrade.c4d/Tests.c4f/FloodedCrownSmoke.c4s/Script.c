/*-- FloodedCrownSmoke.c4s -- headless flood-machine smoke (RED skeleton). --*/
/* Drives one Cog (CGSH) through fill -> settle -> founder -> sunk while     */
/* frame-stamping every phase transition. The Cog flood API                  */
/* (OpenBreach/PlugBreach/GetOpenBreaches/GetFloodLevel/GetFloodPhase) is    */
/* implemented in T2 (Cog.c4d/Script.c); until then every call path errors   */
/* and the smoke stays RED by design.                                        */
/* Driver form: target-0 global effect with GLOBAL callbacks (gotcha #4;     */
/* SaltChainSmoke probe2 variant). Effect-timer FatalError exits 0, so RED   */
/* rides the [error]/FAIL line in the log, never the exit code (gotcha #3).  */
/* [CAL] telemetry is tick-keyed sim state only (no GetTime) -- the          */
/* calibration readout T2 freezes constants against.                         */

#strict 2

// ---- Phase constants (initial values; T2 calibration freezes the set) ----
static const BREACH_FILL   = 1;   // flood units per open breach per frame
static const AWASH_AT      = 100; // settle-band start
static const SINK_TERMINAL = 4;   // wreck terminal velocity px/frame

static g_iStep;
static pCog;
static g_iY0;        // fire-1 GetY (afloat baseline)
static g_iFlood0;    // fire-1 flood level (== 0, asserted)
static g_iLastFlood; // previous fire's flood (monotonic guard)
static g_iLastY;     // previous fire's GetY (sink-rate guard)

/* First liquid row at column iX, scanning down from the map top; the Cog    */
/* spawns just below the surface of the all-water sea (Material=Water map).  */
global func WaterSurfaceY(int iX) {
	var iY = 0;
	while (iY < LandscapeHeight() - 1 && !GBackLiquid(iX, iY)) iY++;
	return iY;
}

protected func Initialize() {
	SetWind(0); // deterministic sea for the whole run
	g_iStep = 0;

	// Fixtures: one Cog (CGSH) mid-map near the surface, laden with 5 SILK
	// bales so the founder step has cargo to eject as floating salvage.
	var iX = LandscapeWidth() / 2;
	pCog = CreateObject(CGSH, iX, WaterSurfaceY(iX) + 10, NO_OWNER);
	if (!pCog)
		FatalError("FloodedCrownSmoke FAIL step 0: CGSH not spawned");
	var i;
	for (i = 0; i < 5; ++i)
		CreateContents(SILK, pCog);
	if (ContentsCount(SILK, pCog) != 5)
		FatalError("FloodedCrownSmoke FAIL step 0: SILK hold not loaded (5)");

	AddEffect("RunTest", 0, 1, 35, 0, 0);
	return true;
}

global func FxRunTestStart(object target, int effect, int temp) { return 1; }

global func FxRunTestTimer(object target, int effect, int timer) {
	++g_iStep;

	// [CAL] per-fire telemetry (tick-keyed sim state only, no GetTime).
	// After the founder swap the Cog is a CWCK wreck whose script owns no
	// flood API; the readout then prints flood/phase as -1 ("state died
	// with the Cog def") and keeps the y/ydir sink readout.
	if (g_iStep <= 8) {
		var iCalFlood = 0;
		var iCalPhase = 0;
		if (GetID(pCog) == CWCK) {
			iCalFlood = -1;
			iCalPhase = -1;
		} else {
			iCalFlood = pCog->GetFloodLevel();
			iCalPhase = pCog->GetFloodPhase();
		}
		Log(Format("[CAL] fire %d: flood %d y %d ydir %d phase %d", g_iStep,
			iCalFlood, GetY(pCog), GetYDir(pCog), iCalPhase));
	}

	// ---- Fire 1 (frame ~35, step 0): afloat + dry, open two breaches ----
	if (g_iStep == 1) {
		if (!InLiquid(pCog))
			FatalError("FloodedCrownSmoke FAIL step 0: Cog not afloat");
		if (pCog->GetFloodLevel() != 0)
			FatalError("FloodedCrownSmoke FAIL step 0: flood not 0");
		if (pCog->GetFloodPhase() != 0)
			FatalError("FloodedCrownSmoke FAIL step 0: phase not DRY");
		pCog->OpenBreach();
		pCog->OpenBreach();
		g_iY0 = GetY(pCog);
		g_iFlood0 = pCog->GetFloodLevel();
		g_iLastY = g_iY0;
		g_iLastFlood = g_iFlood0;
	}

	// ---- Fire 2 (frame ~70, step 1): fill rate 2 breaches x 35 frames ----
	if (g_iStep == 2) {
		var iFlood = pCog->GetFloodLevel();
		if (iFlood - g_iFlood0 != 2 * BREACH_FILL * 35)
			FatalError(Format("FloodedCrownSmoke FAIL step 1: flood delta %d != %d",
				iFlood - g_iFlood0, 2 * BREACH_FILL * 35));
		if (iFlood <= g_iLastFlood)
			FatalError(Format("FloodedCrownSmoke FAIL step 1: flood not monotonic (%d <= %d)",
				iFlood, g_iLastFlood));
		g_iLastFlood = iFlood;
	}

	// ---- Fire 3 (frame ~105, step 2): boatswain plug, inflow halved ----
	if (g_iStep == 3) {
		pCog->PlugBreach();
		if (pCog->GetOpenBreaches() != 1)
			FatalError(Format("FloodedCrownSmoke FAIL step 2: open breaches %d != 1",
				pCog->GetOpenBreaches()));
		g_iLastFlood = pCog->GetFloodLevel(); // per-step delta base for fire 4
	}

	// ---- Fire 4 (frame ~140, step 3): halved delta, AWASH band, settle ----
	if (g_iStep == 4) {
		var iFlood = pCog->GetFloodLevel();
		if (iFlood - g_iLastFlood != 1 * BREACH_FILL * 35)
			FatalError(Format("FloodedCrownSmoke FAIL step 3: flood delta %d != %d",
				iFlood - g_iLastFlood, 1 * BREACH_FILL * 35));
		if (iFlood < AWASH_AT)
			FatalError(Format("FloodedCrownSmoke FAIL step 3: flood %d < AWASH_AT %d",
				iFlood, AWASH_AT));
		if (GetY(pCog) <= g_iY0)
			FatalError(Format("FloodedCrownSmoke FAIL step 3: Cog y %d not deeper than y0 %d (settle ballast)",
				GetY(pCog), g_iY0));
		g_iLastFlood = iFlood;
		g_iLastY = GetY(pCog);
	}

	// ---- Fire 5 (frame ~175, step 4): founder fired, salvage ejected ----
	if (g_iStep == 5) {
		if (GetID(pCog) != CWCK)
			FatalError(Format("FloodedCrownSmoke FAIL step 4: founder not fired (id %d)",
				GetID(pCog)));
		if (!FindObject2(Find_ID(SILK), Find_NoContainer(),
				Find_Distance(150, GetX(pCog), GetY(pCog))))
			FatalError("FloodedCrownSmoke FAIL step 4: no SILK salvage ejected");
		g_iLastY = GetY(pCog);
	}

	// ---- Fires 6+7 (frames ~210/245, steps 5/6): wreck sink invariants ----
	if (g_iStep == 6 || g_iStep == 7) {
		if (GetY(pCog) <= g_iLastY)
			FatalError(Format("FloodedCrownSmoke FAIL step %d: wreck y %d not increasing over %d",
				g_iStep - 1, GetY(pCog), g_iLastY));
		if (Abs(GetYDir(pCog) - SINK_TERMINAL) > 1)
			FatalError(Format("FloodedCrownSmoke FAIL step %d: ydir %d off terminal %d",
				g_iStep - 1, GetYDir(pCog), SINK_TERMINAL));
		g_iLastY = GetY(pCog);
	}

	// ---- Fire 8 (frame ~280, step 7): wreck below depth, pass ----
	if (g_iStep == 8) {
		if (GetY(pCog) <= LandscapeHeight() / 2)
			FatalError(Format("FloodedCrownSmoke FAIL step 7: wreck y %d not below half height %d",
				GetY(pCog), LandscapeHeight() / 2));
		Log("FloodedCrownSmoke PASS");
		GameOver();
	}
	return 1;
}
