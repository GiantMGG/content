/*-- SturmfrontSmoke.c4s -- compressed mirror of Sturmfront.c4s ---------
  Permanent gate (cycle 175, roadmap sturmfront-prototype). The glob
  registers this as smoke_SturmfrontSmoke @ --smoke-run 350, TIMEOUT 60,
  PASS regex "SturmfrontSmoke PASS", FAIL regex "FatalError|[error]|[fatal]".

  8 steps x 35 ticks = 280 of the 350 budget (2 spare steps). Each step
  is one compressed front or one rule pin; the full-pacing playtest runs
  Sturmfront.c4s itself (spec "Instrumented playtest").

  Mirror discipline (FirstLightSmoke precedent): predicates are
  DUPLICATED here, not shared -- drift is caught by the map pins, and
  both headers cross-reference. Map.bmp is a byte-copy of the
  prototype's (verified at authoring time; a structural divergence
  fails step 1 loudly -- risk R6).

  Banked primitives used (probe evidence, scratch/175/probe/):
    - 8-bit map loads statically, seed-independent (run1 map dump);
    - InsertMaterial into free cells is same-tick GBackLiquid-visible
      (run6: paint + census in one timer call);
    - painting on top of standing water stacks (run6);
    - painted water drains through a connected carve within one
      35-tick step (run5);
    - InsertMaterial into solid (non-free) cells fails (run4) -- every
      insert below targets free air.

  RNG discipline: nothing here asserts lightning/wind randomness; the
  storm step asserts only the synchronous lifecycle (EventSmoke
  precedent). Fixtures that could burn (WRKS) are created AFTER the
  storm step, so no strike can touch them; the CTest seed pin
  (SMOKE_SEED_PIN_SturmfrontSmoke 175) pins the whole run per binary
  regardless (gotcha #8 belt-and-suspenders). --*/

#strict 2

static g_iStep;
static g_failed;      // any step FatalError'd (FatalError aborts only the
                      // current timer call -- PASS must never follow it)
static g_base_fp;     // census snapshots for before/after pairs
static g_base_ch;
static g_claim_n;

protected func Initialize()
{
	SetWind(20);
	g_iStep = 0;
	g_failed = false;
	g_base_fp = 0;
	g_base_ch = 0;
	g_claim_n = 0;
	AddEffect("RunTest", 0, 1, 35, 0, 0);
	return true;
}

global func FxRunTestStart(target, effect, temp) { return 1; }

global func SFFail(string why)
{
	g_failed = true;
	FatalError(Format("SturmfrontSmoke FAIL: %s", why));
	return true;
}

global func FxRunTestTimer(target, effect, time)
{
	++g_iStep;
	if (g_iStep == 1) return StepMapPins();
	if (g_iStep == 2) return StepRain();
	if (g_iStep == 3) return StepTrenchCarve();
	if (g_iStep == 4) return StepTrenchAssert();
	if (g_iStep == 5) return StepStorm();
	if (g_iStep == 6) return StepFlood();
	if (g_iStep == 7) return StepClaim();
	if (g_iStep == 8) return StepEval();
	return -1;
}

// ---- shared helpers (mirror copies) ----

global func Liq(int x1, int y1, int x2, int y2)
{
	var n = 0, x, y;
	for (y = y1; y < y2; y++)
		for (x = x1; x < x2; x += 2)
			if (GBackLiquid(x, y)) ++n;
	return n;
}

global func FieldCensus()
{
	return Liq(60, 420, 230, 545) + Liq(430, 420, 640, 545);
}

global func BasinCensus()
{
	// basin box only (x505-559, y446-467): excludes the drain corridor
	// (x428-503) so the step-3/4 before/after pair measures water LEAVING
	// the basin. Re-derived from the first run: the plan's FieldCensus
	// pair measured 194 -> 351 because the corridor sits INSIDE the
	// censused flank box and fills with the drained water -- the plan's
	// "flank census must drop" premise was false against the real map.
	return Liq(505, 446, 559, 468);
}

