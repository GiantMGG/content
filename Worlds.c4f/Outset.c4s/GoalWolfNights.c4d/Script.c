/*-- Wolf Nights (GWNV) -- the C5 goal card. Same display path as
  GSTV/GHWV; IsFulfilled mirrors the scenario FrontDirector's wolf-night
  card state static. --*/

#strict
#include GOAL

public func IsFulfilled()
{
	return g_fr_c5 == 1;
}
