/*-- AutoTasksAccept.c4s — standing-jobs scaffold (cycle 180 auto-tasks-mvp).
   Byte-identical mirror twin in AutoTasksSmoke.c4s (plan R4): the
   SHARED-CORE region below is diffed by the mirror-drift check. Task 1:
   skeletons + fixtures. Task 2: the shared job core — JobAssign / JobCancel /
   JobWatch (+ the stall-budget statics) — inside the markers, and the
   one-cycle probe driver below them (Tasks 4/5 replace the driver). --*/

#strict 3

// SHARED-CORE-BEGIN -----------------------------------------------------------
// Station constants (world px; provisional per spec — the surface y is
// scan-derived, everything else anchors to it).

static const AT_SAWM_X = 150;        // sawmill station
static const AT_MINT_TREE_X = 185;   // reserved mint tree (pre-marked)
static const AT_GRNY_X = 450;        // granary station
static const AT_QRRY_X = 700;        // quarry station
static const AT_OASS_X = 950;        // oasis station
static const AT_CENSUS_RADIUS = 250; // near-mill WOOD census radius (px)
static const AT_SURF_MIN = 140;      // surface scan band (world px)
static const AT_SURF_MAX = 170;
static const AT_SURF_SPREAD = 5;
static AT_FIELD_PLOTS;               // [380,410,470,520] (arrays are runtime)

// Station + crew statics (C4Aul statics are one engine-wide namespace — the
// g_at_ prefix is the cycle's collision guard).
static g_at_sawm, g_at_grny, g_at_qrry, g_at_oass;
static g_at_clonk_reap, g_at_clonk_saw, g_at_clonk_quarry;
static g_at_gy;                      // pinned surface y (GRNY station scan)

// Job bookkeeping (scenario statics, bumped by the def-side job funcs; C4Aul
// statics are engine-global so the System.c4g appendtos and the scenario
// Script.c share them across script units).
static g_at_saw_cycles, g_at_saw_trees;            // Saw re-arms / claims
static g_at_quarry_cycles;                         // Quarry handoffs
static g_at_reap_deposits, g_at_oasis_deposits;    // Deposit tallies
static g_at_stall_saw, g_at_stall_reap, g_at_stall_quarry; // watchdog budgets

// FINDING 2: the JobWatch watchdog is a BOUNDED stall absorber, not a second
// loop sustainer (an unbounded watchdog would mask mutation M1). At most
// AT_STALL_BUDGET re-issues per job across the whole run; budget is reset on
// effect start. Invariant that makes M1 behave as the spec predicts:
//   1 + AT_STALL_BUDGET < 5   (budget 2 => M1 cycles <= 3 < 5-bar: RED;
//   may be raised up to 3, never to a value that breaks this).
static const AT_STALL_BUDGET = 2;

// Sturmfront SFSurfaceY shape: first solid y from 100 up.
global func ATSurfaceY(int x)
{
	var y = 100;
	while (y < 390 && !GBackSolid(x, y)) y++;
	return y;
}

// Near-mill loose-WOOD census. Context-free by construction: FindObjects with
// a Find_ID filter carries no caller position (planet/System.c4g/FindObject.c:33)
// — unlike Find_Distance, which injects this->GetX/GetY (safe only in object
// contexts like the Sickle). The census box is the metric all shipped bars
// are calibrated against.
global func ATWoodCensus()
{
	var w, n = 0;
	for (w in FindObjects(Find_ID(WOOD)))
		if (Abs(GetX(w) - GetX(g_at_sawm)) <= AT_CENSUS_RADIUS
		 && Abs(GetY(w) - GetY(g_at_sawm)) <= AT_CENSUS_RADIUS)
			n++;
	return n;
}

