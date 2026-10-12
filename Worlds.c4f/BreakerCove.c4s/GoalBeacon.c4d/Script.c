/*-- The Beacon (GBCN) -- the C1 goal card. Display rides the engine
  goal system: Activate -> MessageWindow(GetDesc()), re-readable via
  ActivateGameGoalMenu (C4Script.cpp:6353). IsFulfilled mirrors the
  scenario BCDirector's card state static. --*/

#strict
#include GOAL

public func IsFulfilled()
{
	return g_bc_c1 == 1;
}
