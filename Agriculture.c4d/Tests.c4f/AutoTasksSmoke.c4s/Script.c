/*-- AutoTasksSmoke.c4s — standing-jobs scaffold (cycle 180 auto-tasks-mvp).
   Byte-identical mirror twin in AutoTasksAccept.c4s (plan R4): the
   SHARED-CORE region below is diffed by the mirror-drift check. Task 1:
   skeletons + fixtures. Task 2: the shared job core — JobAssign / JobCancel /
   JobWatch (+ the stall-budget statics) — inside the markers, and the
   one-cycle probe driver below them (Tasks 4/5 replace the driver). --*/

#strict 3

// SHARED-CORE-BEGIN -----------------------------------------------------------
// Station constants (world px; provisional per spec — the surface y is
// scan-derived, everything else anchors to it).

static const AT_SAWM_X = 150;        // sawmill station
static const AT_MINT_TREE_X = 284;   // reserved mint tree (pre-marked; moved from 185 in task-2 — its trunk overlapped the SAWM footprint)
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
static g_at_r3_ticks, g_at_saw_assist;             // R3 wedged-log entrance assist

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
	//    Chop's approach target must be reachable, P3-chop probe). Task-2
	//    bisect (BI4/BI5/BI6 + SW-probes): legs 120/138/172/190 jam — their
	//    Chop approach (Target+-6, surface y) lands INSIDE the SAWM footprint
	//    x=[114,186] (SAWM Width=72 Offset=-36), the clonk climbs the mill and
	//    spins Grab/PushTo/Wait forever. Re-laid: left cluster keeps the
	//    working legs 30-102 (approach <= 108 < 114); the four invasive legs
	//    move to the right cluster clear of the mill (approach >= 188 > 186).
	var at_forest = [30, 48, 66, 84, 102, 194, 212, 230, 248, 266];
	for (i = 0; i < GetLength(at_forest); i++)
	{
		var tree = CreateObject(TRE1, at_forest[i], g_at_gy, NO_OWNER);
		if (!tree) FatalError(Format("AutoTasksSmoke FAIL: forest tree %d", i));
	}

	// 5. Reserved mint tree + pre-claim (Sawmill :58 shape: the marker makes
	//    FindTreeToChop skip it, so the saw job never touches the mint fixture).
	//    Task-2: moved from x=185 (trunk overlapped the SAWM footprint) to
	//    x=284, clear right of the right forest cluster.
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

	// 9. Loose sickles at x 620/640 (drop to the surface). Task-2 fix: the
	//    JobReap first leg (MoveTo GRNY 450) passed over the original x=420/440
	//    spawns, the unowned clonk auto-picked a sickle, and CLNK
	//    RejectCollect (MaxContentsCount 1) then refused the sheaves — the
	//    carried=0 banked=0 shape. BI6 proved 620/640 restores banked=4.
	var sk1 = CreateObject(AGSK, 620, g_at_gy - 10, NO_OWNER);
	var sk2 = CreateObject(AGSK, 640, g_at_gy - 10, NO_OWNER);
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
	//     Task-2: the saw clonk moved from x=170 (inside the SAWM footprint
	//     [114,186] — it spawned embedded in the mill) to x=300, clear of the
	//     mill and both forest clusters.
	g_at_clonk_reap = CreateObject(CLNK, 400, g_at_gy - 8, NO_OWNER);
	g_at_clonk_saw = CreateObject(CLNK, 300, g_at_gy - 8, NO_OWNER);
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
	// R3 wedged-log entrance assist (plan Task-2 verification, spec R3). A
	// felled log pushed at the mill rests ~8px short of the entrance x-range
	// (SW4 probe: log center 141 vs entrance [149,170]) and the PushTo ->
	// MoveTo-PushTarget loop spins forever, so the stack NEVER empties and
	// the watchdog above can't help. After >= 210 ticks with a felled,
	// ungrabbed, uncontained log within 80px of the mill, script-Enter it
	// into the mill (the saw chain then completes: ContainedUp saws it into
	// 7x WOOD and ejects them near the mill). Idempotent: a log entered
	// early is the same outcome as a completed push.
	if (g_at_clonk_saw && g_at_sawm)
	{
		var lg, have = 0;
		for (lg in FindObjects(Find_ID(TRE1)))
			if (!Contained(lg) && !lg->~IsStanding())
				if (Abs(GetX(lg) - GetX(g_at_sawm)) <= 80)
					have = 1;
		if (have) g_at_r3_ticks++; else g_at_r3_ticks = 0;
		if (have && g_at_r3_ticks >= 6) // 210 ticks at 35/effect
		{
			for (lg in FindObjects(Find_ID(TRE1)))
				if (!Contained(lg) && !lg->~IsStanding())
					if (Abs(GetX(lg) - GetX(g_at_sawm)) <= 80)
					{
						lg->Enter(g_at_sawm);
						g_at_r3_ticks = 0;
						++g_at_saw_assist;
						Log(Format("ATMT:saw_assist=%d", g_at_saw_assist));
						break;
					}
		}
	}
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

