/*-- FrontierSmoke.c4s -- compressed mirror of the Frontier mechanics
  (cycle 199, roadmap frontier-mvp; restructured cycle 205 for the
  frontier-flood-physical step table). Registered by the tests/CMakeLists.txt
  glob as smoke_FrontierSmoke @ --smoke-run 350 with seed pin 199.
  8 steps x 35 ticks = 280 of the 350 budget. Mirror discipline
  (SturmfrontSmoke precedent): predicates are DUPLICATED here, not shared
  with Worlds.c4f/Outset.c4s.

  1 fixtures   -- granary twin, fixture clonk, waterline scan sane;
                  record the pre-rise baseline g_wy0.
  2 DRGT+hunger -- active event == DRGT (old step 2's assert, MUT-1's
                  target, kept); energy drop measurable (old step 3's
                  assert, MUT-4's target, kept).
  3 feed+verdict -- Eat restores energy + DRGT cleared (old step 4's three
                  asserts, kept); verdict mirror: won AND lost phrasing
                  both correct (new assert, "verdict: format broken").
  4 deposit+cap -- bank >= 10 (old step 5's assert, kept); 11th deposit
                  refused at the cap-10 and the sheaf not lost (old step
                  6's two asserts, MUT-2's target, kept).
  5 rise       -- mirror of Outset FRRiseTick: snapshot own onset
                  geometry, dynamic in-zone walk painting with
                  InsertMaterial toward the SMK_RISE_PX ceiling; assert
                  waterline delta >= 4 (new assert; logs FRSMK:rise_delta).
  6 sweep      -- old step 7's asserts verbatim, now under REAL risen
                  water: band AGSH claimed, control survives, granary
                  bank intact.
  7 recede     -- mirror of Outset FRRecedeTick: per-cell ExtractLiquid
                  walk back down to the per-column onset surfaces; assert
                  waterline back within 3 px of g_wy0 (new assert; logs
                  FRSMK:recede_delta).
  8 PASS       -- "FrontierSmoke PASS"; GameOver().
  The real deal prints are pinned by frontier_outset_boot (MUT-3 reddens
  that). --*/

#strict 2

static g_iStep;
static g_grny;
static g_clonk;
static g_e0;
static g_e1;
static g_wy0;        // pre-rise waterline baseline (step 1)
static g_smk_base;   // step-5 onset waterline (rise ceiling reference)
static g_smk_col0;   // per-column onset surface array (recede target)
static g_smk_cursor; // rise round-robin column cursor
static g_smk_recede; // recede round-robin column cursor
static g_smk_rate;   // walk cells per call (sized for the mirror's map)

// Mirror rise ceiling. NOTE: 10 px, not Outset's 40 -- the smoke's own
// map (Amplitude=40/Period=20/Random=40, seed 199) has NO ground above a
// 40-px-risen band: probe PR3T (scratch/205/t3/probe.log) measured
// wdt=1000 hgt=400 wy=210 terrain=[137,270] highAboveRisen=0, and the
// sweep's control fixture needs a column with surface < waterline-60.
// Any waterline delta >= 13 px leaves every column in-band and the old
// "no high-ground fixture spot"/"sweep removed wrong count" asserts
// cannot hold; delta in [4,12] keeps the control ground. 10 px is
// contained (islands at 137-149 sit >= 51 px above the risen 200) and
// satisfies the >= 4 rise gate with margin.
static const SMK_RISE_PX = 10;

// Verdict quota mirror (Outset FREvalC3: banked >= 8 wins).
static const SMK_VERDICT_QUOTA = 8;

protected func Initialize()
{
	g_iStep = 0;
	g_grny = 0;
	g_clonk = 0;
	g_e0 = 0;
	g_e1 = 0;
	g_wy0 = 0;
	g_smk_base = 0;
	g_smk_col0 = 0;
	g_smk_cursor = 0;
	g_smk_recede = 0;
	g_smk_rate = 0;
	AddEffect("RunTest", 0, 1, 35, 0, 0);
	return true;
}

