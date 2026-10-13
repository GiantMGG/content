/*-- BeaconFlame (BFLM) -- the drawn lit-beacon overlay (cycle 221). --*/
/* Spawned and removed by Lighthouse.c4d's UpdateBeaconVisual sync.     */
/* Purely visual: StaticBack+Foreground+MouseIgnore keeps it drawn      */
/* above the scene, unselectable and non-interactive. The guard effect  */
/* is belt-and-braces: a tower gone dark or removed never leaves an     */
/* orphan flame (CoastalSmoke removes lit LHGTs).                       */

#strict

local pTower;

public func SetTower(object pT)
{
	pTower = pT;
	return 1;
}

protected func Initialize()
{
	AddEffect("FxGuard", this, 1, 5, this);
	return 1;
}

/* Belt-and-braces self-removal: if the owning tower is gone (the engine
   zeroes all refs on removal, so pTower reads 0) or dark, remove the
   overlay. The LHGT sync is the primary owner of spawn/removal; this
   guard only covers orphan cases. */
func FxGuardTimer(object pTarget, int fx)
{
	if (!pTower || !pTower->IsLit())
	{
		RemoveObject();
		return -1;
	}
	return 1;
}
