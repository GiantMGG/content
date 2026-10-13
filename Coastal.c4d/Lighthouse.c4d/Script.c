/*-- Lighthouse (LHGT) -- beacon tower. --*/
/* FxBeacon sweeps a warm particle cone while lit. IsLit()/GetBeaconRange()/ */
/* InBeaconCone() are the navigation API consulted by BeaconLib.BeaconBonusAt. */
/* A lighthouse with no FuelSystem.c4d in [Definitions] is lit for free      */
/* (graceful degradation, spec edge case #10). The beam is purely visual.    */

#strict

local iBeaconRange;
local bLit;
local pFlame;

protected func Initialize()
{
	iBeaconRange = 400;
	bLit = 1;  // lit for free unless a fuel system says otherwise
	AddEffect("FxBeacon", this, 1, 35, this);
	AddEffect("FxBeaconVisual", this, 1, 1, this);
	UpdateBeaconVisual();  // synchronous initial sync (bLit=1 -> Lit + flame)
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

/* ---- Lit visual sync (cycle 221) ---- */

/* Public: sync facet action + flame overlay to bLit. Idempotent, no
   args. The per-tick FxBeaconVisual effect picks external
   LocalN("bLit") flips up within one frame; Initialize calls this once
   synchronously so smokes can assert the lit visual right after
   CreateObject. */
public func UpdateBeaconVisual()
{
	if (bLit)
	{
		if (GetAction() != "Lit") SetAction("Lit");
		if (!pFlame)
		{
			pFlame = CreateObject(BFLM, 0, -14, NO_OWNER);
			if (pFlame) pFlame->SetTower(this);
		}
	}
	else
	{
		if (GetAction() != "Idle") SetAction("Idle");
		if (pFlame)
		{
			RemoveObject(pFlame);
			pFlame = 0;
		}
	}
	return 1;
}

/* Per-tick poll: external bLit flips land within one frame (the beats
   rig shoots ~0.8 s after kindling; a 35-tick poll could leave the
   tower visually dark for up to ~1.2 s after kindling). */
func FxBeaconVisualTimer(object pTarget, int fx)
{
	UpdateBeaconVisual();
	return 1;
}

/* ---- Beacon effect: visual sweep ---- */

func FxBeaconTimer(object pTarget, int fx)
{
	if (!bLit) return 1;
	// Sweeping warm particle cone at the lighthouse top. 14 bright gold
	// MSpark per beat (cycle 218): a lit beacon must read as lit even in
	// a static screenshot.
	CastParticles("MSpark", 14, 25, 0, -30, 35, 55, RGBa(255, 205, 115, 255), RGBa(255, 235, 170, 255));
	return 1;
}
