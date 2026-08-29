/*-- Spider Nest -- ZapNest-style spawner for SPDR spiderlings. --*/
/* Uses the WLF_NestSpawner behaviour from WLFA.                   */

#strict 3

#include WLFA

protected func Initialize()
{
	SetAction("Idle");
	WLFA_AddBehavior(this, "NestSpawner", { SpawnID: SPDR, Cap: 6, Interval: 210 });
	return true;
}

// Convenience: the nest's own TimerCall (DefCore TimerCall=NestTick) is
// redundant with the WLF_NestSpawner effect timer but kept as a no-op
// anchor so scenario authors can override it.
public func NestTick() { return true; }

public func IsSpiderNest() { return true; }