global func ChannelCensus()
{
	return Liq(240, 420, 430, 600);
}

global func SFSurf(int x)
{
	var y = 100;
	while (y < 590 && !GBackSolid(x, y)) y++;
	return y;
}

// ---- step 1: map pins (BMP value -> material, pinned not assumed) ----
global func StepMapPins()
{
	// authored size (100x60 @ zoom 10)
	if (LandscapeWidth() != 1000 || LandscapeHeight() != 600)
		SFFail("landscape size mismatch - authored map not loaded");
	// static-load marker: peak-crown rock high in the sky region, where
	// no procedural fallback would put it (probe run1 lesson)
	if (GetMaterial(930, 130) != Material("Rock"))
		SFFail("no rock at the peak (930,130) - 8-bit map did not load statically");
	if (GetMaterial(930, 300) != Material("Rock"))
		SFFail("peak column not solid rock below the crown");
	// field soil: Earth on both flanks (TEXMAP 30)
	if (GetMaterial(500, 445) != Material("Earth"))
		SFFail("east flank (500,445) not Earth");
	if (GetMaterial(150, 445) != Material("Earth"))
		SFFail("west flank (150,445) not Earth");
	var fy = SFSurf(500);
	if (fy < 430 || fy > 450) SFFail("east flank surface not near 440");
	// terrace: Earth cap (plantable ridge)
	if (GetMaterial(780, 225) != Material("Earth"))
		SFFail("terrace cap (780,225) not Earth");
	var ty = SFSurf(780);
	if (ty < 210 || ty > 240) SFFail("terrace surface not near 220");
	// the canyon is OPEN from the sky down to the river (probe-map lesson:
	// a sealed pocket is NOT a canyon -- this run must prove open air)
	var y = 0;
	while (y < 430 && !GBackSolid(300, y) && !GBackLiquid(300, y)) y += 10;
	if (y < 430) SFFail("canyon column (x=300) is not open to the sky");
	if (!GBackLiquid(300, 545)) SFFail("no standing river at (300,545)");
	if (GBackSolid(300, 545)) SFFail("river cell solid at (300,545)");
	// solid rock floor under the river and along the whole map bottom
	if (GetMaterial(300, 590) != Material("Rock")) SFFail("no rock floor under the river (300,590)");
	if (GetMaterial(50, 590) != Material("Rock")) SFFail("no bottom band under the west crag");
	if (GetMaterial(900, 590) != Material("Rock")) SFFail("no bottom band under the ridge");
	// ridge peak >= 100px above the field surface (peak 120 vs field 440)
	if (SFSurf(930) > 340) SFFail("ridge peak not >=100px above the field surface");
	Log("SturmfrontSmoke step 1: map pins hold (air/EARTH/ROCK/Water mapped)");
	return 1;
}

// ---- step 2: front 1 rain (scripted delivery; no CastPXS anywhere) ----
global func StepRain()
{
	var w = Material("Water");
	var i, x, n = 0;
	for (i = 0; i < 40; i++)
	{
		x = 440 + i * 5;   // 440..635: the east flank
		if (!GBackSolid(x, 438) && !GBackLiquid(x, 438))
			if (InsertMaterial(w, x, 438)) ++n;
	}
	if (n < 20) SFFail("rain burst inserted < 20 cells");
	var wet = 0;
	if (GBackLiquid(500, 438) || GBackLiquid(500, 440) || GBackLiquid(500, 442)) ++wet;
	if (GBackLiquid(520, 438) || GBackLiquid(520, 440) || GBackLiquid(520, 442)) ++wet;
	if (GBackLiquid(540, 438) || GBackLiquid(540, 440) || GBackLiquid(540, 442)) ++wet;
	if (wet < 1) SFFail("no wetness registered at the field surface");
	Log(Format("SturmfrontSmoke step 2: rain wet the field (%d cells)", n));
	return 1;
}

