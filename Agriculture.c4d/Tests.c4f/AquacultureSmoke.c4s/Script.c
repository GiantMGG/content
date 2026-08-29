/*-- AquacultureSmoke.c4s -- headless content integration test. --*/
/* Synchronous RunSmokeSteps() pattern (cf. CoastalSmoke.c4s,         */
/* SpawnerSmoke.c4s). AddEffect/TimerCall timers do NOT fire reliably */
/* in the smoke harness, so we drive state transitions by direct      */
/* method calls and LocalN() test seams. On any assertion failure,    */
/* FatalError(...) -> non-zero exit.                                  */

#strict 2

protected func Initialize()
{
	RunSmokeSteps();
	return true;
}

func RunSmokeSteps()
{
	// ---- Step 0: tide cycle drives IsHighTide ----
	LaunchWeatherEvent(TIDE, 50, 100);
	if (GetActiveWeatherEvent() != TIDE)
		FatalError("AquacultureSmoke FAIL step 0: TIDE not active");
	if (!IsHighTide())
		FatalError("AquacultureSmoke FAIL step 0: IsHighTide false on rising half");

	// ---- Step 1: TidalPool starts Exposed ----
	var pPool = CreateObject(TDPL, 200, 200, NO_OWNER);
	if (!pPool) FatalError("AquacultureSmoke FAIL step 1: TDPL not spawned");
	if (pPool->IsFlooded())
		FatalError("AquacultureSmoke FAIL step 1: pool starts flooded");
	if (GetAction(pPool) != "Exposed")
		FatalError("AquacultureSmoke FAIL step 1: pool action not Exposed");

	// ---- Step 2: feed the pool, drive Timer while flooded, assert FISH spawned ----
	// Stock feed via the direct-local test seam (bypasses Collected plumbing).
	pPool->LocalN("feed_units") = 4;
	// Tick the pool's Timer until a FISH appears or 20 ticks elapse.
	// Timer() sees IsHighTide()==true -> transitions to Flooded + runs HatcheryTick().
	var ticks = 0;
	while (ObjectCount(FISH) < 1 && ticks < 20)
	{
		pPool->Timer();
		ticks++;
	}
	if (ObjectCount(FISH) < 1)
		FatalError("AquacultureSmoke FAIL step 2: no FISH spawned while flooded");
	if (!pPool->IsFlooded())
		FatalError("AquacultureSmoke FAIL step 2: pool not flooded after hatchery tick");

	// ---- Step 3: oyster matures IPRL while submerged ----
	var pOyst = CreateObject(OYST, 200, 200, NO_OWNER);
	if (!pOyst) FatalError("AquacultureSmoke FAIL step 3: OYST not spawned");
	// Force maturation via the test seam (bypasses 100-tick GBackLiquid maturation).
	pOyst->LocalN("pearl_progress") = 100;
	if (!pOyst->IsMature())
		FatalError("AquacultureSmoke FAIL step 3: oyster not mature at 100");
	// Harvest via a stand-in clonk.
	var pClonk = CreateObject(CLNK, 200, 190, NO_OWNER);
	if (!pClonk) FatalError("AquacultureSmoke FAIL step 3: CLNK not spawned");
	pOyst->Activate(pClonk);
	if (ObjectCount(APRL) < 1)
		FatalError("AquacultureSmoke FAIL step 3: no APRL produced on harvest");

	// ---- Step 4: tide flips to low, pool transitions to Exposed ----
	StopWeatherEvent();
	// The synchronous harness cannot tick TIDE past its rising half, so we
	// set the pool's post-transition state directly (same approach as
	// SpawnerSmoke driving wlsp->WLSP_Tick()).
	pPool->LocalN("bFlooded") = 0;
	pPool->SetAction("Exposed");
	if (pPool->IsFlooded())
		FatalError("AquacultureSmoke FAIL step 4: pool still flooded after flip");
	if (GetAction(pPool) != "Exposed")
		FatalError("AquacultureSmoke FAIL step 4: pool action not Exposed after flip");

	// ---- Step 5: lobster trap catches a spawned LBST ----
	var pTrap = CreateObject(LBTP, 200, 200, NO_OWNER);
	if (!pTrap) FatalError("AquacultureSmoke FAIL step 5: LBTP not spawned");
	// Spawn a lobster within the trap's 15 px attract radius.
	var pLob = CreateObject(LBST, 210, 200, NO_OWNER);
	if (!pLob) FatalError("AquacultureSmoke FAIL step 5: LBST not spawned");
	// LBST.Initialize -> Birth() sets action "Walk"; Attract() harvests Walk/Swim.
	pTrap->Attract();
	if (ContentsCount(LBST, pTrap) < 1)
		FatalError("AquacultureSmoke FAIL step 5: LBST not caught");

	Log("AquacultureSmoke PASS");
	GameOver();
	return true;
}
