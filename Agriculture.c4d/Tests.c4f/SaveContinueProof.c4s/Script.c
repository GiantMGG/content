/*-- SaveContinueProof.c4s — save-continue two-phase E2E fixture (cycle 189).
   Own fixture (NOT a mirror twin of AutoTasksSmoke/AutoTasksAccept — the
   plan's save-continue Task 4 fixture derives from their SHARED-CORE; no
   mirror-drift obligation). Two-phase contract:
     Phase A (--smoke-run 700 --smoke-save-at:700:SCProof): fresh-run health
     at t35, pre-save marks at t665 (g_scp_mark_*), engine QuickSaves at
     t700 and caps out — the PASS ladder can never fire in phase A.
     Phase B (--smoke-run 2800, resume the savegame): the effect timer
     continues at t~700; t805 + t1610 drain the granary (unblock its 10-sheaf
     cap), t1260 asserts the marks/g_scp_* statics restored and jobs RESUME
     (stacks live, tallies > marks), t2380 asserts continued growth past the
     t1260 snapshots, then SaveContinueProof PASS.
     Calibration (scratch/189/task4-calibration.md): the plan's t1085/t1365
     windows assumed cycle-180 saw pacing, but at seed 181 the original
     right-cluster tree x=194 (approach 188, 2px from the mill hull x=186)
     wedges the saw for ~1,500 ticks — the class the shared core's own
     re-lay comment documents. The forest is re-laid to [210..282] and the
     mint moved to 300 (approaches >= 204 > 186); saw cycles then land at
     ~t560/~t1120/~t2080, so the resume arm sits at t1260 and the continue
     arm at t2380 (phase B cap 2800). Every window holds >= 1 saw claim with
     >= 130-tick margin; reap + quarry margins are 4-10+.
   Every failure is FatalError("SaveContinueProof FAIL: step <tag> ...") —
   inside an effect timer it exits 0 but logs [error], which the stanza's
   FAIL regex catches (AGENTS.md smoke contract). The SHARED-CORE's own
   FatalError strings were re-prefixed to SaveContinueProof for log-search
   consistency (the twin scripts keep their own prefix).
   Upkeep: the SHARED-CORE world stalls without fixture maintenance (one-shot
   quarry vein, capped granary, finite forest) — the keepers below mirror the
   AutoTasksAccept precedent (wheat/vein keeper) and add a forest keeper +
   granary drain so the three job tallies keep GROWING across both phase-B
   windows (strict-growth proof, spec §5). --*/

#strict 3

// SHARED-CORE-BEGIN -----------------------------------------------------------
// Station constants (world px; provisional per spec — the surface y is
// scan-derived, everything else anchors to it).