// ---- steps 3-4: trench causality, no inflow race (probe-run5 recipe) ----
global func StepTrenchCarve()
{
	// contained basin in the east flank, filled, snapshotted; then the
	// drain slot carved to the canyon in the same step -- the pair is
	// deterministic because the basin cannot drain anywhere else first
	DigFreeRect(500, 444, 60, 24);   // basin x500-560, y444-468 (earth lid above)
	var w = Material("Water");
	var n = 0, x, y;
	for (y = 466; y >= 446; y--)
		for (x = 501; x <= 559; x++)
			if (InsertMaterial(w, x, y)) ++n;
	g_base_fp = BasinCensus();
	if (g_base_fp < 100) SFFail("basin fill census too low (did the map change?)");
	// the drain: a slot from the basin floor west through the flank wall
	// into the canyon air (fully connected in one shot)
	DigFreeRect(428, 460, 76, 10);   // x428-504, y460-470: basin -> canyon
	Log(Format("SturmfrontSmoke step 3: basin %d sampled px, drain carved", g_base_fp));
	return 1;
}

global func StepTrenchAssert()
{
	// one 35-tick step of native flow later the BASIN census must have
	// STRICTLY dropped (probe-run5: connected carve drains within one
	// step; assert drop, not full drain -- robust margin). The pinch is
	// BasinCensus, NOT FieldCensus: the drain corridor (x428-504) lies
	// inside the east FieldCensus box, so drained water only travels
	// basin -> corridor WITHIN that box and the plan's FieldCensus pin
	// inverted in run-1 (194 -> 351) even though the drain worked.
	var now = BasinCensus();
	if (now >= g_base_fp)
		SFFail(Format("trench did not drain the basin (%d -> %d)", g_base_fp, now));
	Log(Format("SturmfrontSmoke step 4: drained %d -> %d in one step", g_base_fp, now));
	return 1;
}

// ---- step 5: front 2 storm lifecycle (EventSmoke precedent, sync) ----
global func StepStorm()
{
	// lightning is RNG -- never asserted. Launch/active/stop/cleared are
	// synchronous (EventSmoke.c4s:24-32 proves the shape).
	LaunchWeatherEvent(STRM, 50, 35);
	if (GetActiveWeatherEvent() != STRM)
		SFFail("STRM did not become the active event");
	StopWeatherEvent();
	// nil is NOT a literal under #strict 2 (C4AulParse.cpp:679 demands
	// STRICT3+); truthiness is the strict-2 idiom -- a cleared event
	// returns C4VNull (falsy), an active one returns a truthy C4ID.
	if (GetActiveWeatherEvent())
		SFFail("STRM did not clear after stop");
	Log("SturmfrontSmoke step 5: storm lifecycle holds");
	return 1;
}

// ---- step 6: front 3 scripted rise + dry-until-threshold ----
global func StepFlood()
{
	// Harden the plan's rows 525-527 (run-1: delta 0 -- water painted
	// 6+ px above the standing river surface is mid-air falling water,
	// NOT same-tick GBackLiquid; only contiguous stacking on standing
	// water registers instantly, probe-run6). Re-derived: scan the
	// canyon surface and paint 3 rows directly ABOVE it, so every insert
	// stacks on standing water and shows up in the same-tick census.
	var sf = 100;
	while (sf < 560 && !GBackLiquid(300, sf)) sf++;
	if (sf >= 560) SFFail("no standing river surface found at x=300");
	if (sf < 450) SFFail("river surface unrealistically high");
	g_base_ch = ChannelCensus();
	var w = Material("Water");
	var x, y, n = 0;
	for (y = sf - 1; y >= sf - 3; y--)
		for (x = 240; x <= 430; x++)
			if (InsertMaterial(w, x, y)) ++n;
	if (n < 540)
		SFFail(Format("flood rise painted only %d of 573 cells", n));
	var now = ChannelCensus();
	// Delta floor re-derived from the map (run-2: 274 with 550 painted):
	// Liq samples every 2nd x -> 3 painted rows contribute 3 x 95 = 285
	// sampled px max. 23 of 573 cells cannot take material: x=430 is the
	// solid east-flank earth column (map col 43), and the cells right at
	// x~428 are already-liquid from the step-4 trench inflow surge. So
	// ~5% loss is structural, not a flow failure -- pin at 255 (89% of
	// the ceiling, 19px under the observed 274).
	if (now - g_base_ch < 255)
		SFFail(Format("flood rise delta %d too small (painted %d)", now - g_base_ch, n));
	// the 3 new rows sit far below the rim (440) and far above the dry
	// probes: field-surface and terrace stay dry until the spill
	// threshold -- pin only what is deterministic (no RNG asserts)
	if (GBackLiquid(500, 430)) SFFail("field probe wet below the spill threshold");
	if (GBackLiquid(780, 210)) SFFail("terrace probe wet");
	Log(Format("SturmfrontSmoke step 6: canyon rose %d sampled px (painted %d)", now - g_base_ch, n));
	return 1;
}