global func FxRunTestTimer(target, effect, time)
{
	++g_iStep;
	if (g_iStep == 1) return StepFixtures();
	if (g_iStep == 2) return StepDroughtHunger();
	if (g_iStep == 3) return StepFeedVerdict();
	if (g_iStep == 4) return StepDeposit();
	if (g_iStep == 5) return StepRise();
	if (g_iStep == 6) return StepSweep();
	if (g_iStep == 7) return StepRecede();
	if (g_iStep == 8) return StepPass();
	return -1;
}

global func SmkFail(string why)
{
	FatalError(Format("FrontierSmoke FAIL: %s", why));
	return true;
}

global func SmkSurfaceY(int x)
{
	var y = 10;
	while (y < LandscapeHeight() - 5 && !GBackSolid(x, y)) y++;
	return y;
}

global func SmkWaterY()
{
	var wdt = LandscapeWidth();
	var x, y, best = LandscapeHeight();
	for (x = wdt / 10; x < wdt; x += wdt / 10)
	{
		y = 0;
		while (y < LandscapeHeight() && !GBackLiquid(x, y)) y++;
		if (y < best) best = y;
	}
	return best;
}

global func SmkInBand(int x, int y)
{
	return y >= SmkWaterY() - 60;
}

// Compressed FRSweep mirror: loose AGSH in the band is claimed.
global func SmkSweep()
{
	var n = 0;
	var pS;
	for (pS in FindObjects(Find_ID(AGSH)))
		if (!pS->Contained() && SmkInBand(GetX(pS), GetY(pS)))
		{
			RemoveObject(pS);
			++n;
		}
	return n;
}

// Verdict phrasing mirror of Outset FREvalC3 (pure helper, SFEvaluate-mirror
// precedent): won phrasing when banked >= quota, lost phrasing otherwise;
// both branches carry "Drought broken", the banked count and "(need 8)".
global func SmkVerdict(int banked)
{
	if (banked >= SMK_VERDICT_QUOTA)
		return Format("Drought broken -- %d edibles banked (need 8) -- the granary held", banked);
	return Format("Drought broken -- %d edibles banked (need 8) -- the clonks go hungry", banked);
}

// Onset snapshot (mirror of Outset FRFloodSnapshot): per-column top-liquid
// surface for EVERY column in [0, LandscapeWidth()) -- the per-column
// recede target (dry columns store the LandscapeHeight() sentinel) -- plus
// the baseline waterline. Runs at the step-5 start; the walk rate is sized
// so one call finishes the whole walk at mirror sizes (wdt * (PX + 24)
// cells covers the full 10-px rise volume; the walks' step-guard ends the
// call early on convergence).
global func SmkFloodSnapshot()
{
	var wdt = LandscapeWidth();
	g_smk_base = SmkWaterY();
	g_smk_cursor = 0;
	g_smk_recede = 0;
	g_smk_col0 = CreateArray(wdt);
	var x, y;
	for (x = 0; x < wdt; x++)
	{
		y = 0;
		while (y < LandscapeHeight() && !GBackLiquid(x, y)) y++;
		g_smk_col0[x] = y;
	}
	g_smk_rate = wdt * (SMK_RISE_PX + 24);
	Log(Format("FRSMK:geom base_wy=%d wdt=%d", g_smk_base, wdt));
	return true;
}

// Rise mirror of Outset FRRiseTick (dynamic extent): every call re-discovers
// the flooded columns (surface y in [base - PX, base]) and paints one cell
// just above each column's current surface, row by row toward the ceiling.
// Round-robin column cursor; a full pass that paints nothing ends the call.
global func SmkRiseTick()
{
	var w = Material("Water");
	var wdt = LandscapeWidth();
	var n = 0, step = 0, x, y;
	while (n < g_smk_rate)
	{
		x = g_smk_cursor % wdt;
		++g_smk_cursor;
		++step;
		y = 0;
		while (y < LandscapeHeight() && !GBackLiquid(x, y)) y++;
		if (y > g_smk_base) { if (step > wdt) break; continue; }
		if (y <= g_smk_base - SMK_RISE_PX) { if (step > wdt) break; continue; }
		InsertMaterial(w, x, y - 1);
		++n;
		step = 0;
	}
	return true;
}

