/*-- CoastalSmoke.c4s -- headless content integration test. --*/
/* Uses the direct-call RunSmokeSteps() pattern (called from Initialize).   */
/* The AddEffect timer pattern does NOT fire reliably in the smoke harness  */
/* (spec edge case #12), so we follow the proven SeaTradeSmoke.c4s pattern. */
/* On any assertion failure, FatalError -> non-zero exit.                   */

#strict 2

protected func Initialize()
{
	RunSmokeSteps();
	return true;
}

func RunSmokeSteps()
{
	// ---- Step 0: scenario + pack loaded ----
	// The smoke Script.c references bare C4IDs DKST/LHGT/TIDE/FOGG/HMST
	// (all Coastal.c4d sub-defs) and SILK (TradeGoods.c4d). It only COMPILES
	// if those packs are in [Definitions] -- so merely reaching here proves
	// the pack loaded. The LandscapeWidth check confirms the map loaded.
	if (LandscapeWidth() <= 0)
		FatalError("CoastalSmoke FAIL step 0: landscape not loaded");

	// ---- Step 1: tide rises and falls ----
	LaunchWeatherEvent(TIDE, 50, 100);
	if (GetActiveWeatherEvent() != TIDE)
		FatalError("CoastalSmoke FAIL step 1: TIDE not active");
	if (!IsHighTide())
		FatalError("CoastalSmoke FAIL step 1: IsHighTide false on rising half");
	if (GetTideLevel() < 0)
		FatalError("CoastalSmoke FAIL step 1: GetTideLevel negative");
	StopWeatherEvent();
	if (GetActiveWeatherEvent() != 0)
		FatalError("CoastalSmoke FAIL step 1: TIDE not stopped");

	// ---- Step 2: dock + market-stall contract ----
	var pDock = CreateObject(DKST, 50, 30, NO_OWNER);
	if (!pDock) FatalError("CoastalSmoke FAIL step 2: DKST not spawned");
	RegisterTradeGood(SILK, pDock, 10);
	var aGoods = pDock->GetTradeGoods();
	if (!aGoods || GetLength(aGoods) < 1)
		FatalError("CoastalSmoke FAIL step 2: GetTradeGoods empty");
	if (ContentsCount(SILK, pDock) < 1)
		FatalError("CoastalSmoke FAIL step 2: SILK not stocked in dock");
	if (!pDock->CanMooreShip(0))
		FatalError("CoastalSmoke FAIL step 2: CanMooreShip false with no tide");

	// ---- Step 3: lighthouse beacon API ----
	var pLHGT = CreateObject(LHGT, 60, 30, NO_OWNER);
	if (!pLHGT) FatalError("CoastalSmoke FAIL step 3: LHGT not spawned");
	if (!pLHGT->IsLit())
		FatalError("CoastalSmoke FAIL step 3: IsLit false (no FuelSystem -> lit for free)");
	if (pLHGT->GetBeaconRange() <= 0)
		FatalError("CoastalSmoke FAIL step 3: GetBeaconRange <= 0");
	if (!pLHGT->InBeaconCone(GetX(pLHGT) + 50, GetY(pLHGT)))
		FatalError("CoastalSmoke FAIL step 3: InBeaconCone false within range");

	// ---- Step 4: BeaconLib query ----
	if (BeaconBonusAt(GetX(pLHGT), GetY(pLHGT)) <= 0)
		FatalError("CoastalSmoke FAIL step 4: BeaconBonusAt <= 0 with lit LHGT");
	RemoveObject(pLHGT);
	if (BeaconBonusAt(60, 30) > 0)
		FatalError("CoastalSmoke FAIL step 4: BeaconBonusAt > 0 with no lit LHGT");

	// ---- Step 5: fog event + lighthouse interaction ----
	LaunchWeatherEvent(FOGG, 50, 100);
	if (GetActiveWeatherEvent() != FOGG)
		FatalError("CoastalSmoke FAIL step 5: FOGG not active");
	pLHGT = CreateObject(LHGT, 60, 30, NO_OWNER);
	if (BeaconBonusAt(GetX(pLHGT), GetY(pLHGT)) <= 0)
		FatalError("CoastalSmoke FAIL step 5: lit LHGT does not cut through fog");
	StopWeatherEvent();
	if (GetActiveWeatherEvent() != 0)
		FatalError("CoastalSmoke FAIL step 5: FOGG not stopped");

	// ---- Step 6: HarborMaster throughput ----
	var pHMST = CreateObject(HMST, 10, 10, NO_OWNER);
	if (!pHMST) FatalError("CoastalSmoke FAIL step 6: HMST not spawned");
	var pDock2 = CreateObject(DKST, 70, 30, NO_OWNER);
	if (!pDock2) FatalError("CoastalSmoke FAIL step 6: second DKST not spawned");
	pDock2->RegisterShipment();
	// Drive the port-duty tally directly (smoke harness does not tick effect timers).
	pHMST->TickPortDuty();
	if (pHMST->GetPortThroughput() <= 0)
		FatalError("CoastalSmoke FAIL step 6: GetPortThroughput <= 0 after shipment");

	// ---- Step 7: pass + end ----
	Log("CoastalSmoke PASS");
	GameOver();
}
