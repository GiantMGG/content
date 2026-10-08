/*-- WolfNightsSmoke.c4s -- headless C5 wolf-night leg test. --*/
/* Synchronous RunSmokeSteps() pattern (cf. SpawnerSmoke.c4s):       */
/* forced-night spawns to NightTargetCount(), the roster mirror holds */
/* and flags loss after a Kill(), dawn run attaches the retreat leash */
/* (RetreatAllPredators). No engine timers, so wolf AI never fires    */
/* mid-smoke. The roster predicate is DUPLICATED here -- mirror       */
/* discipline, never imported from Outset.c4s (FrontierSmoke          */
/* precedent).                                                        */

#strict 3

protected func Initialize()
{
	RunSmokeSteps();
	return true;
}

/* Mirrored C5 roster predicate (duplicate of the Outset.c4s C5 card's
   FRCrewTotal/FRCrewSample family, adapted to ownerless fixtures):
   the card wins iff the live-crew count never drops below the
   night-1 dusk baseline. Counts LIVE crew only -- dead clonks persist
   as corpses, so a raw ObjectCount(CLNK) would be loss-blind. */
func SmokeCrewTotal()
{
	var n = 0;
	for (var p in FindObjects(Find_ID(CLNK), Find_NoContainer()))
		if (GetAlive(p)) n++;
	return n;
}

func SmokeCrewLost(int baseline)
{
	// Mirrors Outset FRCrewSample: any in-window sample below the
	// baseline latches the loss flag.
	if (SmokeCrewTotal() < baseline) return true; // lost
	return false; // held
}

func RunSmokeSteps()
{
	/* Step 0: fixtures -- 1 C4D_Structure hut (HUT1) + 2 ownerless crew
	   Clonks (Prey=1, genuinely huntable) + the WLSP rule.
	   GetSettlementWealth() = structs(1) + crew(2) = 3. */
	var hut = CreateObject(HUT1, 200, 100, NO_OWNER);
	var crew1 = CreateObject(CLNK, 200, 90, NO_OWNER);
	var crew2 = CreateObject(CLNK, 220, 90, NO_OWNER);
	var wlsp = CreateObject(WLSP, 250, 100, 0);
	if (!hut || !crew1 || !crew2)
		FatalError("WolfNightsSmoke FAIL step 0: could not place fixtures (hut/crew)");
	if (!wlsp)
		FatalError("WolfNightsSmoke FAIL step 0: could not create WLSP rule");

	/* Step 1: assert the wealth-scaled night target is deterministic
	   and >= 1 for this fixture set (wealth 3 -> 1 + 3/3 = 2). */
	var wealth = GetSettlementWealth();
	if (wealth != 3)
		FatalError(Format("WolfNightsSmoke FAIL step 1: GetSettlementWealth()=%d, expected 3 (1 hut + 2 crew)", wealth));
	var target = wlsp->NightTargetCount();
	if (target != 2)
		FatalError(Format("WolfNightsSmoke FAIL step 1: NightTargetCount()=%d, expected 2 (1 + 3/3)", target));
	if (target < 1)
		FatalError(Format("WolfNightsSmoke FAIL step 1: NightTargetCount()=%d, expected >= 1", target));

	// Roster baseline snapshotted at night-1 dusk (Outset g_fr_roster0).
	var roster0 = SmokeCrewTotal();
	if (roster0 != 2)
		FatalError(Format("WolfNightsSmoke FAIL step 1: baseline live crew=%d, expected 2", roster0));

	/* Step 2: force night and tick until the night target is reached.
	   Each tick spawns up to one predator; target=2 takes two ticks. */
	wlsp->WLSP_SetForcePhase(1); // force night
	var ticks = 0;
	while (ObjectCount(WOLF) < target && ticks < 10)
	{
		wlsp->WLSP_Tick();
		ticks++;
	}
	if (ObjectCount(WOLF) != target)
		FatalError(Format("WolfNightsSmoke FAIL step 2: ObjectCount(WOLF)=%d, expected %d",
		                  ObjectCount(WOLF), target));

	/* Step 3: roster mirror (held) -- no crew lost, so the mirrored
	   C5 predicate reports "held". */
	if (SmokeCrewLost(roster0))
		FatalError(Format("WolfNightsSmoke FAIL step 3: roster mirror flagged loss with live crew=%d, baseline %d",
		                  SmokeCrewTotal(), roster0));

	/* Step 4: loss leg -- Kill() one crew clonk; the live count must
	   drop below baseline and the mirror must flag "lost". The corpse
	   persists, but GetAlive() excludes it. */
	if (!GetAlive(crew1))
		FatalError("WolfNightsSmoke FAIL step 4: crew1 not alive before Kill()");
	Kill(crew1);
	if (SmokeCrewTotal() != 1)
		FatalError(Format("WolfNightsSmoke FAIL step 4: live crew after Kill()=%d, expected 1 (corpse persists but is not live)",
		                  SmokeCrewTotal()));
	if (!SmokeCrewLost(roster0))
		FatalError("WolfNightsSmoke FAIL step 4: roster mirror did NOT flag loss after a crew Kill()");

	/* Step 5: dawn -- restore auto phase (no TIME object -> day) and
	   tick; the day path runs RetreatAllPredators, and every live wolf
	   carries WLF_RetreatLeash (SpawnerSmoke step-5 precedent). */
	wlsp->WLSP_SetForcePhase(-1); // auto
	wlsp->WLSP_Tick();
	var wolves = FindObjects(Find_ID(WOLF), Find_NoContainer());
	for (var w in wolves)
	{
		if (!GetAlive(w)) continue;
		if (!GetEffect("WLF_RetreatLeash", w))
			FatalError("WolfNightsSmoke FAIL step 5: live wolf without WLF_RetreatLeash at dawn");
	}

	Log("WolfNightsSmoke PASS");
	GameOver();
	return true;
}