static const AT_SAWM_X = 150;        // sawmill station
static const AT_MINT_TREE_X = 300;   // reserved mint tree (pre-marked; moved to 300 by this fixture's calibration — the shared core's 284 sat 2px from the re-laid right cluster)
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
static g_at_saw_delivers;                          // saw-log deliveries into the mill (AMENDMENT 1)

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
			FatalError(Format("SaveContinueProof FAIL: surface y=%d at x=%d outside [%d,%d]",
			                  gy, xs[i], AT_SURF_MIN, AT_SURF_MAX));
		if (gy < ymin) ymin = gy;
		if (gy > ymax) ymax = gy;
		if (xs[i] == AT_GRNY_X) gm = gy;
	}
	if (ymax - ymin > AT_SURF_SPREAD)
		FatalError(Format("SaveContinueProof FAIL: surface spread %d > %d",
		                  ymax - ymin, AT_SURF_SPREAD));
	g_at_gy = gm; // pinned surface (x=450 GRNY station)

	// 2. Material pin: the plate under the quarry station is Earth.
	if (GetMaterial(700, g_at_gy + 5) != Material("Earth"))
		FatalError(Format("SaveContinueProof FAIL: material at (700,%d+5) is not Earth",
		                  g_at_gy));
	Log("ATFIX:mat700 ok");

	// 3. Sawmill — complete build (Pearls.c4s/Script.c:38 shape; SAWM graphic
	//    bottom at y+28, DefCore Offset=-36,-27 Height=55).
	gy = ATSurfaceY(AT_SAWM_X);
	g_at_sawm = CreateConstruction(SAWM, AT_SAWM_X, gy - 28, NO_OWNER, 100, 1);
	if (!g_at_sawm || GetCon(g_at_sawm) < 100)
		FatalError("SaveContinueProof FAIL: SAWM not built to con>=100");
	Log(Format("ATFIX:sawm con=%d", GetCon(g_at_sawm)));

	// 4. Forest: 10 TRE1, WIDE spacing (originally 9px — measured false
	//    premise, task-2 probe: a fallen log being pushed through a 9px-spaced
	//    forest wedges against standing trunks and the PushTo stalls forever
	//    at ~126px). 18-20px gaps leave ~12-14px trunk clearance for pushed
	//    logs. CALIBRATION (save-continue, scratch/189/task4-calibration.md):
	//    the right cluster is re-laid to x=210..282 (approaches >= 204) — the
	//    shared core's x=194 approach (188) wedges the saw for ~1,500 ticks at
	//    seed 181, and the mint moves to 300 alongside.
	var at_forest = [30, 48, 66, 84, 102, 210, 228, 246, 264, 282];
	for (i = 0; i < GetLength(at_forest); i++)
	{
		var tree = CreateObject(TRE1, at_forest[i], g_at_gy, NO_OWNER);
		if (!tree) FatalError(Format("SaveContinueProof FAIL: forest tree %d", i));
	}

	// 5. Reserved mint tree + pre-claim (Sawmill :58 shape: the marker makes
	//    FindTreeToChop skip it, so the saw job never touches the mint fixture).
	//    Task-2: moved from x=185 (trunk overlapped the SAWM footprint) to
	//    x=300, clear right of the relayed right forest cluster.
	var mint = CreateObject(TRE1, AT_MINT_TREE_X, g_at_gy, NO_OWNER);
	if (!mint) FatalError("SaveContinueProof FAIL: mint tree");
	AddEffect("IntSawmillTreeMarker", mint, 1, 5000, g_at_sawm, 0, 0);

	// 6. Tree count pin (after both spawns — single unambiguous line).
	if (ObjectCount(TRE1) != 11)
		FatalError(Format("SaveContinueProof FAIL: TRE1 count %d != 11", ObjectCount(TRE1)));
	Log("ATFIX:trees n=11");

	// 7. Granary (donor geometry: bottom at y+0).
	g_at_grny = CreateObject(GRNY, AT_GRNY_X, g_at_gy, NO_OWNER);
	if (!g_at_grny) FatalError("SaveContinueProof FAIL: GRNY not spawned");
	Log("ATFIX:grny ok");

	// 8. Wheat: 4 ripe plots (Construction forces Seedling — re-set after
	//    full-con spawn, EventSmoke idiom).
	AT_FIELD_PLOTS = [380, 410, 470, 520];
	for (i = 0; i < GetLength(AT_FIELD_PLOTS); i++)
	{
		var wheat = CreateObject(AGWH, AT_FIELD_PLOTS[i], g_at_gy, NO_OWNER);
		if (!wheat) FatalError(Format("SaveContinueProof FAIL: wheat plot %d", i));
		wheat->SetAction("Ready");
		if (!wheat->IsRipe())
			FatalError(Format("SaveContinueProof FAIL: wheat %d not ripe", i));
	}
	Log("ATFIX:wheat n=4");

	// 9. Loose sickles at x 620/640 (drop to the surface). Task-2 fix: the
	//    JobReap first leg (MoveTo GRNY 450) passed over the original x=420/440
	//    spawns, the unowned clonk auto-picked a sickle, and CLNK
	//    RejectCollect (MaxContentsCount 1) then refused the sheaves — the
	//    carried=0 banked=0 shape. BI6 proved 620/640 restores banked=4.
	var sk1 = CreateObject(AGSK, 620, g_at_gy - 10, NO_OWNER);
	var sk2 = CreateObject(AGSK, 640, g_at_gy - 10, NO_OWNER);
	if (!sk1 || !sk2) FatalError("SaveContinueProof FAIL: sickles missing");
	Log("ATFIX:sickles n=2");

	// 10. Sandstone vein (40 wide x 20 deep = 800 px^2 ~ 400 extractions) then
	//     the quarry (bottom at y+0; probe (0,4) hits gy+4 = vein top —
	//     DesertSmoke's exact relative geometry).
	DrawMaterialQuad("Sandstone", 680, g_at_gy + 4, 720, g_at_gy + 4,
	                 720, g_at_gy + 24, 680, g_at_gy + 24, false);
	g_at_qrry = CreateObject(QRRY, AT_QRRY_X, g_at_gy, NO_OWNER);
	if (!g_at_qrry) FatalError("SaveContinueProof FAIL: QRRY not spawned");
	Log("ATFIX:qrry ok");

	// 11. Oasis (24x8, bottom at y+0; Construction carves the basin, Fill waters
	//     it — a pool the hauler must NOT wade: Task 2's MoveTo targets x=925).
	g_at_oass = CreateObject(OASS, AT_OASS_X, g_at_gy, NO_OWNER);
	if (!g_at_oass) FatalError("SaveContinueProof FAIL: OASS not spawned");
	Log("ATFIX:oass ok");

	// 12. Crew: one unowned CLNK per job (SpawnerSmoke.c4s/Script.c:22 shape).
	//     Task-2: the saw clonk moved from x=170 (inside the SAWM footprint
	//     [114,186] — it spawned embedded in the mill) to x=300, clear of the
	//     mill and both forest clusters.
	g_at_clonk_reap = CreateObject(CLNK, 400, g_at_gy - 8, NO_OWNER);
	g_at_clonk_saw = CreateObject(CLNK, 300, g_at_gy - 8, NO_OWNER);
	g_at_clonk_quarry = CreateObject(CLNK, 660, g_at_gy - 8, NO_OWNER);
	if (!g_at_clonk_reap || !g_at_clonk_saw || !g_at_clonk_quarry)
		FatalError("SaveContinueProof FAIL: crew not spawned");
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
	// (AMENDMENT 1: the R3 wedged-log entrance assist was removed — its wedge
	// (stock PushTo -> MoveTo-PushTarget into the mill hull) cannot occur
	// without the Production PushTo leg, which JobSaw no longer issues.)
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

