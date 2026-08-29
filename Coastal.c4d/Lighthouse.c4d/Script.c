/*-- Lighthouse (LHGT) -- beacon tower. --*/
/* FxBeacon sweeps a warm particle cone while lit. IsLit()/GetBeaconRange()/ */
/* InBeaconCone() are the navigation API consulted by BeaconLib.BeaconBonusAt. */
/* A lighthouse with no FuelSystem.c4d in [Definitions] is lit for free      */
/* (graceful degradation, spec edge case #10). The beam is purely visual.    */

#strict

local iBeaconRange;
local bLit;

protected func Initialize()
{
	iBeaconRange = 400;
	bLit = 1;  // lit for free unless a fuel system says otherwise
	AddEffect("FxBeacon", this, 1, 35, this);
	return 1;
}

/* ---- Navigation API ---- */

public func IsLit()          { return bLit; }
public func GetBeaconRange() { return iBeaconRange; }

public func InBeaconCone(int iX, int iY)
{
	var iDx = iX - GetX();
	var iDy = iY - GetY();
	if (iDx * iDx + iDy * iDy > iBeaconRange * iBeaconRange) return 0;
	return 1;
}

/* ---- Beacon effect: visual sweep ---- */

func FxBeaconTimer(object pTarget, int fx)
{
	if (!bLit) return 1;
	// Sweeping warm particle cone at the lighthouse top.
	CastParticles("MSpark", 4, 15, 0, -30, 20, 20, 5, 5);
	return 1;
}
