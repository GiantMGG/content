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
	// Probe: is the basin already full of Water? (GetMaterial is
	// object-relative in object context -- the offsets are oasis-relative,
	// i.e. inside the basin carved by Construction.)
	if (GetMaterial(-2, -2) == Material("Water")
	 && GetMaterial(2, -2) == Material("Water"))
		return 0;
	// Cast Water PXS into the basin (CastPXS likewise object-relative).
	CastPXS("Water", 20, 16, 0, -2);
	// PXS cast this tick (smoke-assertable).
	return 20;
}
