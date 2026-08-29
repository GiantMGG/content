/*-- Oasis (OASS) -- self-contained water-pool generator. --*/
/* Tide.c4d CastPXS pattern; basin carved via DigFreeRect.   */

#strict

public func Construction()
{
	// Carve a shallow basin (bounded).
	DigFreeRect(GetX() - 12, GetY() - 4, 24, 8);
	return 1;
}

public func Fill()
{
	// Probe: is the basin already full of Water?
	if (GetMaterial(GetX() - 2, GetY() - 2) == Material("Water")
	 && GetMaterial(GetX() + 2, GetY() - 2) == Material("Water"))
		return 0;
	// Cast Water PXS into the basin.
	CastPXS("Water", 20, 16, GetX(), GetY() - 2);
	// PXS cast this tick (smoke-assertable).
	return 20;
}