// ---- task-4 350-tick step ladder (plan Task 4; replaces the task-2 probe) --
// Global-effect ladder on a 35-tick interval (FirstLightClimate.c4s shape;
// strict-3 object slots take nil, not 0). Every timer fires
//   ATS:step=<timer> t=<time> <desc>
// every failure is
//   FatalError("AutoTasksSmoke FAIL: step <N> <what>")
// with <N> one of 1, 2a, 2b, 2c, 2d, 3, 4 — the FatalError tags are
// load-bearing: mutation M2's RED proof greps "step 2c" (plan 5e). An
// effect-timer FatalError exits 0 but logs "[error] User error: ..." which
// the smoke entry's FAIL regex catches (AGENTS.md smoke contract).

// Monotone reap-walk stage latches for the step-3 poll (plan adaptation 1:
// no stage_sickle — the committed JobReap has no Acquire legs, so the reap
// clonk never carries a sickle by construction).
static g_stage_plot, g_stage_sheaf;

protected func Initialize()
{
	ATFixtures();
	JobAssign(g_at_clonk_reap, "reap");
	JobAssign(g_at_clonk_saw, "saw");
	JobAssign(g_at_clonk_quarry, "quarry");
	// Strict-3 engine calls take nil (not 0) in object slots (C4AulExec
	// CheckOpPar). AddEffect args: name, target, prio, interval, cmdTarget, ...
	AddEffect("RunTest", nil, 1, 35, nil, 0);
	AddEffect("JobWatch", nil, 1, 35, nil, 0);
	return true;
}

global func FxRunTestStart(target, effect, temp) { return 1; }

