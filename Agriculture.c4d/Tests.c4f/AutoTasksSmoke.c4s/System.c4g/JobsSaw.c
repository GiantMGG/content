/*-- System.c4g/JobsSaw.c — standing Saw job on the stock Sawmill (plan
   auto-tasks-mvp 2b; spec decision row B). Scenario System.c4g #appendto —
   the DM_Baldoon PreventChop mechanism (PreventChop.c:4 shape). Byte-identical
   mirror twin in AutoTasksAccept.c4s/System.c4g — keep in sync.
   The counter statics (g_at_saw_* ) are scenario statics declared in the
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

/* The stock ProductionLoop (Sawmill.c4d/Script.c:152-158 shape) with
   bookkeeping: re-arm first at the stack bottom, then Production issues the
   Chop/PushTo work on top. Production returns 1 iff a tree was claimed —
   cycles counts re-arms, trees counts successful claims. No shadowing of the
   stock funcs; the appendto only adds. The re-arm line is mutation M1's
   target. */
public func JobSaw(object clonk)
{
	AppendCommand(clonk, "Call", this(), 0, 0, 0, 0, "JobSaw");
	if (Production(clonk))
	{
		++g_at_saw_cycles;
		++g_at_saw_trees;
		Log(Format("JobSaw:cycles=%d", g_at_saw_cycles));
		Log(Format("JobSaw:trees=%d", g_at_saw_trees));
	}
	return 1;
}