// ---- save-continue ladder + upkeep (see the header contract) --------------
// g_scp_* statics: the phase-A pre-save marks and the phase-B snapshots. The
// g_scp_ prefix is this cycle's collision guard (statics are one engine-wide
// namespace; the g_at_ prefix guard is the cycle-180 precedent). The marks
// MUST ride Game.txt into the savegame — the t1085 arm proves the restore.
static g_scp_mark_reap, g_scp_mark_saw, g_scp_mark_quarry;
static g_scp_marks_set;
static g_scp_seen_reap, g_scp_seen_saw, g_scp_seen_quarry;

// Keeper cursors (typed int before any arithmetic — the strict-3 "%" gotcha).
static g_wheat_cursor, g_forest_cursor;

// Rotating forest replant row (same 10 positions as ATFixtures step 4).
static g_forest_rows;

protected func Initialize()
{
	ATFixtures();
	JobAssign(g_at_clonk_reap, "reap");
	JobAssign(g_at_clonk_saw, "saw");
	JobAssign(g_at_clonk_quarry, "quarry");
	// Strict-3 engine calls take nil (not 0) in object slots (C4AulExec
	// CheckOpPar). AddEffect args: name, target, prio, interval, cmdTarget, ...
	g_wheat_cursor = 0;
	g_forest_cursor = 0;
	g_forest_rows = [30, 48, 66, 84, 102, 194, 212, 230, 248, 266];
	AddEffect("RunTest", nil, 1, 35, nil, 0);
	AddEffect("JobWatch", nil, 1, 35, nil, 0);
	return true;
}

global func FxRunTestStart(target, effect, temp) { return 1; }

