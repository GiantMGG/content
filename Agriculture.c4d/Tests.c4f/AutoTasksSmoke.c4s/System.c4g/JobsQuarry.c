/*-- System.c4g/JobsQuarry.c — standing Quarry-haul job on the stock QRRY
   (plan auto-tasks-mvp 2c; spec decision row C). #appendto QRRY —
   PreventChop.c:4 mechanism. Byte-identical mirror twin in
   AutoTasksAccept.c4s/System.c4g — keep in sync.
   The SNDT gate ships dormant: no [WeatherEvents] block exists in the
   scenarios, so GetActiveWeatherEvent() is nil (engine-nil-safe without
   WeatherEvents loaded, C4Script.cpp:3346-48). The stone is handed to the
   clonk with the Sturmfront bank pair (quarry->carried), then walked to the
   Oasis and deposited via its appended Deposit (OasisDeposit.c). --*/

#strict
#appendto QRRY

/* Menu face — the shared JobAssign core. */
public func ContextJobQuarryHaul(object clonk)
{
	[$Job: Quarry Haul$|Image=SNDS]
	return JobAssign(clonk, "quarry");
}

public func ContextJobStop(object clonk)
{
	[$Job: Stop$|Image=SNDS]
	return JobCancel(clonk);
}

/* The quarry haul loop. The mint is a DIRECT script call (script calls DO
   return, unlike Call commands — C4Command.cpp:2369-86 never propagates).
   On exhaustion (vein empty => Quarry() logs and returns 0,
   Quarry.c4d/Script.c:22-26) the loop idles: the re-arm already appended at
   the stack bottom, so the loop polls the vein cheaply without issuing more
   work legs. */
public func JobQuarry(object clonk)
{
	// SNDT gate — no weather here, dormant one-liner (spec Risk R6).
	if (GetActiveWeatherEvent() == SNDT) return 1;
	// Re-arm at the stack bottom (ProductionLoop.c4d/Script.c:154 shape).
	AppendCommand(clonk, "Call", this(), 0, 0, 0, 0, "JobQuarry");
	if (!Quarry())
	{
		Log("ATMT:quarry_idle");
		return 1;
	}
	// Minted: hand the SNDS to the clonk (Sturmfront bank pair, headless
	// container-Put is broken) and walk it to the Oasis approach point —
	// 25 px short of the basin, which fills with Water (do not wade).
	// The pair is issued CALL-then-MoveTo (AddCommand puts each new command
	// on top = executed first) so the walk precedes the deposit.
	// NOTE: plain #strict here — object slots take 0, not nil (strict-3 only).
	var b = FindContents(SNDS);
	if (b) RemoveObject(b);
	CreateContents(SNDS, clonk, 1);
	AddCommand(clonk, "Call", g_at_oass, 0, 0, 0, 0, "Deposit");
	AddCommand(clonk, "MoveTo", 0, AT_OASS_X - 25, g_at_gy);
	++g_at_quarry_cycles;
	return 1;
}
