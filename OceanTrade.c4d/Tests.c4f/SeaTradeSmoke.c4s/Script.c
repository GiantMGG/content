/*-- SeaTradeSmoke.c4s -- headless Phase-1 smoke. --*/
/* Exercises: CGSH container, trade-good loading, STRM launch +     */
/* IsStormActive, PSLP spawn + IsPirateShip, STLD delivery counter. */
/* On assertion failure, FatalError -> non-zero exit.               */
/* Uses the direct-call RunSmokeSteps() pattern (called from        */
/* Initialize). The AddEffect timer pattern does NOT fire in the    */
/* smoke harness -- known engine limitation. See AirshipSmoke.c4s   */
/* for the reference direct-call pattern.                           */
/*                                                                  */
/* MaxPlayer=0: no player is available, so BuyGoodAt/SellGoodAt     */
/* (which need GetAnyPlayer() for wealth accounting) are not        */
/* exercised here. Instead, goods are loaded via CreateContents     */
/* and the container is verified via FindContents/ContentsCount.    */

#strict 2

protected func Initialize() {
	RunSmokeSteps();
	return true;
}

func RunSmokeSteps() {
	// -- Setup: west/east trade posts + cargo ship --
	var pWestPost = CreateObject(MKTS, 20, 30, NO_OWNER);
	var pEastPost = CreateObject(MKTS, 80, 30, NO_OWNER);
	if (!pWestPost) FatalError("SeaTradeSmoke FAIL step 0: MKTS west not spawned");
	if (!pEastPost) FatalError("SeaTradeSmoke FAIL step 0: MKTS east not spawned");
	RegisterTradeGood(SILK, pWestPost, 10);
	RegisterTradeGood(SILK, pEastPost, 10);

	// Assertion 1: CGSH spawned + is a container.
	var pCog = CreateObject(CGSH, 25, 20, NO_OWNER);
	if (!pCog) FatalError("SeaTradeSmoke FAIL step 0: CGSH not spawned");
	if (!pCog->IsCargoShip())
		FatalError("SeaTradeSmoke FAIL step 0: IsCargoShip false");

	// Assertion 2: trade goods can be loaded into the cog's hold.
	var pSilk = CreateContents(SILK, pCog);
	if (!pSilk) FatalError("SeaTradeSmoke FAIL step 1: could not CreateContents(SILK)");
	if (ContentsCount(SILK, pCog) < 1)
		FatalError("SeaTradeSmoke FAIL step 1: SILK not in cog hold");
	if (!pSilk->IsTradeGood())
		FatalError("SeaTradeSmoke FAIL step 1: SILK IsTradeGood false");

	// Assertion 3: STRM launches + IsStormActive.
	LaunchWeatherEvent(STRM, 50, 100);
	if (GetActiveWeatherEvent() != STRM)
		FatalError("SeaTradeSmoke FAIL step 2: STRM not active");
	if (!IsStormActive())
		FatalError("SeaTradeSmoke FAIL step 2: IsStormActive false");

	// Assertion 4: stop storm, assert cleared.
	StopWeatherEvent();
	if (GetActiveWeatherEvent() != 0)
		FatalError("SeaTradeSmoke FAIL step 3: storm not stopped");
	if (IsStormActive())
		FatalError("SeaTradeSmoke FAIL step 3: IsStormActive true after stop");

	// Assertion 5: PSLP spawned + IsPirateShip.
	var pPirate = CreateObject(PSLP, 50, 20, NO_OWNER);
	if (!pPirate) FatalError("SeaTradeSmoke FAIL step 3: PSLP not spawned");
	if (!pPirate->IsPirateShip())
		FatalError("SeaTradeSmoke FAIL step 3: IsPirateShip false");

	// Assertion 6: STLD (StormLaneDirector) spawns + RegisterDelivery.
	var pDir = CreateObject(STLD, 50, 50, NO_OWNER);
	if (!pDir) FatalError("SeaTradeSmoke FAIL step 4: STLD not spawned");
	pDir->RegisterDelivery();
	pDir->RegisterDelivery();
	pDir->RegisterDelivery();
	// After 3 deliveries, STLD should have fired GameOver (win).
	// We can't assert GameOver here, but the smoke will end.

	// Assertion 7: pass + end.
	Log("SeaTradeSmoke PASS");
	GameOver();
}
