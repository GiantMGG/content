/*-- Starving Season (GSTV) -- the C3 goal card. Display rides the engine
  goal system: Activate -> MessageWindow(GetDesc()), re-readable via
  ActivateGameGoalMenu (C4Script.cpp:6353). IsFulfilled mirrors the
  scenario FrontDirector's card state static. --*/

#strict
#include GOAL

public func IsFulfilled()
{
	return g_fr_c3 == 1;
}