global func FxRunTestTimer(target, effect, time)
{
	// ---- timer 1 (t35) — step 1: fixtures + live crew stacks. -------------
	if (time == 35)
	{
		Log("ATS:step=1 t=35 fixtures");
		if (!g_at_clonk_reap || !g_at_clonk_saw || !g_at_clonk_quarry)
			FatalError("AutoTasksSmoke FAIL: step 1 crew dead");
		// Stations present (ATFixtures pinned them at t0 — re-assert the
		// world they live in).
		if (!FindObject(GRNY) || !FindObject(SAWM) || !FindObject(QRRY)
		 || !FindObject(OASS))
			FatalError("AutoTasksSmoke FAIL: step 1 station missing");
		if (GetCon(FindObject(SAWM)) < 100)
			FatalError("AutoTasksSmoke FAIL: step 1 sawm con < 100");
		// 4 ripe wheat plots (IsRipe = action "Ready").
		var w, ripes = 0;
		for (w in FindObjects(Find_ID(AGWH)))
			if (w->IsRipe()) ripes++;
		if (ripes < 4)
			FatalError(Format("AutoTasksSmoke FAIL: step 1 ripes %d < 4", ripes));
		// >= 11 standing TRE1 (standing = IsStanding, Tree.c4d/Script.c:138).
		var t, standing = 0;
		for (t in FindObjects(Find_ID(TRE1)))
			if (t->~IsStanding()) standing++;
		if (standing < 11)
			FatalError(Format("AutoTasksSmoke FAIL: step 1 standing %d < 11", standing));
		// Each crew stack non-empty (the job's re-arm Call at the stack
		// bottom guarantees this while the loop lives).
		if (!GetCommand(g_at_clonk_reap, 0, 0)
		 || !GetCommand(g_at_clonk_saw, 0, 0)
		 || !GetCommand(g_at_clonk_quarry, 0, 0))
			FatalError("AutoTasksSmoke FAIL: step 1 empty crew stack");
		return true;
	}

	// ---- timer 2 (t70) — step 2: sync pins (a) mint, (b) quarry, (c) cap.
	if (time == 70)
	{
		Log("ATS:step=2 t=70 sync pins");

		// (2a) mint — Enter the reserved tree into the mill. Position-based
		// lookup (the fixture is the only TRE1 at AT_MINT_TREE_X — felled
		// logs rest near the mill, the R3 box is |x-150|<=80). Call form
		// verified against the engine reg (Enter, C4Script.cpp:7307) and the
		// shared-core R3 assist (lg->Enter(g_at_sawm)). Completion (sawing ->
		// WOOD ejected near the mill) is asserted at timer 7 (t245).
		var mint;
		for (mint in FindObjects(Find_ID(TRE1)))
			if (GetX(mint) == AT_MINT_TREE_X) break;
		if (!mint || GetX(mint) != AT_MINT_TREE_X)
			FatalError("AutoTasksSmoke FAIL: step 2a mint tree missing");
		if (!mint->Enter(g_at_sawm))
			FatalError("AutoTasksSmoke FAIL: step 2a Enter refused");
		Log("ATS:mint_entered");

		// (2b) quarry sync pin — DesertSmoke verbatim shape
		// (DesertSmoke.c4s/Script.c:125-133). Plan adaptation 3: the job's
		// t0 mint consumed the one-shot vein pixel ("vein exhausted" —
		// part-1 finding; Quarry() returns 0 after), so first re-draw the
		// exact quad ATFixtures painted, then Quarry() must mint again.
		DrawMaterialQuad("Sandstone", 680, g_at_gy + 4, 720, g_at_gy + 4,
		                 720, g_at_gy + 24, 680, g_at_gy + 24, false);
		if (!g_at_qrry->Quarry())
			FatalError("AutoTasksSmoke FAIL: step 2b Quarry() no mint");
		if (ContentsCount(SNDS, g_at_qrry) < 1)
			FatalError("AutoTasksSmoke FAIL: step 2b no SNDS in quarry");
		// Handoff pair to the quarry clonk + direct OASS deposit. The clonk
		// is mid-haul and may already carry the t0 SNDS — the final assert
		// holds for whichever SNDS got banked.
		var b = g_at_qrry->FindContents(SNDS);
		if (b) RemoveObject(b);
		CreateContents(SNDS, g_at_clonk_quarry, 1);
		g_at_oass->Deposit(g_at_clonk_quarry);
		if (ContentsCount(SNDS, g_at_oass) < 1)
			FatalError("AutoTasksSmoke FAIL: step 2b no SNDS at oasis");

		// (2c) cap — delta-fill GRNY to exactly 10 sheaves (the reap clonk
		// may have banked some already), hand the reap clonk a spare sheaf,
		// and prove Deposit is REFUSED. THIS IS MUTATION M2'S TARGET ASSERT
		// (plan 5e greps "AutoTasksSmoke FAIL: step 2c"). Holds in both
		// carry states: if the clonk already holds its own sheaf the
		// CreateContents is silently refused and the clonk-assert sees that
		// sheaf instead (CLNK RejectCollect caps non-special items at 1).
		CreateContents(AGSH, g_at_grny, 10 - ContentsCount(AGSH, g_at_grny));
		CreateContents(AGSH, g_at_clonk_reap, 1);
		g_at_grny->Deposit(g_at_clonk_reap);
		if (ContentsCount(AGSH, g_at_grny) != 10)
			FatalError(Format("AutoTasksSmoke FAIL: step 2c grny %d != 10 (cap broken)",
			                  ContentsCount(AGSH, g_at_grny)));
		if (ContentsCount(AGSH, g_at_clonk_reap) < 1)
			FatalError("AutoTasksSmoke FAIL: step 2c sheaf lost on refusal");
		// Non-vacuity (plan adaptation 2): drain what was just created so a
		// later step-4 "sheaves banked >= 1" cannot be satisfied by the
		// pre-fill. AGSH items are indistinguishable, so the drain removes
		// everything in GRNY including any real walked deposits — the cycle
		// pin at t315 then requires the reap loop to re-bank from scratch.
		var sh, i, n = ContentsCount(AGSH, g_at_grny);
		for (i = 0; i < n; i++)
		{
			sh = g_at_grny->FindContents(AGSH);
			if (sh) RemoveObject(sh);
		}
		Log("ATS:prefill_drained");
		return true;
	}

	// ---- timer 3 (t105) — step 2d: cancel the quarry job. ---------------
	if (time == 105)
	{
		Log("ATS:step=3 t=105 cancel quarry");
		// Capture before the cancel: JobCancel clears the crew static, so
		// GetCommand via the static would nil-test the wrong thing.
		var qc = g_at_clonk_quarry;
		g_at_qrry->ContextJobStop(g_at_clonk_quarry);
		if (GetCommand(qc, 0, 0))
			FatalError("AutoTasksSmoke FAIL: step 2d cancel left a command");
		// Cancel blast-radius pin: the other two loops must be untouched.
		if (!GetCommand(g_at_clonk_reap, 0, 0) || !GetCommand(g_at_clonk_saw, 0, 0))
			FatalError("AutoTasksSmoke FAIL: step 2d cancel hit another job");
		return true;
	}

	// ---- timers 4-8 (t140-280) — step 3: reap walk poll. -----------------
	if (time >= 140 && time <= 280)
	{
		Log(Format("ATS:step=%d t=%d reap poll", time / 35, time));
		// The reap loop must live every tick (re-arm Call at the stack
		// bottom; no Acquire legs to kill it).
		if (!GetCommand(g_at_clonk_reap, 0, 0))
			FatalError("AutoTasksSmoke FAIL: step 3 reap stack died");
		// Monotone stage latches (checked cumulatively at timer 8). No
		// stage_sickle — plan adaptation 1: JobReap has no Acquire legs, so
		// the reap clonk never carries a sickle by construction.
		var w, ripes = 0;
		for (w in FindObjects(Find_ID(AGWH)))
			if (w->IsRipe()) ripes++;
		if (ripes < 4) g_stage_plot = true;          // a plot got harvested
		if (ContentsCount(AGSH, g_at_clonk_reap) >= 1) g_stage_sheaf = true; // sheaf on the clonk observed
		// timer 7 (t245): 2a completion — the minted tree was sawed and
		// ejected near the mill (ContentsCheck 35 + Saw 35 pacing).
		if (time == 245)
			if (ATWoodCensus() < 1)
				FatalError(Format("AutoTasksSmoke FAIL: step 2a census %d", ATWoodCensus()));
		// timer 8 (t280): all remaining stage latches set.
		if (time == 280)
		{
			if (!g_stage_plot)
				FatalError("AutoTasksSmoke FAIL: step 3 no plot harvested");
			if (!g_stage_sheaf)
				FatalError("AutoTasksSmoke FAIL: step 3 no sheaf carried");
		}
		return true;
	}

	// ---- timer 9 (t315) — step 4: cycle pin. -----------------------------
	if (time == 315)
	{
		Log("ATS:step=9 t=315 cycle pin");
		// Non-vacuous after the 2c drain: the reap counter bumps only on
		// real walked deposits (this one ran at least twice before t70) and
		// a real post-drain deposit must have re-banked a sheaf by now.
		if (g_at_reap_deposits < 1)
			FatalError("AutoTasksSmoke FAIL: step 4 no reap deposit counted");
		if (ContentsCount(AGSH, g_at_grny) < 1)
			FatalError("AutoTasksSmoke FAIL: step 4 no sheaf banked after drain");
		Log("AutoTasksSmoke PASS");
		GameOver();
		return true;
	}
	return true;
}
