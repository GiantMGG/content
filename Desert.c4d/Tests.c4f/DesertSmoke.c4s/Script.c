/*-- DesertSmoke.c4s -- headless content integration test. --*/
/* Synchronous RunSmokeSteps() pattern (cf. EventSmoke.c4s).     */
/* The AddEffect timer pattern does NOT fire reliably in the     */
/* smoke harness, so every interaction is driven synchronously.  */

#strict 3

protected func Initialize()
{
	RunSmokeSteps();
	return true;
}

func RunSmokeSteps()
{
	/* Step 0: SNDT event lifecycle (launch -> active -> stop -> cleared). */
	LaunchWeatherEvent(SNDT, 50, 100);
	if (GetActiveWeatherEvent() != SNDT)
		FatalError("DesertSmoke FAIL step 0: SNDT not active");
	StopWeatherEvent();
	if (GetActiveWeatherEvent() != nil)
		FatalError("DesertSmoke FAIL step 0: SNDT not stopped");

	/* Step 1: date palm 3-stage growth + harvest (soil-agnostic,
	   Wheat precedent -- deterministic at any spawn point). */
	var palm = CreateObject(DATP, 50, 30, NO_OWNER);
	if (!palm) FatalError("DesertSmoke FAIL step 1: DATP not spawned");
	palm->SetAction("Seedling");
	palm->Grow();  // Seedling -> Growing
	palm->Grow();  // Growing -> Ready
	if (!palm->IsRipe())
		FatalError("DesertSmoke FAIL step 1: DATP not ripe after 2 Grow() calls");
	var dates = palm->Harvest();
	if (!dates) FatalError("DesertSmoke FAIL step 1: Harvest failed");

	/* Step 2: DATE food item spawns (Script.c compiles => Apple-style
	   Activate/Eat contract present; eating is play-verified, not smoked) */
	var date = CreateObject(DATE, 50, 30, NO_OWNER);
	if (!date) FatalError("DesertSmoke FAIL step 2: DATE not spawned");

	/* Step 3: scorpion + camel spawn with WLFA behaviours. */
	var scorpion = CreateObject(SCRP, 50, 30, NO_OWNER);
	if (!scorpion) FatalError("DesertSmoke FAIL step 3: SCRP not spawned");
	if (!GetEffect("WLF_HuntPrey", scorpion))
		FatalError("DesertSmoke FAIL step 3: SCRP missing WLF_HuntPrey");
	var camel = CreateObject(CAML, 60, 30, NO_OWNER);
	if (!camel) FatalError("DesertSmoke FAIL step 3: CAML not spawned");

	/* Step 3b: scorpion sting slows the victim (spec §3: WLFA_Attack +
	   Webbed-style physical reduction, reused). */
	var victim = CreateObject(SCRP, 52, 30, NO_OWNER);
	if (!victim) FatalError("DesertSmoke FAIL step 3b: SCRP victim not spawned");
	var victim_walk = GetPhysical("Walk", 0, victim);
	scorpion->WLFA_Attack(victim, 8);
	if (!GetEffect("ScorpionVenom", victim))
		FatalError("DesertSmoke FAIL step 3b: sting applied no venom");
	if (GetPhysical("Walk", 0, victim) >= victim_walk)
		FatalError("DesertSmoke FAIL step 3b: venom did not slow the victim");

	/* Step 4: oasis carves basin + fills it with Water, self-limiting
	   once the basin is full (H1: Fill()'s probe and cast coordinates
	   are oasis-relative and must land inside the carved basin). */
	/* Deterministic ground prep: the random map may cover the probe site
	   with undiggable Rock.  Paint diggable Sand over the basin area so
	   the oasis basin carve always has known material to work on. */
	DrawMaterialQuad("Sand", 36, 26, 64, 26, 64, 36, 36, 36, false);
	var oasis = CreateObject(OASS, 50, 30, NO_OWNER);
	if (!oasis) FatalError("DesertSmoke FAIL step 4: OASS not spawned");
	if (GetMaterial(50, 30) != -1)
		FatalError("DesertSmoke FAIL step 4: OASS did not carve basin");
	var cast = oasis->Fill();
	if (!cast)
		FatalError("DesertSmoke FAIL step 4: OASS Fill() cast no Water PXS");
	/* Simulate a full basin deterministically (no PXS physics in the
	   harness): paint Water directly over the basin-full probe band.
	   DrawMaterialQuad is absolute; the probe pixels are oasis-relative
	   (-2,-2)/(2,-2) = absolute (48,28)/(52,28). */
	DrawMaterialQuad("Water", 46, 28, 54, 28, 54, 32, 46, 32, false);
	if (GetMaterial(48, 28) != Material("Water")
	 || GetMaterial(52, 28) != Material("Water"))
		FatalError("DesertSmoke FAIL step 4: basin probe pixels are not Water");
	/* Fill()'s "0" return is nil at the oasis script's strict level
	   (0 == nil below #strict 3), so assert falsiness rather than
	   equality with 0. */
	if (oasis->Fill())
		FatalError("DesertSmoke FAIL step 4: Fill() did not self-limit on a full basin");

	/* Step 4b: oasis growth boost (spec §5: palms near an oasis grow
	   faster).  One boosted Grow() advances two stages at once. */
	var palm3 = CreateObject(DATP, 55, 30, NO_OWNER);
	if (!palm3) FatalError("DesertSmoke FAIL step 4b: DATP not spawned");
	palm3->SetAction("Seedling");
	palm3->Grow();
	if (!palm3->IsRipe())
		FatalError("DesertSmoke FAIL step 4b: DATP near OASS did not grow boosted");

	/* Step 5: quicksand attaches sink effect to a walker on the patch,
	   the Walk physical is cut while sinking, and the effect ends once
	   the walker escapes the patch (H2). */
	var qs = CreateObject(QKSD, 80, 30, NO_OWNER);
	if (!qs) FatalError("DesertSmoke FAIL step 5: QKSD not spawned");
	var walker = CreateObject(SCRP, GetX(qs), GetY(qs), NO_OWNER);
	if (!walker) FatalError("DesertSmoke FAIL step 5: SCRP walker not spawned");
	var walk_base = GetPhysical("Walk", 0, walker);
	qs->Sink();  // one hazard tick: attaches QuicksandSink to objects on patch
	var sink_num = GetEffect("QuicksandSink", walker);
	if (!sink_num)
		FatalError("DesertSmoke FAIL step 5: walker on QKSD has no sink effect");
	/* On the patch: the timer keeps the effect alive.  FX_OK (0) is nil
	   at the quicksand script's strict level, so assert falsiness. */
	if (qs->FxQuicksandSinkTimer(walker, sink_num, 1))
		FatalError("DesertSmoke FAIL step 5: sink timer killed the effect on-patch");
	if (GetPhysical("Walk", 0, walker) != 20000)
		FatalError("DesertSmoke FAIL step 5: sink effect did not cut Walk");
	/* Escape: move the walker off the patch; the timer must end the
	   effect (FX_Execute_Kill = -1, truthy). */
	SetPosition(GetX(walker) + 60, GetY(walker), walker);
	if (!qs->FxQuicksandSinkTimer(walker, sink_num, 2))
		FatalError("DesertSmoke FAIL step 5: sink effect did not end off-patch");
	/* The stop path restores the baseline Walk physical. */
	qs->FxQuicksandSinkStop(walker, sink_num, 0, false);
	if (GetPhysical("Walk", 0, walker) != walk_base)
		FatalError("DesertSmoke FAIL step 5: Walk physical not restored after escape");

	/* Step 6: quarry produces SNDS from Sandstone ground (ground prep is
	   deterministic: paint a solid Sandstone vein, then Quarry()). */
	DrawMaterialQuad("Sandstone", 86, 34, 98, 34, 98, 40, 86, 40, false);
	var quarry = CreateObject(QRRY, 90, 30, NO_OWNER);
	if (!quarry) FatalError("DesertSmoke FAIL step 6: QRRY not spawned");
	if (!quarry->Quarry())
		FatalError("DesertSmoke FAIL step 6: QRRY Quarry() produced nothing");
	if (ContentsCount(SNDS, quarry) < 1)
		FatalError("DesertSmoke FAIL step 6: no SNDS in QRRY contents");

	/* Step 7: sandstorm pauses palm growth (event coupling). */
	var palm2 = CreateObject(DATP, 40, 30, NO_OWNER);
	palm2->SetAction("Seedling");
	LaunchWeatherEvent(SNDT, 50, 100);
	palm2->Grow();
	if (palm2->GetAction() != "Seedling")
		FatalError("DesertSmoke FAIL step 7: DATP grew during SNDT");
	StopWeatherEvent();

	Log("DesertSmoke PASS");
	GameOver();
	return true;
}
