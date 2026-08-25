/*-- Sichel --*/

#strict

/* Use the Sickle on a Wheat tile to harvest. */

public func ControlDigDouble(object pClonk)
{
	[$TxtHarvest$]
	return(HarvestNearby(pClonk));
}

public func ContextHarvest(object pClonk)
{
	[$TxtHarvest$|Image=AGWH]
	return(HarvestNearby(pClonk));
}

private func HarvestNearby(object pClonk)
{
	if (!pClonk) return(0);
	// Find a ripe wheat within reach in front of the Clonk.
	var pWheat;
	for (var pWheat in FindObjects(Find_Distance(20), Find_ID(AGWH), Sort_Distance()))
	{
		if (!pWheat->~IsRipe()) { Message("$TxtNotRipe$", pClonk); Sound("Error"); return(0); }
		AddCommand(pClonk, "MoveTo", 0, GetX(pWheat), GetY(pWheat));
		AddCommand(pClonk, "Call", pWheat, 0, 0, 0, 0, "Harvest");
		return(1);
	}
	// No wheat found.
	Message("$TxtNowheat$", pClonk);
	Sound("Error");
	return(0);
}

public func DoHarvest(object pClonk, object pWheat)
{
	if (!pWheat) return(0);
	pWheat->~Harvest(pClonk);
	return(1);
}

protected func Hit() { Sound("MetalHit*"); }