// Recede mirror of Outset FRRecedeTick (dynamic extent): per-cell
// ExtractLiquid at every column whose CURRENT surface is above its onset
// target, walking down toward g_smk_col0[x]; never below the onset surface,
// so the standing lake stays and dry-at-onset columns drain to the sentinel.
global func SmkRecedeTick()
{
	var wdt = LandscapeWidth();
	var n = 0, step = 0, x, y, t;
	while (n < g_smk_rate)
	{
		x = g_smk_recede % wdt;
		++g_smk_recede;
		++step;
		y = 0;
		while (y < LandscapeHeight() && !GBackLiquid(x, y)) y++;
		t = g_smk_col0[x];
		if (y < t)
		{
			ExtractLiquid(x, y);
			++n;
			step = 0;
		}
		if (step > wdt) break;
	}
	return true;
}

global func StepFixtures()
{
	// granary twin on a dry surface spot
	var gx = LandscapeWidth() / 2, gy, tr;
	for (tr = 0; tr < 8; tr++)
	{
		gy = SmkSurfaceY(gx);
		if (!GBackLiquid(gx, gy - 4)) break;
		gx += 25;
	}
	g_grny = CreateObject(GRNY, gx, gy, NO_OWNER);
	if (!g_grny) return SmkFail("GRNY spawn");

	// fixture clonk (ownerless CLNK is OCF_CrewMember: CrewMember=1)
	g_clonk = CreateObject(CLNK, gx + 40, gy - 20, NO_OWNER);
	if (!g_clonk) return SmkFail("fixture clonk");
	g_e0 = GetEnergy(g_clonk);

	var wy = SmkWaterY();
	if (wy <= 10 || wy >= LandscapeHeight()) return SmkFail("waterline scan");
	g_wy0 = wy;
	Log(Format("FRSMK:water_y=%d", wy));
	Log(Format("FRSMK:fixtures ok (grny=%d,%d energy=%d)", gx, gy, g_e0));
	return 1;
}

global func StepDroughtHunger()
{
	// (old step 2) DRGT launch -- MUT-1's target assert
	LaunchWeatherEvent(DRGT, 50, 700);
	if (GetActiveWeatherEvent() != DRGT)
		return SmkFail("DRGT not active after launch");
	Log("FRSMK:announce=The drought is upon us -- keep every clonk fed.");
	// (old step 3) compressed FRHunger mirror: one drain beat on the
	// fixture (MUT-4 disables this line -- the drop assert must go red).
	if (GetEnergy(g_clonk) > 15) DoEnergy(-2, g_clonk);
	g_e1 = GetEnergy(g_clonk);
	if (g_e1 >= g_e0)
		return SmkFail("hunger drop not measurable");
	Log(Format("FRSMK:hunger energy=%d", g_e1));
	return 1;
}

global func StepFeedVerdict()
{
	// (old step 4, all three asserts kept)
	CreateContents(AGAP, g_grny, 1);
	if (!g_grny->Eat(g_clonk)) return SmkFail("GRNY Eat refused");
	if (GetEnergy(g_clonk) <= g_e1)
		return SmkFail("Eat did not restore energy");
	StopWeatherEvent();
	if (GetActiveWeatherEvent() != 0) return SmkFail("event not cleared");
	// verdict mirror: won phrasing (banked 9 >= 8) AND lost phrasing
	// (banked 7 < 8) must both carry "Drought broken", the banked count
	// and "(need 8)" (exact compare against the Outset phrasing; MUT-7
	// swaps the branches -- the won check must go red).
	if (SmkVerdict(9) != Format("Drought broken -- 9 edibles banked (need 8) -- the granary held"))
		return SmkFail("verdict: format broken");
	if (SmkVerdict(7) != Format("Drought broken -- 7 edibles banked (need 8) -- the clonks go hungry"))
		return SmkFail("verdict: format broken");
	Log("FRSMK:feed restored energy; DRGT cleared; verdict both cases ok");
	return 1;
}

