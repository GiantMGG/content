/*-- Storm Watch (GSWV) -- the C6 goal card. Same display path as
  GSTV/GHWV; IsFulfilled mirrors the scenario FrontDirector's storm
  card state static. --*/

#strict
#include GOAL

public func IsFulfilled()
{
	return g_fr_c6 == 1;
}
