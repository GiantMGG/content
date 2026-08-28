/*-- SeaTradeConvoySmoke.c4s -- headless Phase-2 smoke. --*/
/* Verifies the convoy + director + marker wiring.                    */
/* Uses the direct-call RunSmokeSteps() pattern (called from           */
/* Initialize). The AddEffect timer pattern does NOT fire in the      */
/* smoke harness -- known engine limitation. See AirshipSmoke.c4s     */
/* for the reference direct-call pattern.                              */

#strict 2

protected func Initialize() {
	RunSmokeSteps();
	return true;
}

func RunSmokeSteps() {
	// -- Setup: 2 island trade posts + 2 cargo ships --
	var pWest = CreateObject(MKTS, 20, 30, NO_OWNER);
	var pEast = CreateObject(MKTS, 80, 30, NO_OWNER);
	if (!pWest) FatalError("SeaTradeConvoySmoke FAIL step 0: MKTS west not spawned");
	if (!pEast) FatalError("SeaTradeConvoySmoke FAIL step 0: MKTS east not spawned");
	RegisterTradeGood(SILK, pWest, 10);
	RegisterTradeGood(SILK, pEast, 10);

	// 2 Cogs at west dock.
	var pCog1 = CreateObject(CGSH, 25, 20, NO_OWNER);
	var pCog2 = CreateObject(CGSH, 30, 20, NO_OWNER);
	if (!pCog1) FatalError("SeaTradeConvoySmoke FAIL step 0: CGSH #1 not spawned");
	if (!pCog2) FatalError("SeaTradeConvoySmoke FAIL step 0: CGSH #2 not spawned");

	// 3 SeaLaneMarkers across the lane.
	CreateObject(SLMR, 35, 25, NO_OWNER);
	CreateObject(SLMR, 50, 25, NO_OWNER);
	CreateObject(SLMR, 65, 25, NO_OWNER);

	// PirateDirector rule.
	CreateObject(PRDR, 50, 50, NO_OWNER);

	// Start 2 sea caravans (west -> east -> west ...).
	StartSeaCaravan(pCog1, pWest, pEast, [GetX(pWest), GetX(pEast)]);
	StartSeaCaravan(pCog2, pWest, pEast, [GetX(pWest), GetX(pEast)]);

	// Assertion 1: caravans active.
	if (!pCog1->IsSeaCaravanActive())
		FatalError("SeaTradeConvoySmoke FAIL step 0: caravan #1 not active");
	if (!pCog2->IsSeaCaravanActive())
		FatalError("SeaTradeConvoySmoke FAIL step 0: caravan #2 not active");

	// Assertion 2: SLMR markers exist.
	if (ObjectCount(SLMR) < 1)
		FatalError("SeaTradeConvoySmoke FAIL step 1: no SLMR markers");

	// Assertion 3: PRDR director exists.
	if (!FindObject(PRDR))
		FatalError("SeaTradeConvoySmoke FAIL step 2: PRDR director missing");

	// Assertion 4: STRM storm launches + IsStormActive.
	LaunchWeatherEvent(STRM, 50, 100);
	if (GetActiveWeatherEvent() != STRM)
		FatalError("SeaTradeConvoySmoke FAIL step 3: STRM not active");
	if (!IsStormActive())
		FatalError("SeaTradeConvoySmoke FAIL step 3: IsStormActive false");
	StopWeatherEvent();

	// Assertion 5: pass + end.
	Log("SeaTradeConvoySmoke PASS");
	GameOver();
}