// Spawns + asserts every fixture. FatalError on any miss (top-level Initialize
// path => exit 1). The surface scan is the spec's color->material pin: a
// silently-fallback procedural map produces a randomized surface that fails
// the band/spread assert.
global func ATFixtures()
{
	// 1. Surface scan at the four stations.
	var i, gy, gm;
	var xs = [AT_SAWM_X, AT_GRNY_X, AT_QRRY_X, AT_OASS_X];
	var ymax = 0, ymin = 1000;
	for (i = 0; i < 4; i++)
	{
		gy = ATSurfaceY(xs[i]);
		Log(Format("ATFIX:surface x=%d y=%d", xs[i], gy));
		if (gy < AT_SURF_MIN || gy > AT_SURF_MAX)
			FatalError(Format("AutoTasksSmoke FAIL: surface y=%d at x=%d outside [%d,%d]",
			                  gy, xs[i], AT_SURF_MIN, AT_SURF_MAX));
		if (gy < ymin) ymin = gy;
		if (gy > ymax) ymax = gy;
		if (xs[i] == AT_GRNY_X) gm = gy;
	}
	if (ymax - ymin > AT_SURF_SPREAD)
		FatalError(Format("AutoTasksSmoke FAIL: surface spread %d > %d",
		                  ymax - ymin, AT_SURF_SPREAD));
	g_at_gy = gm; // pinned surface (x=450 GRNY station)

	// 2. Material pin: the plate under the quarry station is Earth.
	if (GetMaterial(700, g_at_gy + 5) != Material("Earth"))
		FatalError(Format("AutoTasksSmoke FAIL: material at (700,%d+5) is not Earth",
		                  g_at_gy));
	Log("ATFIX:mat700 ok");

	// 3. Sawmill — complete build (Pearls.c4s/Script.c:38 shape; SAWM graphic
	//    bottom at y+28, DefCore Offset=-36,-27 Height=55).
	gy = ATSurfaceY(AT_SAWM_X);
	g_at_sawm = CreateConstruction(SAWM, AT_SAWM_X, gy - 28, NO_OWNER, 100, 1);
	if (!g_at_sawm || GetCon(g_at_sawm) < 100)
		FatalError("AutoTasksSmoke FAIL: SAWM not built to con>=100");
	Log(Format("ATFIX:sawm con=%d", GetCon(g_at_sawm)));

	// 4. Forest: 10 TRE1, WIDE spacing (originally 9px — measured false
	//    premise, task-2 probe: a fallen log being pushed through a 9px-spaced
	//    forest wedges against standing trunks and the PushTo stalls forever
	//    at ~126px). 18-20px gaps leave ~12-14px trunk clearance for pushed
	//    logs; two clusters keep every tree within 500px of the mill and the
	//    push legs short. The y anchor is the surface (see the top comment:
	//    Chop's approach target must be reachable, P3-chop probe).
	var at_forest = [48, 66, 84, 102, 120, 138, 172, 190, 208, 226];
	for (i = 0; i < GetLength(at_forest); i++)
	{
		var tree = CreateObject(TRE1, at_forest[i], g_at_gy, NO_OWNER);
		if (!tree) FatalError(Format("AutoTasksSmoke FAIL: forest tree %d", i));
	}

	// 5. Reserved mint tree + pre-claim (Sawmill :58 shape: the marker makes
	//    FindTreeToChop skip it, so the saw job never touches the mint fixture).
	var mint = CreateObject(TRE1, AT_MINT_TREE_X, g_at_gy, NO_OWNER);
	if (!mint) FatalError("AutoTasksSmoke FAIL: mint tree");
	AddEffect("IntSawmillTreeMarker", mint, 1, 5000, g_at_sawm, 0, 0);

	// 6. Tree count pin (after both spawns — single unambiguous line).
	if (ObjectCount(TRE1) != 11)
		FatalError(Format("AutoTasksSmoke FAIL: TRE1 count %d != 11", ObjectCount(TRE1)));
	Log("ATFIX:trees n=11");

	// 7. Granary (donor geometry: bottom at y+0).
	g_at_grny = CreateObject(GRNY, AT_GRNY_X, g_at_gy, NO_OWNER);
	if (!g_at_grny) FatalError("AutoTasksSmoke FAIL: GRNY not spawned");
	Log("ATFIX:grny ok");

	// 8. Wheat: 4 ripe plots (Construction forces Seedling — re-set after
	//    full-con spawn, EventSmoke idiom).
	AT_FIELD_PLOTS = [380, 410, 470, 520];
	for (i = 0; i < GetLength(AT_FIELD_PLOTS); i++)
	{
		var wheat = CreateObject(AGWH, AT_FIELD_PLOTS[i], g_at_gy, NO_OWNER);
		if (!wheat) FatalError(Format("AutoTasksSmoke FAIL: wheat plot %d", i));
		wheat->SetAction("Ready");
		if (!wheat->IsRipe())
			FatalError(Format("AutoTasksSmoke FAIL: wheat %d not ripe", i));
	}
	Log("ATFIX:wheat n=4");

	// 9. Loose sickles at x 420/440 (drop to the surface).
	var sk1 = CreateObject(AGSK, 420, g_at_gy - 10, NO_OWNER);
	var sk2 = CreateObject(AGSK, 440, g_at_gy - 10, NO_OWNER);
	if (!sk1 || !sk2) FatalError("AutoTasksSmoke FAIL: sickles missing");
	Log("ATFIX:sickles n=2");

	// 10. Sandstone vein (40 wide x 20 deep = 800 px^2 ~ 400 extractions) then
	//     the quarry (bottom at y+0; probe (0,4) hits gy+4 = vein top —
	//     DesertSmoke's exact relative geometry).
	DrawMaterialQuad("Sandstone", 680, g_at_gy + 4, 720, g_at_gy + 4,
	                 720, g_at_gy + 24, 680, g_at_gy + 24, false);
	g_at_qrry = CreateObject(QRRY, AT_QRRY_X, g_at_gy, NO_OWNER);
	if (!g_at_qrry) FatalError("AutoTasksSmoke FAIL: QRRY not spawned");
	Log("ATFIX:qrry ok");

	// 11. Oasis (24x8, bottom at y+0; Construction carves the basin, Fill waters
	//     it — a pool the hauler must NOT wade: Task 2's MoveTo targets x=925).
	g_at_oass = CreateObject(OASS, AT_OASS_X, g_at_gy, NO_OWNER);
	if (!g_at_oass) FatalError("AutoTasksSmoke FAIL: OASS not spawned");
	Log("ATFIX:oass ok");

	// 12. Crew: one unowned CLNK per job (SpawnerSmoke.c4s/Script.c:22 shape).
	g_at_clonk_reap = CreateObject(CLNK, 400, g_at_gy - 8, NO_OWNER);
	g_at_clonk_saw = CreateObject(CLNK, 170, g_at_gy - 8, NO_OWNER);
	g_at_clonk_quarry = CreateObject(CLNK, 660, g_at_gy - 8, NO_OWNER);
	if (!g_at_clonk_reap || !g_at_clonk_saw || !g_at_clonk_quarry)
		FatalError("AutoTasksSmoke FAIL: crew not spawned");
	Log("ATFIX:clonks n=3");

	return true;
}

