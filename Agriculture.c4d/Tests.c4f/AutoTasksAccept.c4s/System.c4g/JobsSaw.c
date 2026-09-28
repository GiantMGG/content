/*-- System.c4g/JobsSaw.c — standing Saw job on the stock Sawmill (plan
   auto-tasks-mvp 2b as amended by AMENDMENT 1; spec decision row B).
   Scenario System.c4g #appendto — the DM_Baldoon PreventChop mechanism
   (PreventChop.c:4 shape). Byte-identical mirror twin in
   AutoTasksAccept.c4s/System.c4g — keep in sync.
   AMENDMENT 1 (primary decision after the Task-5 stop, 2026-09-28): the
   saw job is self-contained — it claims its own tree and issues its own
   Chop -> MoveTo-approach -> SawDeliver leg chain, never calling stock
   Production (whose PushTo leg wedges permanently on the flat-plate
   fixture; scratch/180/task5-findings.md). The re-arm line below remains
   the loop's ONLY sustainer and mutation M1's target.
   The counter statics (g_at_saw_*) are scenario statics declared in the
   scenario Script.c shared core (engine-wide g_at_ namespace). --*/

#strict
#appendto SAWM

/* Menu face — the shared JobAssign core (menus and scripted runs both go
   through it). */
public func ContextJobSaw(object clonk)
{
	[$Job: Saw$|Image=WOOD|Condition=IsBuilt]
	return JobAssign(clonk, "saw");
}

public func ContextJobStop(object clonk)
{
	[$Job: Stop$|Image=WOOD]
	return JobCancel(clonk);
}

/* Deliver the felled log into the mill (AMENDMENT 1). Fired as the LAST leg
   of the saw chain via AddCommand(clonk, "Call", g_at_sawm, ..., "SawDeliver").
   Finds the felled, uncontained, non-standing TRE1 nearest the mill within
   250px (Manhattan) and Enters it; the mill's ContentsCheck then saws it into
   7x WOOD and ejects them near the mill — the committed R3 assist and the
   smoke 2a mint both proved this exact Enter. Moves only the LOG, never
   touches the clonk's command stack, so M1-safety holds: the re-arm remains
   the loop's only sustainer. */
public func SawDeliver(object clonk)
{
	var lg, best, best_d = 250;
	for (lg in FindObjects(Find_ID(TRE1)))
		if (!Contained(lg) && !lg->~IsStanding())
		{
			var d = Abs(GetX(lg) - GetX()) + Abs(GetY(lg) - GetY());
			if (d <= best_d)
			{
				best_d = d;
				best = lg;
			}
		}
	if (!best) return 0;
	best->Enter(this());
	Log(Format("ATMT:saw_deliver=%d", ++g_at_saw_delivers));
	return 1;
}

/* The saw loop — AMENDMENT 1 self-contained chain. Re-arm first at the stack
   bottom, then claim the nearest standing, unmarked TRE1 within 250px of the
   mill and issue the work legs in REVERSE (each AddCommand lands on top;
   execution order Chop -> MoveTo-approach -> Call-SawDeliver):
     1. AddCommand "Call" SawDeliver   (runs LAST of the three)
     2. AddCommand "MoveTo" approach   (runs second; approach point on the
        same side of the mill as the tree, clear of the mill hull x=[114,186]:
        AT_SAWM_X-45 = 105 (left) or AT_SAWM_X+50 = 200 (right). C4Aul has
        no ternary — the if/else below)
     3. AddCommand "Chop" tree         (issued last = runs FIRST)
   "cycles" counts re-arms-with-a-claim, "trees" counts claims — one tree
   per cycle, the two Log lines are load-bearing (calibration greps them).
   The re-arm line is mutation M1's target. */
public func JobSaw(object clonk)
{
	AppendCommand(clonk, "Call", this(), 0, 0, 0, 0, "JobSaw");
	var tree, best, best_d = 250;
	for (tree in FindObjects(Find_ID(TRE1)))
		if (!Contained(tree) && tree->~IsStanding())
			if (GetEffectCount("IntSawmillTreeMarker", tree) == 0)
			{
				var d = Abs(GetX(tree) - GetX()) + Abs(GetY(tree) - GetY());
				if (d <= best_d)
				{
					best_d = d;
					best = tree;
				}
			}
	if (!best) return 1;
	// Claim: mark the tree so no cycle double-claims it (Sawmill :58 shape).
	AddEffect("IntSawmillTreeMarker", best, 1, 5000, this(), 0, 0);
	++g_at_saw_cycles;
	++g_at_saw_trees;
	Log(Format("JobSaw:cycles=%d", g_at_saw_cycles));
	Log(Format("JobSaw:trees=%d", g_at_saw_trees));
	// Approach point on the same side of the mill as the tree (no ternary
	// in C4Aul): left of the mill -> 105, right -> 200, both clear of the
	// mill hull x=[114,186].
	var at_ax;
	if (GetX(best) < AT_SAWM_X) at_ax = AT_SAWM_X - 45;
	else at_ax = AT_SAWM_X + 50;
	AddCommand(clonk, "Call", g_at_sawm, 0, 0, 0, 0, "SawDeliver");
	AddCommand(clonk, "MoveTo", 0, at_ax, g_at_gy);
	AddCommand(clonk, "Chop", best);
	return 1;
}
