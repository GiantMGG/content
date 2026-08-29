/*-- TidalPool (TDPL) -- tide-cycled hatchery basin. --*/

#strict

local feed_units;   // MEAT/FISH-type feed consumed by the hatchery
local fish_stock;   // live FISH currently counted as hatched (cap=4)
local bFlooded;     // mirrors GetAction() eq "Flooded"

protected func Construction()
{
	feed_units = 0;
	fish_stock = 0;
	bFlooded = 0;
	return 1;
}

/* Initialize runs after DoCon -> Con == FullCon, so SetAction sticks
   (cf. C4Object::SetAction: Con<FullCon without IncompleteActivity
   forces ActIdle). */
protected func Initialize()
{
	SetAction("Exposed");   // start dry
	return 1;
}

/* Tide probe -- queries the TIDE weather-event object directly via an
   indirect ~call so this script has no hard parse-time dependency on
   Coastal.c4d's global IsHighTide()/IsLowTide(). When Coastal.c4d is
   not loaded, FindObject(TIDE) returns 0 and the pool simply stays dry. */
private func IsHighTide()
{
	var pTide = FindObject(TIDE);
	if (!pTide) return 0;
	return pTide->~IsRising();
}

private func IsLowTide()
{
	var pTide = FindObject(TIDE);
	if (!pTide) return 0;
	return !pTide->~IsRising();
}

/* TimerCall (every 10 frames) -- tide coupling + hatchery + harvest routing.
   Public so the synchronous smoke harness can drive it as a seam. */
public func Timer()
{
	var bHigh = IsHighTide();
	var bLow  = IsLowTide();

	if (bHigh && !bFlooded) { SetAction("Flooded"); bFlooded = 1; }
	else if (bLow && bFlooded) { SetAction("Exposed"); bFlooded = 0; }

	if (bFlooded) HatcheryTick();
	return 1;
}

/* Flooded-state hatchery: spawn FISH from feed up to cap 4. */
private func HatcheryTick()
{
	if (fish_stock >= 4) return 1;
	if (feed_units <= 0) return 1;
	var pFish = CreateObject(FISH, GetX(), GetY(), GetOwner());
	if (!pFish) return 0;
	fish_stock++;
	feed_units--;
	return 1;
}

/* Feed input -- clonk puts MEAT/FISH-type into the pool. */
protected func RejectCollect(id def, object obj)
{
	if (def == MEAT || def == FISH || def == DFSH || def == CFSH)
	{
		if (feed_units < 8) return 0;
		return 1;
	}
	return 1;
}

public func Collected(object pClonk)
{
	// Count any feed-type just put in.
	var p; var i = 0;
	while (p = Contents(i++))
	{
		var id = GetID(p);
		if (id == MEAT || id == FISH || id == DFSH || id == CFSH)
		{
			feed_units++;
			RemoveObject(p);
		}
	}
	return 1;
}

/* Harvest: ControlDigDouble pulls matured pearls + kelp fibre. */
public func ControlDigDouble(object pClonk)
{
	[$TxtHarvest$]
	HarvestOysters(pClonk);
	HarvestKelp(pClonk);
	return 1;
}

private func HarvestOysters(object pClonk)
{
	var pOyst;
	for (pOyst in FindObjects(Find_Container(this()), Find_ID(OYST)))
	{
		if (pOyst->~IsMature()) pOyst->~HarvestPearl(pClonk);
	}
	// Also free-placed oysters within the pool footprint.
	var pOyst2;
	for (pOyst2 in FindObjects(Find_Distance(20), Find_ID(OYST)))
		if (pOyst2->~IsMature()) pOyst2->~HarvestPearl(pClonk);
}

private func HarvestKelp(object pClonk)
{
	var pKelp;
	for (pKelp in FindObjects(Find_Distance(20), Find_ID(KLPM)))
		pKelp->~Activate(pClonk);
}

public func IsTidalPool() { return 1; }
public func IsFlooded()   { return bFlooded; }
