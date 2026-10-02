/*-- Granary.c4d — scenario-local granary def (AGWM donor sprite), shared by
   the AutoTasksSmoke/AutoTasksAccept scenario pair (plan R4). Byte-identical
   mirror twin in the other scenario dir — keep in sync. Task 1: the
   RejectCollect face. Task 2: the reap job funcs (Deposit + menu contexts).
   The reap-side stats (g_at_reap_deposits etc.) are scenario statics — see
   the scenario Script.c shared core. --*/

#strict 3

/* Menu fleet-condition parity: GRNY spawns complete via CreateObject, so this
   is trivially true (Sawmill.c4d/Script.c:140-142 shape). */
protected func IsBuilt()
{
	return GetCon() >= 100;
}

/* Windmill.c4d/Script.c:91-99 shape, cap raised to 10. This is the
   engine-collect face; the headless script path is capped in Deposit (Task 2).
   Accept AGSH/AGAP while under their caps, reject everything else. */
protected func RejectCollect(id def, object obj)
{
	if (def == AGSH)
	{
		if (ContentsCount(AGSH) < 10) return(0);
		return(1);
	}
	if (def == AGAP)
	{
		if (ContentsCount(AGAP) < 10) return(0);
		return(1);
	}
	return(1);
}

/* Job core, reap row (spec decision table row A). Deposit is the headless
   banking pair (Sturmfront bank pair): RemoveObject the carried sheaf +
   CreateContents into this granary. Script-side cap mirrors RejectCollect's
   cap-10: no room, the sheaf stays carried and we return 0 (the smoke's cap
   pin asserts exactly this refusal). */
public func Deposit(object clonk)
{
	if (ContentsCount(AGSH) + ContentsCount(AGAP) >= 10) return 0;
	var sheaf = clonk->FindContents(AGSH);
	if (!sheaf) return 0;
	RemoveObject(sheaf);
	CreateContents(AGSH, this, 1);
	++g_at_reap_deposits;
	Log(Format("ATMT:reap_deposit=%d", g_at_reap_deposits));
	return 1;
}

/* Menu face -> the shared JobAssign core (menus and scripted runs both go
   through it; one job per clonk is enforced there). */
public func ContextJobReap(object clonk)
{
	[$Job: Reap$|Image=AGSH|Condition=IsBuilt]
	return JobAssign(clonk, "reap");
}

public func ContextJobStop(object clonk)
{
	[$Job: Stop$|Image=AGSH]
	return JobCancel(clonk);
}

/* The reap loop: re-arm at the stack bottom (Sawmill :154 shape), then issue
   the work legs. AddCommand puts each new command on TOP (executed first), so
   each leg pair is issued CALL-then-MoveTo — the walk precedes the operation
   and the chain is genuinely walked. Intended top-down order:
     [MoveTo ripes AGWH -> Call Harvest] -> [MoveTo GRNY -> Call Deposit]
     -> (re-arm).
   NO Acquire legs: a failed empty Acquire falls into a Buy sub-command whose
   C4CMD_Mode_Sub failure propagates to the re-arm Call below and kills the
   whole loop (measured in the task-2 probe; harvest needs no sickle — the
   Wheat Harvest has no tool check, Wheat.c4d/Script.c:46-60 — and the sheaves
   enter the clonk via Collect, so the legs carry no value). */
public func JobReap(object clonk)
{
	AppendCommand(clonk, "Call", this(), 0, 0, nil, 0, "JobReap");
	AddCommand(clonk, "Call", this(), 0, 0, nil, 0, "Deposit");
	AddCommand(clonk, "MoveTo", nil, GetX(this()), GetY(this()));
	var w;
	for (w in FindObjects(Find_ID(AGWH), Find_Distance(200), Sort_Distance()))
		if (w->IsRipe())
		{
			AddCommand(clonk, "Call", w, 0, 0, nil, 0, "Harvest");
			AddCommand(clonk, "MoveTo", nil, GetX(w), GetY(w));
			break;
		}
	return 1;
}