global func StepDeposit()
{
	// (old step 5) bank one sheaf per leg until the granary holds 10
	// (player-check element 4: bank >= 10 in the granary)
	var legs = 0;
	while (ContentsCount(AGSH, g_grny) + ContentsCount(AGAP, g_grny) < 10 && legs < 20)
	{
		CreateContents(AGSH, g_clonk, 1);
		if (!g_grny->Deposit(g_clonk)) break;
		++legs;
	}
	var banked = ContentsCount(AGSH, g_grny) + ContentsCount(AGAP, g_grny);
	if (banked < 10) return SmkFail(Format("banked %d < 10", banked));
	Log(Format("FRSMK:banked=%d legs=%d", banked, legs));
	// (old step 6, both asserts kept) 11th deposit refused at the cap-10
	// (AutoTasksSmoke step-2c pattern; MUT-2's target assert)
	CreateContents(AGSH, g_clonk, 1);
	g_grny->Deposit(g_clonk);
	if (ContentsCount(AGSH, g_grny) != 10)
		return SmkFail("cap broken: granary != 10");
	if (ContentsCount(AGSH, g_clonk) < 1)
		return SmkFail("sheaf lost on refusal");
	Log("FRSMK:cap=10 held");
	return 1;
}

global func StepRise()
{
	// snapshot own onset geometry (mirror FRFloodSnapshot at step-5 start)
	SmkFloodSnapshot();
	// dynamic rise walk to the SMK_RISE_PX ceiling -- completes in this
	// one call (mirror-sized rate; the walk's step-guard ends it early)
	SmkRiseTick();
	var wy1 = SmkWaterY();
	var rise = g_smk_base - wy1;
	if (rise < 0) rise = -rise;
	Log(Format("FRSMK:rise_delta=%d", rise));
	if (rise < 4) return SmkFail("rise: waterline delta < 4");
	Log("FRSMK:rise ok -- flood over the floodplain");
	return 1;
}

global func StepSweep()
{
	// (old step 7, asserts kept verbatim -- now under REAL risen water:
	// the band below the step-5 risen waterline is truly flooded)
	// band fixture (claimed) + control fixture (survives); the granary bank
	// must come through the sweep untouched.
	var cx = -1, hx = -1, x, sy;
	for (x = 20; x < LandscapeWidth() && (cx < 0 || hx < 0); x += 10)
	{
		sy = SmkSurfaceY(x);
		if (cx < 0 && SmkInBand(x, sy)) cx = x;
		if (hx < 0 && !SmkInBand(x, sy)) hx = x;
	}
	if (cx < 0) return SmkFail("no floodplain fixture spot");
	if (hx < 0) return SmkFail("no high-ground fixture spot");
	var pClaim = CreateObject(AGSH, cx, SmkSurfaceY(cx), NO_OWNER);
	var pCtrl = CreateObject(AGSH, hx, SmkSurfaceY(hx), NO_OWNER);
	if (!pClaim || !pCtrl) return SmkFail("sweep fixtures missing");
	var before = ObjectCount(AGSH);
	var n = SmkSweep();
	if (n < 1) return SmkFail("claim sweep removed nothing");
	if (ObjectCount(AGSH) != before - 1)
		return SmkFail("sweep removed wrong count");
	if (ContentsCount(AGSH, g_grny) != 10)
		return SmkFail("sweep touched the granary bank");
	Log(Format("FRSMK:sweep claimed=%d bank intact", n));
	return 1;
}

global func StepRecede()
{
	// dynamic recede walk back down to the per-column onset surfaces
	// (mirror FRRecedeTick; completes in this one call)
	SmkRecedeTick();
	var wy2 = SmkWaterY();
	var back = wy2 - g_wy0;
	if (back < 0) back = -back;
	Log(Format("FRSMK:recede_delta=%d", back));
	if (back > 3) return SmkFail("recede: waterline not back within 3");
	Log("FRSMK:recede ok -- waterline back on the onset surface");
	return 1;
}

global func StepPass()
{
	Log("FrontierSmoke PASS");
	GameOver();
	return 1;
}
