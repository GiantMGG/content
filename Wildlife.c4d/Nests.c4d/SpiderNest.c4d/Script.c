/*-- Spider Nest -- ZapNest-style spawner for SPDR spiderlings. --*/
/* Uses the WLF_NestSpawner behaviour from WLFA.                   */

#strict 3

#include WLFA

protected func Initialize()
{
	SetAction("Idle");
	// opts: [Interval, SpawnID, Cap]. See WLFA.
	WLFA_AddBehavior(this, "NestSpawner", [210, SPDR, 6]);
	return true;
}

// Convenience: the nest's own TimerCall (DefCore TimerCall=NestTick) is
// redundant with the WLF_NestSpawner effect timer but kept as a no-op
// anchor so scenario authors can override it.
public func NestTick() { return true; }

public func IsSpiderNest() { return true; }