// ---- step 7: the claim rule (flood bite, mechanically enforced) ----
global func SFClaimSweep()
{
	// mirror of Sturmfront.c4s SFClaimSweep (both flank rects, ripe
	// wheat + loose sheaves under liquid; counts into g_claim_n)
	var n = 0, rect, x1, x2;
	for (rect = 0; rect < 2; rect++)
	{
		if (rect == 0) { x1 = 60; x2 = 230; }
		else { x1 = 430; x2 = 640; }
		for (var pW in FindObjects(Find_ID(AGWH), Find_InRect(x1, 420, x2 - x1, 125)))
			if (pW->~IsRipe() && GBackLiquid(GetX(pW), GetY(pW)))
			{
				RemoveObject(pW);
				++n;
			}
		for (var pS in FindObjects(Find_ID(AGSH), Find_InRect(x1, 420, x2 - x1, 125)))
			if (!pS->Contained() && GBackLiquid(GetX(pS), GetY(pS)))
			{
				RemoveObject(pS);
				++n;
			}
	}
	g_claim_n = n;
	Log(Format("SFMT:flood_claimed=%d", n));
	return true;
}

global func StepClaim()
{
	// Flood bite, mechanically enforced: the claim rule removes ripe
	// wheat + loose sheaves whose footprint reads GBackLiquid. Run-2/3
	// lessons (SFMT:col520/col480): a painted puddle on the OPEN field
	// only holds ~3 px (x=520 sits over the drained step-3 basin that
	// swallows the paint; even on solid ground InsertMaterial into free
	// air collapses to a thin film the same tick), and the loose AGSH
	// sheaf is Float=1 -- its anchor settles ~6 px above the spawn row,
	// out of any shallow puddle. The banked way to hold DEEP water in a
	// cell of the field is the run4 recipe: carve a contained pit and
	// fill it bottom-up (570/576 retained). So: pit carved on solid
	// field WEST of the step-3 basin (x452-497 keeps its floor sealed
	// against the drained cavity), filled to a 21-row column, fixtures
	// spawned inside the pooled water -- sheaf spawn raised +6 so its
	// floated anchor still reads liquid.
	var w = Material("Water");
	var x, y, n = 0;
	DigFreeRect(452, 430, 46, 22);   // pit x452-497, y430-451 (solid field)
	for (y = 451; y >= 430; y--)
		for (x = 453; x <= 496; x++)
			if (InsertMaterial(w, x, y)) ++n;
	if (n < 700) SFFail(Format("claim pit filled only %d of 968 cells", n));
	// fixtures inside the pooled pit water (GetY reads liquid same tick)
	var p1 = CreateObject(AGWH, 460, 440, NO_OWNER);
	var p2 = CreateObject(AGWH, 480, 440, NO_OWNER);
	if (p1) { p1->SetAction("Seedling"); p1->~Grow(); p1->~Grow(); }
	if (p2) { p2->SetAction("Seedling"); p2->~Grow(); p2->~Grow(); }
	var p3 = CreateObject(AGSH, 492, 448, NO_OWNER);
	// ridge control: outside the field ROI, must survive
	var pCtl = CreateObject(AGWH, 740, 220, NO_OWNER);
	if (pCtl) pCtl->SetAction("Seedling");
	if (!p1 || !p2 || !p3 || !pCtl) SFFail("claim fixtures did not spawn");
	if (!p1->~IsRipe() || !p2->~IsRipe()) SFFail("claim wheat not ripe after Grow x2");
	if (!GBackLiquid(460, 440) && !GBackLiquid(480, 440) && !GBackLiquid(492, 440))
		SFFail("claim fixture site not wet");
	SFClaimSweep();
	var left = FindObjects(Find_ID(AGWH), Find_InRect(460, 435, 60, 20));
	if (GetLength(left) > 0) SFFail("submerged field wheat survived the sweep");
	if (g_claim_n != 3) SFFail(Format("flood_claimed=%d, want 3 (2 wheat + 1 sheaf)", g_claim_n));
	var ctl = FindObjects(Find_ID(AGWH), Find_InRect(730, 210, 30, 16));
	if (GetLength(ctl) < 1) SFFail("ridge control wheat was swept - claim rule leaks out of the field ROI");
	Log(Format("SturmfrontSmoke step 7: claim swept %d units, ridge control intact", g_claim_n));
	return 1;
}