// Drain the granary's AGSH contents — unblocks the Deposit cap (GRNY accepts
// only 10 sheaves; once full, g_at_reap_deposits freezes). The smoke 2c drain
// idiom. Without this the reap tally cannot keep growing after ~t245.
global func SCPDrainGranary()
{
	var sh, i, n = ContentsCount(AGSH, g_at_grny);
	for (i = 0; i < n; i++)
	{
		sh = g_at_grny->FindContents(AGSH);
		if (sh) RemoveObject(sh);
	}
	Log(Format("SCP:drain n=%d", n));
}

// Upkeep keepers (every timer): the SHARED-CORE world is one-shot — the
// quarry vein exhausts after a few mints, the wheat field disappears once
// harvested, and the forest shrinks as the saw fells trees. AutoTasksAccept
// proved the wheat + vein keeper patterns; the forest keeper is the saw-side
// analog (re-grow a tree whenever standing count drops, so JobSaw always has
// a claimable TRE1). All three keep the job tallies growing across BOTH
// phase-B windows — the strict > proof those windows make.
global func SCPKeepers()
{
	// Wheat keeper: field stays >= 4 ripe (EventSmoke SetAction idiom;
	// AutoTasksAccept FxRunAcceptTimer shape).
	var w, ripes = 0;
	for (w in FindObjects(Find_ID(AGWH)))
		if (w->IsRipe()) ripes++;
	if (ripes < 4)
	{
		var plot_x = AT_FIELD_PLOTS[g_wheat_cursor % GetLength(AT_FIELD_PLOTS)];
		g_wheat_cursor++;
		w = CreateObject(AGWH, plot_x, g_at_gy, NO_OWNER);
		if (w) w->SetAction("Ready");
	}
	// Forest keeper: standing TRE1 < 8 -> replant one at the rotating row
	// (standing = the saw's claim pool; the mint tree is marked, never
	// claimed, and always standing).
	var t, standing = 0;
	for (t in FindObjects(Find_ID(TRE1)))
		if (t->~IsStanding()) standing++;
	if (standing < 8)
	{
		var tree_x = g_forest_rows[g_forest_cursor % GetLength(g_forest_rows)];
		g_forest_cursor++;
		if (!CreateObject(TRE1, tree_x, g_at_gy, NO_OWNER))
			FatalError("SaveContinueProof FAIL: step keeper forest spawn");
	}
	// Vein keeper: re-draw the exact ATFixtures quad EVERY timer — the
	// quarry's Quarry() consumes the one-shot probe cell per mint; a
	// sustained haul loop needs the cell restored (AutoTasksAccept re-draw
	// precedent, denser cadence for the shorter windows).
	DrawMaterialQuad("Sandstone", 680, g_at_gy + 4, 720, g_at_gy + 4,
	                 720, g_at_gy + 24, 680, g_at_gy + 24, false);
}

