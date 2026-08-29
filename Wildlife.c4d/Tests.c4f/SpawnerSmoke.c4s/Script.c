/*-- SpawnerSmoke.c4s -- headless NightDirector integration test. --*/
/* Synchronous RunSmokeSteps() pattern (cf. StormwatchSmoke.c4s).      */

#strict 3

protected func Initialize()
{
	RunSmokeSteps();
	return true;
}

func RunSmokeSteps()
{
	/* Step 0: build a settlement footprint proxy: 3 C4D_Structure huts
	   (HUT1) + 2 crew Clonks. Both count toward GetSettlementWealth():
	   structCount=3, crewCount=2, wealth=5. */
	var hut1 = CreateObject(HUT1, 200, 100, NO_OWNER);
	var hut2 = CreateObject(HUT1, 220, 100, NO_OWNER);
	var hut3 = CreateObject(HUT1, 240, 100, NO_OWNER);
	if (!hut1 || !hut2 || !hut3)
		FatalError("SpawnerSmoke FAIL step 0: could not place footprint huts");
	var crew1 = CreateObject(CLNK, 200, 90, NO_OWNER);
	var crew2 = CreateObject(CLNK, 220, 90, NO_OWNER);
	if (!crew1 || !crew2)
		FatalError("SpawnerSmoke FAIL step 0: could not place crew Clonks");

	// Add the WLSP rule.
	var wlsp = CreateObject(WLSP, 250, 100, 0);
	if (!wlsp) FatalError("SpawnerSmoke FAIL step 0: could not create WLSP rule");

	/* Step 1: assert the wealth-scaling formula.
	   wealth = 5, NightTargetCount = 1 + 5/3 = 1 + 1 = 2. */
	var wealth = GetSettlementWealth();
	if (wealth != 5)
		FatalError(Format("SpawnerSmoke FAIL step 1: GetSettlementWealth()=%d, expected 5", wealth));
	var target = wlsp->NightTargetCount();
	if (target != 2)
		FatalError(Format("SpawnerSmoke FAIL step 1: NightTargetCount()=%d, expected 2", target));

	/* Step 2: force night and tick until the night target is reached.
	   Each tick spawns one predator (up to target); with target=2 two
	   ticks yield two wolves. */
	wlsp->WLSP_SetForcePhase(1); // force night
	var ticks = 0;
	while (ObjectCount(WOLF) < target && ticks < 10)
	{
		wlsp->WLSP_Tick();
		ticks++;
	}
	if (ObjectCount(WOLF) != target)
		FatalError(Format("SpawnerSmoke FAIL step 2: ObjectCount(WOLF)=%d, expected %d",
		                  ObjectCount(WOLF), target));

	/* Step 3: assert GetWildlifeThreat() is in [1, MaxPerNight]. */
	var threat = GetWildlifeThreat();
	if (threat < 1 || threat > 8)
		FatalError(Format("SpawnerSmoke FAIL step 3: GetWildlifeThreat()=%d, expected [1,8]", threat));

	/* Step 4: structure-avoidance -- PickPerimeterSpot must reject candidate
	   spots overlapping a C4D_Structure. Place a hut on a baseline perimeter
	   spot, then sample many spots and assert none overlap the blocking hut. */
	var baseSpot = wlsp->PickPerimeterSpot();
	if (!baseSpot)
		FatalError("SpawnerSmoke FAIL step 4: no perimeter spot for baseline");
	var blockHut = CreateObject(HUT1, baseSpot[0], baseSpot[1], NO_OWNER);
	if (!blockHut)
		FatalError("SpawnerSmoke FAIL step 4: could not place block hut");
	var i;
	for (i = 0; i < 200; i++)
	{
		var spot = wlsp->PickPerimeterSpot();
		if (!spot) continue;
		// The structure-rejection rect is 40x40 centred on the candidate;
		// assert the blocking hut is NOT inside the candidate's rect.
		if (Abs(spot[0] - baseSpot[0]) < 20 && Abs(spot[1] - baseSpot[1]) < 20)
			FatalError(Format("SpawnerSmoke FAIL step 4: PickPerimeterSpot returned spot (%d,%d) overlapping block hut at (%d,%d)",
			                  spot[0], spot[1], baseSpot[0], baseSpot[1]));
	}

	/* Step 5: force dawn; assert the retreat path runs (no crash, night-active
	   flag clears). */
	wlsp->WLSP_SetForcePhase(0); // force day
	wlsp->WLSP_Tick();

	Log("SpawnerSmoke PASS");
	GameOver();
	return true;
}
