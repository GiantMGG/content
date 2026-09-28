/*-- System.c4g/OasisDeposit.c — OASS Deposit for the quarry haul job (plan
   auto-tasks-mvp 2d; spec "OASS Deposit" as a pack edit — the appendto keeps
   Desert.c4d untouched: same mechanism, smaller blast radius [flagged]).
   #appendto OASS — PreventChop.c:4 mechanism. Byte-identical mirror twin in
   AutoTasksAccept.c4s/System.c4g — keep in sync.
   No ContextJobStop needed here: the quarry job's cancel menu lives on the
   QRRY side per spec. --*/

#strict
#appendto OASS

/* Headless banking pair (Sturmfront bank pair) for the carried SNDS. The
   quarry clonk arrives at the approach point AT_OASS_X - 25 (never wades
   the basin), and this Call deposits the block across the last 25 px. */
public func Deposit(object clonk)
{
	var b = clonk->FindContents(SNDS);
	if (!b) return 0;
	RemoveObject(b);
	CreateContents(SNDS, this, 1);
	++g_at_oasis_deposits;
	Log(Format("ATMT:quarry_deposit=%d", g_at_oasis_deposits));
	return 1;
}