global func FxRunTestTimer(target, effect, time)
{
	// Upkeep first (every timer).
	SCPKeepers();

	// ---- phase-A fresh-run (t35): crew + stations + live stacks. The
	// effect resumes at t~700 in phase B, so this arm can only fire in A.
	if (time == 35)
	{
		Log("SCP:step=phaseA t=35 fixtures");
		if (!g_at_clonk_reap || !g_at_clonk_saw || !g_at_clonk_quarry)
			FatalError("SaveContinueProof FAIL: step phaseA crew dead");
		if (!FindObject(GRNY) || !FindObject(SAWM) || !FindObject(QRRY)
		 || !FindObject(OASS))
			FatalError("SaveContinueProof FAIL: step phaseA station missing");
		if (GetCon(FindObject(SAWM)) < 100)
			FatalError("SaveContinueProof FAIL: step phaseA sawm con < 100");
		// Each crew stack non-empty (the job's re-arm Call at the stack
		// bottom guarantees this while the loop lives).
		if (!GetCommand(g_at_clonk_reap, 0, 0)
		 || !GetCommand(g_at_clonk_saw, 0, 0)
		 || !GetCommand(g_at_clonk_quarry, 0, 0))
			FatalError("SaveContinueProof FAIL: step phaseA empty crew stack");
		return true;
	}

	// ---- granary drains (phase B): unblock the Deposit cap so the reap
	// tally keeps growing across both proof windows. t805 sits ~105 ticks
	// after the resume (~t700); t1610 re-drains between the snapshots.
	if (time == 805 || time == 1610)
	{
		SCPDrainGranary();
		return true;
	}

	// ---- phase-A pre-save marks (t665, 35 ticks before the t700 save):
	// copy the live tallies into the g_scp_ statics that must ride Game.txt
	// into the savegame.
	if (time == 665)
	{
		g_scp_mark_reap = g_at_reap_deposits;
		g_scp_mark_saw = g_at_saw_cycles;
		g_scp_mark_quarry = g_at_quarry_cycles;
		g_scp_marks_set = 1;
		Log(Format("SCP:marks reap=%d saw=%d quarry=%d",
		           g_scp_mark_reap, g_scp_mark_saw, g_scp_mark_quarry));
		return true;
	}

	// ---- phase-B resume health (t1260, ~560 ticks post-resume): the marks
	// restored, crew + stations intact, stacks live, every tally strictly
	// greater than its pre-save mark (jobs kept running across the save).
	// The window holds the saw's cycle-3 claim (~t1120) with ~140 ticks of
	// margin; reap + quarry margins are 4-10+ (calibration md).
	if (time == 1260)
	{
		Log("SCP:step=resume t=1260");
		if (g_scp_marks_set != 1)
			FatalError("SaveContinueProof FAIL: step resume marks not restored");
		if (!g_at_clonk_reap || !g_at_clonk_saw || !g_at_clonk_quarry)
			FatalError("SaveContinueProof FAIL: step resume crew dead");
		if (!FindObject(GRNY) || !FindObject(SAWM) || !FindObject(QRRY)
		 || !FindObject(OASS))
			FatalError("SaveContinueProof FAIL: step resume station missing");
		if (!GetCommand(g_at_clonk_reap, 0, 0)
		 || !GetCommand(g_at_clonk_saw, 0, 0)
		 || !GetCommand(g_at_clonk_quarry, 0, 0))
			FatalError("SaveContinueProof FAIL: step resume empty crew stack");
		if (g_at_reap_deposits <= g_scp_mark_reap)
			FatalError(Format("SaveContinueProof FAIL: step resume reap %d <= mark %d",
			                  g_at_reap_deposits, g_scp_mark_reap));
		if (g_at_saw_cycles <= g_scp_mark_saw)
			FatalError(Format("SaveContinueProof FAIL: step resume saw %d <= mark %d",
			                  g_at_saw_cycles, g_scp_mark_saw));
		if (g_at_quarry_cycles <= g_scp_mark_quarry)
			FatalError(Format("SaveContinueProof FAIL: step resume quarry %d <= mark %d",
			                  g_at_quarry_cycles, g_scp_mark_quarry));
		g_scp_seen_reap = g_at_reap_deposits;
		g_scp_seen_saw = g_at_saw_cycles;
		g_scp_seen_quarry = g_at_quarry_cycles;
		Log(Format("SCP:resumed reap=%d saw=%d quarry=%d",
		           g_scp_seen_reap, g_scp_seen_saw, g_scp_seen_quarry));
		return true;
	}

	// ---- phase-B continued growth (t2380, ~1120 ticks past the snapshots):
	// tallies strictly greater than the t1260 snapshots — the jobs resume
	// AND keep producing (not one lucky restored tick). The window holds
	// the saw's cycle-4 claim (~t2080) with ~300 ticks of margin. PASS.
	if (time == 2380)
	{
		Log("SCP:step=continue t=2380");
		if (g_at_reap_deposits <= g_scp_seen_reap)
			FatalError(Format("SaveContinueProof FAIL: step continue reap %d <= seen %d",
			                  g_at_reap_deposits, g_scp_seen_reap));
		if (g_at_saw_cycles <= g_scp_seen_saw)
			FatalError(Format("SaveContinueProof FAIL: step continue saw %d <= seen %d",
			                  g_at_saw_cycles, g_scp_seen_saw));
		if (g_at_quarry_cycles <= g_scp_seen_quarry)
			FatalError(Format("SaveContinueProof FAIL: step continue quarry %d <= seen %d",
			                  g_at_quarry_cycles, g_scp_seen_quarry));
		Log("SaveContinueProof PASS");
		GameOver();
		return true;
	}
	return true;
}
