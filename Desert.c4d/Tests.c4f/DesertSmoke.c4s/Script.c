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

	/* Step 4: oasis carves basin + casts Water PXS (synchronous asserts). */
	/* Deterministic ground prep: the random map may cover the probe site
	   with undiggable Rock.  Paint diggable Sand over the basin area so
	   the oasis basin carve always has known material to work on. */
	DrawMaterialQuad("Sand", 36, 26, 64, 26, 64, 36, 36, 36, false);
	var oasis = CreateObject(OASS, 50, 30, NO_OWNER);
	if (!oasis) FatalError("DesertSmoke FAIL step 4: OASS not spawned");
	if (GetMaterial(50, 30) != -1)
		FatalError("DesertSmoke FAIL step 4: OASS did not carve basin");
	var cast = oasis->Fill();
	if (cast <= 0)
		FatalError("DesertSmoke FAIL step 4: OASS Fill() cast no Water PXS");

	/* Step 5: quicksand attaches sink effect to a walker on the patch. */
	var qs = CreateObject(QKSD, 80, 30, NO_OWNER);
	if (!qs) FatalError("DesertSmoke FAIL step 5: QKSD not spawned");
	var walker = CreateObject(SCRP, GetX(qs), GetY(qs), NO_OWNER);
	if (!walker) FatalError("DesertSmoke FAIL step 5: SCRP walker not spawned");
	qs->Sink();  // one hazard tick: attaches QuicksandSink to objects on patch
	if (!GetEffect("QuicksandSink", walker))
		FatalError("DesertSmoke FAIL step 5: walker on QKSD has no sink effect");

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