// ---- step 8: eval unit test + PASS ----
global func SFEvaluate(int granary, int quota, bool intact, int crew)
{
	// mirror of Sturmfront.c4s SFEvaluate (verbatim -- branch for branch)
	if (granary < quota) return "granary";
	if (!intact) return "structures";
	if (crew < 1) return "crew";
	return "win";
}

global func GranaryUnits()
{
	// mirror: FLOU anywhere + AGSH in WRKS/AGWM
	var n = ObjectCount(FLOU);
	var pBase = FindObject(WRKS);
	if (pBase) n += ContentsCount(AGSH, pBase);
	var pMill = FindObject(AGWM);
	if (pMill) n += ContentsCount(AGSH, pMill);
	return n;
}

global func HomesteadIntact()
{
	var pBase = FindObject(WRKS);
	if (!pBase) return false;
	if (GetCon(pBase) < 100) return false;
	if (OnFire(pBase)) return false;
	return true;
}

global func StepEval()
{
	// decision-core branches (negative controls for the eval logic)
	if (SFEvaluate(5, 6, true, 2) != "granary") SFFail("LOSE-granary branch wrong");
	if (SFEvaluate(6, 6, true, 2) != "win") SFFail("WIN branch wrong at the quota boundary");
	if (SFEvaluate(9, 6, false, 2) != "structures") SFFail("LOSE-structures branch wrong");
	if (SFEvaluate(9, 6, true, 0) != "crew") SFFail("LOSE-crew branch wrong");
	// granary arithmetic on synthesized state
	var pBase = CreateObject(WRKS, 720, 218, NO_OWNER);
	if (!pBase) SFFail("no WRKS fixture");
	CreateContents(AGSH, pBase, 5);
	var pF = CreateObject(FLOU, 700, 210, NO_OWNER);
	if (!pF) SFFail("no FLOU fixture");
	if (GranaryUnits() != 6)
		SFFail(Format("granary arithmetic got %d, want 6 (5 sheaves + 1 flour)", GranaryUnits()));
	if (!HomesteadIntact()) SFFail("intact WRKS reported not intact");
	RemoveObject(pBase);
	if (HomesteadIntact()) SFFail("missing WRKS reported intact");
	if (g_failed) return -1;   // never PASS after any failure
	Log("SturmfrontSmoke PASS");
	GameOver();
	return -1;
}
