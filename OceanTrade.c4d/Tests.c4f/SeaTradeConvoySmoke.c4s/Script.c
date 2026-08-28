/*-- SeaTradeConvoySmoke.c4s -- headless Phase-2 smoke. --*/
/* Verifies the convoy + director + marker wiring within the 350-tick   */
/* smoke window. Does NOT assert raid-wave scaling (2000-frame raid tick */
/* exceeds the window); that is manual-playtest-gated (spec).           */

#strict 2

static g_iStep;
static pCog1, pCog2;

protected func Initialize() {
	g_iStep = 0;
	// 2 island MTIP posts (west=producer SILK, east=consumer).
	var pWest = CreateObject(MKTS, 20, 30, NO_OWNER);
	var pEast = CreateObject(MKTS, 80, 30, NO_OWNER);
	RegisterTradeGood(SILK, pWest, 10);
	RegisterTradeGood(SILK, pEast, 10);
	// 2 Cogs at west dock.
	pCog1 = CreateObject(CGSH, 25, 20, NO_OWNER);
	pCog2 = CreateObject(CGSH, 30, 20, NO_OWNER);
	// 3 SeaLaneMarkers across the lane.
	CreateObject(SLMR, 35, 25, NO_OWNER);
	CreateObject(SLMR, 50, 25, NO_OWNER);
	CreateObject(SLMR, 65, 25, NO_OWNER);
	// PirateDirector rule.
	CreateObject(PRDR, 50, 50, NO_OWNER);
	// Start 2 sea caravans (west -> east -> west ...).
	StartSeaCaravan(pCog1, pWest, pEast, [GetX(pWest), GetX(pEast)]);
	StartSeaCaravan(pCog2, pWest, pEast, [GetX(pWest), GetX(pEast)]);
	AddEffect("RunTest", this, 1, 35, this);
	return true;
}

func FxRunTestStart(target, effect, temp) { return 1; }

func FxRunTestTimer(object target, int effect, int timer) {
	if (g_iStep == 0) {
		// Assertion 1: 2 Cogs spawned + caravans active.
		if (!pCog1) FatalError("SeaTradeConvoySmoke FAIL step 0: CGSH #1 not spawned");
		if (!pCog2) FatalError("SeaTradeConvoySmoke FAIL step 0: CGSH #2 not spawned");
		if (!pCog1->IsSeaCaravanActive())
			FatalError("SeaTradeConvoySmoke FAIL step 0: caravan #1 not active");
		if (!pCog2->IsSeaCaravanActive())
			FatalError("SeaTradeConvoySmoke FAIL step 0: caravan #2 not active");
	}
	else if (g_iStep == 1) {
		// Assertion 2: SLMR markers exist.
		if (ObjectCount(SLMR) < 1)
			FatalError("SeaTradeConvoySmoke FAIL step 1: no SLMR markers");
	}
	else if (g_iStep == 2) {
		// Assertion 3: PRDR director exists.
		if (!FindObject(PRDR))
			FatalError("SeaTradeConvoySmoke FAIL step 2: PRDR director missing");
	}
	else if (g_iStep == 3) {
		// Assertion 4: STRM storm launches + IsStormActive.
		LaunchWeatherEvent(STRM, 50, 100);
		if (GetActiveWeatherEvent() != STRM)
			FatalError("SeaTradeConvoySmoke FAIL step 3: STRM not active");
		if (!IsStormActive())
			FatalError("SeaTradeConvoySmoke FAIL step 3: IsStormActive false");
		StopWeatherEvent();
	}
	else if (g_iStep == 4) {
		// Assertion 5: pass + end.
		Log("SeaTradeConvoySmoke PASS");
		GameOver();
		return -1;
	}
	++g_iStep;
	return 1;
}
