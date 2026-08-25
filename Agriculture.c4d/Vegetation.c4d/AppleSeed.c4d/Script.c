/*-- Apfelkern --*/

#strict

public func Activate(object pClonk)
{
	[$TxtPlant$]
	return(Plant(pClonk));
}

public func Plant(object pClonk)
{
	if (!pClonk) return(0);
	var mat = GetMaterial(GetX(pClonk) - GetX(), GetY(pClonk) - GetY() + 8);
	if (mat == -1 || (mat != Material("Earth") && mat != Material("Tunnel") && mat != Material("Earth-soil")))
	{
		Message("$TxtBadSoil$", pClonk);
		return(0);
	}
	// Spawn a sapling that will mature into an AppleTree.
	var pSapling = CreateObject(AGAT, GetX(pClonk) - GetX(), GetY(pClonk) - GetY() + 8, GetOwner(pClonk));
	if (pSapling) pSapling->~SetAsSapling();
	Sound("Dig?");
	RemoveObject();
	return(1);
}

protected func Hit() { Sound("WoodHit*"); }