// ---- shared job core (permanent; byte-identical between the scenario pair);
// ---- menus and scripted runs both go through JobAssign (spec: same core).
// One job per clonk, then dispatch to the owning station's appended job func.
// Same-job re-assign proceeds (the job func re-arms idempotently); a clonk
// already on a DIFFERENT job is refused.
global func JobAssign(object clonk, string job)
{
	if (!clonk) return 0;
	if (clonk == g_at_clonk_reap && job != "reap") return 0;
	if (clonk == g_at_clonk_saw && job != "saw") return 0;
	if (clonk == g_at_clonk_quarry && job != "quarry") return 0;
	if (job == "saw")
	{
		g_at_clonk_saw = clonk;
		return g_at_sawm->JobSaw(clonk);
	}
	if (job == "reap")
	{
		g_at_clonk_reap = clonk;
		return g_at_grny->JobReap(clonk);
	}
	if (job == "quarry")
	{
		g_at_clonk_quarry = clonk;
		return g_at_qrry->JobQuarry(clonk);
	}
	return 0;
}

// Clear the command stack (Sturmfront :752 shape) + the matching crew static;
// clearing the static is what keeps the watchdog from resurrecting a cancelled
// job (the 35-tick JobWatch skips clonks with nil crew statics).
global func JobCancel(object clonk)
{
	if (!clonk) return 0;
	SetCommand(clonk, "None");
	if (clonk == g_at_clonk_saw)
		g_at_clonk_saw = 0;
	else if (clonk == g_at_clonk_reap)
		g_at_clonk_reap = 0;
	else if (clonk == g_at_clonk_quarry)
		g_at_clonk_quarry = 0;
	return 1;
}

