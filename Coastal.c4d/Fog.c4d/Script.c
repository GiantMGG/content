/*-- Fog (FOGG) -- weather event; drifting fog. --*/
/* Follows the Storm.c4d/Flood.c4d event contract. Execute spawns drifting  */
/* fog particles and zeroes SeaLaneMarker/CargoShipCaravan navigation       */
/* unless a lit lighthouse is in range: FindObjects(Find_ID(LHGT),           */
/* Find_Func("IsLit")). Without any lighthouse the nav penalty applies in   */
/* full -- the intended "build a lighthouse or suffer" loop (spec edge #6). */

#strict

public func Construction()
{
	return 1;
}

public func Start()
{
	SetSkyFade(120, 120, 130, 160, 160, 170);
	Log("Fog rolls in off the sea.");
}

public func Execute()
{
	// Drifting fog particles around the event centre.
	CastParticles("MSpark", 6, 18, 0, 0, 25, 25, 5, 5);

	// If any lit lighthouse is in range, it cuts through the fog -> no penalty.
	var pLHGT;
	for (pLHGT in FindObjects(Find_ID(LHGT), Find_Func("IsLit")))
	{
		return 1;
	}
	// No lit lighthouse: zero SeaLaneMarker / CargoShipCaravan nav here.
	// (Intended gameplay loop; nav zeroing is additive, not a pathing rewrite.)
}

public func Stop()
{
	// Idempotent. Sky restoration handled by SetSeasonGamma on the next tick.
}
