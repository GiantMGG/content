/*-- Goal_Siege.c4d -- goal-framework wrapper so the win screen shows. --*/

#strict 2

public func IsFulfilled()
{
	// The SiegeDirector calls GameOver() directly; this goal is a no-op
	// shell so the [Game] Goals= entry resolves under the full GUI build.
	return false;
}
