/*-- Raeucherei --*/

#strict

public func ContextSmoke(object pCaller)
{
	[$TxtSmoke$|Image=AGSF]
	return(ProductionOrder(pCaller));
}

public func ControlDigDouble(object pCaller)
{
	[$TxtSmoke$|Image=AGSF]
	return(ProductionOrder(pCaller));
}

public func ProductionOrder(object pWorker)
{
	// Fish in the smokehouse or worker inventory
	if (!FindFishContents() && !FindFishContents(pWorker))
		return(Message("$TxtNofish$", pWorker));
	// Begin work
	AddCommand(pWorker, "Call", this, 0, 0, 0, 0, "ProductionStart");
	AddCommand(pWorker, "MoveTo", 0, GetX(), GetY());
	// Acquire wood (2 needed)
	ProductionAcquireMaterial(WOOD, 2, pWorker);
	// Acquire one fish
	ProductionAcquireMaterialFish(pWorker);
	return(1);
}

private func ProductionAcquireMaterial(id idMaterial, int iAmount, object pWorker)
{
	var amount = Max(iAmount - ContentsCount(idMaterial), 0);
	while (amount--)
	{
		AddCommand(pWorker, "Put", this, 0, 0, 0, 0, idMaterial);
		AddCommand(pWorker, "Acquire", 0, 0, 0, this, 0, idMaterial, 5, 3);
	}
}

private func ProductionAcquireMaterialFish(object pWorker)
{
	// Acquire any fish-type id (FISH, DFSH, CFSH) — try DFSH first (dead, easy to haul)
	var fishIds = [DFSH, CFSH, FISH];
	for (var idFish in fishIds)
	{
		if (FindContents(idFish)) return; // already have one
		AddCommand(pWorker, "Put", this, 0, 0, 0, 0, idFish);
		AddCommand(pWorker, "Acquire", 0, 0, 0, this, 0, idFish, 5, 3);
		return;
	}
}

public func ProductionStartFailed() { return(1); }

public func ProductionStart()
{
	// Need 2 wood
	if (ContentsCount(WOOD) < 2)
	{
		Message("$TxtNotenoughwood$", this);
		return(false);
	}
	// Need one fish-type input
	var pFish = FindFishContents();
	if (!pFish)
	{
		Message("$TxtNofish$", this);
		return(false);
	}
	// Kill live fish before smoking (mirrors Trapper Kill(pFish))
	if (GetID(pFish) == FISH) Kill(pFish);
	// Consume wood
	RemoveObject(FindContents(WOOD));
	RemoveObject(FindContents(WOOD));
	// Consume fish input
	RemoveObject(pFish);
	SetAction("Smoking");
	return(1);
}

private func Smoking()
{
	if (GetActTime() < 300) return(1);
	SetAction("Idle");
	// Smoke particles
	Smoke(0, -30, 12);
	Sound("Pshshsh");
	// Output SmokedFish
	var pOut = CreateObject(AGSF, 0, 0, GetOwner());
	if (pOut) pOut->Exit(RandomX(-6, 6), -10);
	return(1);
}

private func FindFishContents(object pContainer)
{
	var i, obj;
	while (obj = Contents(i++, pContainer))
	{
		var id = GetID(obj);
		if (id == FISH || id == DFSH || id == CFSH) return(obj);
	}
	return(0);
}

protected func RejectCollect(id def, object obj)
{
	// Wood ok
	if (def == WOOD)
	{
		if (ContentsCount(WOOD) < 5) return(0);
		Message("$TxtMorewoodwontfit$", this);
		return(1);
	}
	// Fish-type ok
	var id = def;
	if (id == FISH || id == DFSH || id == CFSH)
	{
		if (ContentsCount() - ContentsCount(WOOD) < 4) return(0);
		Message("$TxtMorefishwontfit$", this);
		return(1);
	}
	// Reject everything else
	return(1);
}
