/*-- Oyster (OYST) -- slow-cycle pearl producer. --*/

#strict

local pearl_progress;   // 0..100

protected func Construction()
{
	pearl_progress = 0;
	SetAction("Closed");
	return 1;
}

/* TimerCall -- mature only while submerged (GBackLiquid). */
protected func Timer()
{
	if (!GBackLiquid()) return 1;
	if (pearl_progress >= 100) return 1;
	pearl_progress++;
	if (pearl_progress == 100) SetAction("Open");
	return 1;
}

public func IsMature()        { return pearl_progress >= 100; }
public func GetPearlProgress() { return pearl_progress; }

/* Harvest the pearl into the clonk's contents. */
public func HarvestPearl(object pClonk)
{
	if (pearl_progress < 100) return 0;
	var pPearl = CreateContents(APRL, pClonk);
	if (!pPearl) Enter(pClonk, CreateObject(APRL, 0, 0, GetOwner()));
	pearl_progress = 0;
	SetAction("Closed");
	return 1;
}

/* Direct Activate: dive-harvest by any clonk. */
public func Activate(object pClonk)
{
	if (!pClonk) return 1;
	if (pearl_progress < 100)
	{
		pClonk->Message("$TxtNotmature$");
		return 1;
	}
	return HarvestPearl(pClonk);
}
