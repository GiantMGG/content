/*-- Weizensamen --*/

#strict

public func Activate(object pClonk)
{
	[$TxtPlant$]
	return(Plant(pClonk));
}

public func Plant(object pClonk)
{
	if (!pClonk) return(0);
	// Check the tile the Clonk is standing on.
	var mat = GetMaterial(GetX(pClonk) - GetX(), GetY(pClonk) - GetY() + 8);
	if (mat == -1)
	{
		Message("$TxtBadSoil$", pClonk);
		return(0);
	}
	if (mat != Material("Earth") && mat != Material("Tunnel") && mat != Material("Earth-soil"))
	{
		Message("$TxtBadSoil$", pClonk);
		return(0);
	}
	// Spawn wheat on the tile.
	var pWheat = CreateObject(AGWH, GetX(pClonk) - GetX(), GetY(pClonk) - GetY() + 8, GetOwner(pClonk));
	if (!pWheat) return(0);
	Sound("Dig?");
	// Consume the seed.
	RemoveObject();
	return(1);
}

protected func Hit() { Sound("WoodHit*"); }