// JobWatch — 35-tick bounded watchdog (global effect form,
// FirstLightClimate.c4s/Script.c:25). Re-issues a job only when its stack has
// genuinely emptied (GetCommand nil) AND budget remains; a cancelled job
// already cleared its crew static above, so the clonk is skipped entirely.
global func FxJobWatchStart(target, effect, temp)
{
	g_at_stall_saw = AT_STALL_BUDGET;
	g_at_stall_reap = AT_STALL_BUDGET;
	g_at_stall_quarry = AT_STALL_BUDGET;
	return 1;
}

global func FxJobWatchTimer(target, effect, time)
{
	// Reap pair
	if (g_at_clonk_reap)
		if (!GetCommand(g_at_clonk_reap, 0, 0))
			if (g_at_stall_reap > 0)
			{
				--g_at_stall_reap;
				g_at_grny->JobReap(g_at_clonk_reap);
				Log("ATMT:watch_reissue=reap");
			}
			else Log("ATMT:watch_budget=reap");
	// Saw pair
	if (g_at_clonk_saw)
		if (!GetCommand(g_at_clonk_saw, 0, 0))
			if (g_at_stall_saw > 0)
			{
				--g_at_stall_saw;
				g_at_sawm->JobSaw(g_at_clonk_saw);
				Log("ATMT:watch_reissue=saw");
			}
			else Log("ATMT:watch_budget=saw");
	// Quarry pair
	if (g_at_clonk_quarry)
		if (!GetCommand(g_at_clonk_quarry, 0, 0))
			if (g_at_stall_quarry > 0)
			{
				--g_at_stall_quarry;
				g_at_qrry->JobQuarry(g_at_clonk_quarry);
				Log("ATMT:watch_reissue=quarry");
			}
			else Log("ATMT:watch_budget=quarry");
	return true;
}
// SHARED-CORE-END -------------------------------------------------------------

// ---- task-2 one-cycle probe driver (Tasks 4/5 replace this section) --------
// Assign all three jobs, add the watchdog + a 35-tick probe. The probe's
// counts with no sync assists anywhere: every census/bank/deposit > 0 below
// was walked by the real chain (ATWoodCensus near the mill; SNDS at the Oasis;
// sheaves banked in the Granary).

protected func Initialize()
{
	ATFixtures();
	JobAssign(g_at_clonk_reap, "reap");
	JobAssign(g_at_clonk_saw, "saw");
	JobAssign(g_at_clonk_quarry, "quarry");
	// Strict-3 engine calls take nil (not 0) in object slots (C4AulExec
	// CheckOpPar: non-strict scripts pass empty values leniently, strict-3
	// converts strictly). FirstLightClimate's shape uses 0 — that file is
	// #strict 2. AddEffect args: name, target, prio, interval, cmdTarget, ...
	AddEffect("JobWatch", nil, 1, 35, nil, 0);
	AddEffect("ATProbe", nil, 1, 35, nil, 0);
	return true;
}

global func FxATProbeStart(target, effect, temp) { return 1; }

global func FxATProbeTimer(target, effect, time)
{
	// P1 (isolated MoveTo) proved unowned-CLNK walking works — the stall is
	// job-command-specific. Full reap state per step: ripe-wheat census, head
	// command (nil-guarded — %s with nil errors), position, carried sheaves.
	var at_ripes = 0, at_w;
	for (at_w in FindObjects(Find_ID(AGWH)))
		if (at_w->IsRipe()) at_ripes++;
	var at_rcmd = GetCommand(g_at_clonk_reap, 0, 0);
	if (!at_rcmd) at_rcmd = "-";
	var at_scmd = GetCommand(g_at_clonk_saw, 0, 0);
	if (!at_scmd) at_scmd = "-";
	Log(Format("ATPROBE:step=%d saw_cycles=%d saw_census=%d snds_oass=%d reap_banked=%d ripes=%d reap_x=%d reap_ags=%d reap_cmd=%s",
	           time / 35, g_at_saw_cycles, ATWoodCensus(),
	           ContentsCount(SNDS, g_at_oass), ContentsCount(AGSH, g_at_grny),
	           at_ripes, GetX(g_at_clonk_reap),
	           ContentsCount(AGSH, g_at_clonk_reap), at_rcmd));
	Log(Format("ATPROBE:S step=%d saw_x=%d saw_ags=%d saw_cmd=%s",
	           time / 35, GetX(g_at_clonk_saw),
	           ContentsCount(WOOD, g_at_sawm), at_scmd));
	return true;
}
