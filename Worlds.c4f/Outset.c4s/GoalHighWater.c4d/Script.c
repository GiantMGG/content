/*-- High Water Harvest (GHWV) -- the C4 goal card. Same display path as
  GSTV; IsFulfilled mirrors the flood card state. --*/

#strict
#include GOAL

public func IsFulfilled()
{
	return g_fr_c4 == 1;
}
