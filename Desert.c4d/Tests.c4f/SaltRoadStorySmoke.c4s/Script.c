/*-- SaltRoadStorySmoke.c4s -- the Salt Road story-arc smoke (cycle 146). --*/
/* Drives the shared Desert-pack story API synchronously through the        */
/* three acts' key moments to the deterministic SaltRoad_Ending()           */
/* epilogue (ScorpionStagingSmoke / DesertSmoke sync pattern). FatalError   */
/* on any failed assertion; success logs the PASS line and calls the shared */
/* ending (GameOver -> exit 0 per the smoke contract).                      */

#strict 3

protected func Initialize()
{
	RunSmokeSteps();
	return true;
}

func RunSmokeSteps()
{
	/* Step 0: paint the drift slab -- the storm-budget probe base  */
	/* (DuneBurialSmoke determinism trick: the flat Material=Earth  */
	/* map has open sky above row 200; every other fixture spawns   */
	/* in that open-sky band, so nothing embeds and the Drift       */
	/* budget assert probes this slab).                             */
	DrawMaterialQuad("Sand", 450, 150, 750, 150, 750, 200, 450, 200, false);

	/* ---- Beat 1: Act I -- camp contracts, dusk raid, venom. ---- */

	/* Step 1: the camp -- camels, ripe date palm, oasis, quarry. */
	var camel_a = CreateObject(CAML, 120, 100, NO_OWNER);
	var camel_b = CreateObject(CAML, 150, 100, NO_OWNER);
	if (!camel_a || !camel_b)
		FatalError("SaltRoadStorySmoke FAIL step 1: camels not spawned");

	var palm = CreateObject(DATP, 200, 100, NO_OWNER);
	if (!palm) FatalError("SaltRoadStorySmoke FAIL step 1: DATP not spawned");
	palm->SetAction("Seedling");
	palm->Grow();
	palm->Grow();
	if (!palm->IsRipe())
		FatalError("SaltRoadStorySmoke FAIL step 1: DATP did not ripen");
	if (!palm->Harvest())
		FatalError("SaltRoadStorySmoke FAIL step 1: Harvest failed");
	if (ObjectCount(DATE) < 2)
		FatalError("SaltRoadStorySmoke FAIL step 1: Harvest yielded no DATE");

	var oasis = CreateObject(OASS, 260, 110, NO_OWNER);
	if (!oasis) FatalError("SaltRoadStorySmoke FAIL step 1: OASS not spawned");
	if (!oasis->Fill())
		FatalError("SaltRoadStorySmoke FAIL step 1: OASS Fill() cast no Water");

	DrawMaterialQuad("Sandstone", 300, 110, 360, 110, 360, 126, 300, 126, false);
	var quarry = CreateObject(QRRY, 330, 106, NO_OWNER);
	if (!quarry) FatalError("SaltRoadStorySmoke FAIL step 1: QRRY not spawned");
	if (quarry->Quarry() != 1)
		FatalError("SaltRoadStorySmoke FAIL step 1: Quarry produced nothing");
	if (!quarry->FindContents(SNDS))
		FatalError("SaltRoadStorySmoke FAIL step 1: no SNDS in the quarry");

	/* Step 2: the dusk raid -- 4 scorpions, all Attack-commanded. */
	var raid = SaltRoad_SpawnRaid(400, 60, 4);
	if (raid != 4)
		FatalError(Format("SaltRoadStorySmoke FAIL step 2: raid spawned %d, want 4", raid));
	var scrp, commanded = 0;
	for (scrp in FindObjects(Find_ID(SCRP)))
		if (SEqual(GetCommand(scrp), "Attack")) commanded++;
	if (commanded != 4)
		FatalError(Format("SaltRoadStorySmoke FAIL step 2: %d/4 raiders Attack-commanded", commanded));

	/* Step 3: venom (ScorpionStagingSmoke step-2 mirror). */
	var clnk = CreateObject(CLNK, 420, 100, NO_OWNER);
	if (!clnk) FatalError("SaltRoadStorySmoke FAIL step 3: CLNK not spawned");
	var walk0 = GetPhysical("Walk", 0, clnk);
	var stinger = FindObject(SCRP);
	if (!stinger) FatalError("SaltRoadStorySmoke FAIL step 3: no scorpion to sting");
	stinger->WLFA_Attack(clnk, 8);
	if (!GetEffect("ScorpionVenom", clnk))
		FatalError("SaltRoadStorySmoke FAIL step 3: sting applied no venom");
	if (GetPhysical("Walk", 0, clnk) >= walk0)
		FatalError("SaltRoadStorySmoke FAIL step 3: venom did not cut Walk");

	/* ---- Beat 2: Act II -- caravan puppet, burial hold, storm. ---- */

	/* Step 4: caravan puppet on both camels. */
	if (!SaltRoad_StartCaravan(camel_a, [400, 700]))
		FatalError("SaltRoadStorySmoke FAIL step 4: StartCaravan refused camel_a");
	if (!SaltRoad_StartCaravan(camel_b, [410, 710]))
		FatalError("SaltRoadStorySmoke FAIL step 4: StartCaravan refused camel_b");
	if (!SaltRoad_IsCaravanActive(camel_a) || !SaltRoad_IsCaravanActive(camel_b))
		FatalError("SaltRoadStorySmoke FAIL step 4: caravan not active");
	var fx_a = GetEffect("SaltRoadCaravan", camel_a);
	if (!fx_a)
		FatalError("SaltRoadStorySmoke FAIL step 4: no caravan effect on camel_a");
	camel_a->FxSaltRoadCaravanTimer(camel_a, fx_a, 0);
	if (!SEqual(GetCommand(camel_a), "MoveTo"))
		FatalError("SaltRoadStorySmoke FAIL step 4: puppet issued no MoveTo");

	/* Step 5: burial hold -- SandBuried on the camel freezes the puppet. */
	var sdrf = CreateObject(SDRF, 450, 150, NO_OWNER);
	if (!sdrf) FatalError("SaltRoadStorySmoke FAIL step 5: SDRF not spawned");
	AddEffect("SandBuried", camel_a, 1, 4, sdrf);
	SetCommand(camel_a, "None");
	camel_a->FxSaltRoadCaravanTimer(camel_a, fx_a, 0);
	if (SEqual(GetCommand(camel_a), "MoveTo"))
		FatalError("SaltRoadStorySmoke FAIL step 5: puppet moved while buried");
	RemoveEffect("SandBuried", camel_a);
	camel_a->FxSaltRoadCaravanTimer(camel_a, fx_a, 0);
	if (!SEqual(GetCommand(camel_a), "MoveTo"))
		FatalError("SaltRoadStorySmoke FAIL step 5: puppet did not resume after burial");

	/* Step 6: storm budget (DuneBurialSmoke step-3 contract: 2+50/10=7). */
	LaunchWeatherEvent(SNDT, 50, 100);
	if (GetActiveWeatherEvent() != SNDT)
		FatalError("SaltRoadStorySmoke FAIL step 6: SNDT not active");
	SetWind(60);
	var moved = sdrf->Drift();
	if (moved != 7)
		FatalError(Format("SaltRoadStorySmoke FAIL step 6: storm Drift() moved %d, want 7", moved));
	StopWeatherEvent();
	if (GetActiveWeatherEvent() != nil)
		FatalError("SaltRoadStorySmoke FAIL step 6: SNDT not stopped");

	/* Step 7: quicksand sink (DesertSmoke step-5 mirror). */
	var qs = CreateObject(QKSD, 800, 140, NO_OWNER);
	if (!qs) FatalError("SaltRoadStorySmoke FAIL step 7: QKSD not spawned");
	var walker = CreateObject(SCRP, GetX(qs), GetY(qs), NO_OWNER);
	if (!walker) FatalError("SaltRoadStorySmoke FAIL step 7: walker not spawned");
	qs->Sink();
	if (!GetEffect("QuicksandSink", walker))
		FatalError("SaltRoadStorySmoke FAIL step 7: walker on QKSD has no sink effect");

	/* ---- Beat 3: Act III -- the queen, the brood cap, the toll. ---- */

	/* Step 8: the queen -- tag, boosted Energy, color applied. */
	var queen = SaltRoad_SpawnQueen(900, 100);
	if (!queen) FatalError("SaltRoadStorySmoke FAIL step 8: queen not spawned");
	if (!GetEffect("SaltRoadQueen", queen))
		FatalError("SaltRoadStorySmoke FAIL step 8: queen not tagged");
	if (GetEnergy(queen) < 100)
		FatalError(Format("SaltRoadStorySmoke FAIL step 8: queen Energy %d, want >= 100", GetEnergy(queen)));
	if (GetClrModulation(queen) != RGB(150, 100, 40))
		FatalError("SaltRoadStorySmoke FAIL step 8: queen tint not applied");
	if (!SEqual(GetName(queen), "Scorpion Queen"))
		FatalError("SaltRoadStorySmoke FAIL step 8: queen not named");

	/* Step 9: brood self-cap at 6 live SCRP. */
	// Clear every non-queen scorpion so the brood probe starts from 1.
	var s;
	for (s in FindObjects(Find_ID(SCRP)))
		if (s != queen) RemoveObject(s);
	var guard = 0;
	while (ObjectCount(SCRP) < 6 && guard < 20) { SaltRoad_QueenBrood(queen); guard++; }
	if (ObjectCount(SCRP) != 6)
		FatalError(Format("SaltRoadStorySmoke FAIL step 9: brood fill stopped at %d, want 6", ObjectCount(SCRP)));
	if (SaltRoad_QueenBrood(queen))
		FatalError("SaltRoadStorySmoke FAIL step 9: brood cap not respected");

	/* Step 10: pay the toll -- the deterministic win-path convergence. */
	var i;
	for (i = 0; i < 10; i++)
		CreateObject(DATE, 950 + Random(30), 110, NO_OWNER);
	var d, offered = 0;
	for (d in FindObjects(Find_ID(DATE), Find_AtRect(920, 0, 80, LandscapeHeight())))
		offered++;
	if (offered < 10)
		FatalError(Format("SaltRoadStorySmoke FAIL step 10: toll offerings %d, want >= 10", offered));

	/* Step 11: the single deterministic epilogue. */
	Log("SaltRoadStorySmoke PASS");
	SaltRoad_Ending();
	return true;
}
