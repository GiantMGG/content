/*-- Weizen --*/

#strict

local grow_stage; // 0=Seedling, 1=Growing, 2=Ready

protected func Construction()
{
	grow_stage = 0;
	SetAction("Seedling");
	return(1);
}

public func IsWheat() { return(1); }

public func IsRipe() { return(GetAction() eq "Ready"); }

/* TimerCall — advances through the 3 phases once each */
public func Grow()
{
	if (GetAction() eq "Seedling")
	{
		SetAction("Growing");
		Sound("Dig?");
		return(1);
	}
	if (GetAction() eq "Growing")
	{
		SetAction("Ready");
		Sound("Chop?");
		return(1);
	}
	// Already ripe: nothing to do.
	return(1);
}

/* Harvested by the Sickle (see Tools/Sickle.c4d). Returns 2 WheatSheaf. */
public func Harvest(object pClonk)
{
	if (!IsRipe()) return(0);
	var i;
	for (i = 0; i < 2; i++)
	{
		var pSheaf = CreateObject(AGSH, 0, 0, GetOwner());
		if (pClonk) pClonk->Collect(pSheaf);
		else pSheaf->Exit(RandomX(-5, 5), -5);
	}
	RemoveObject();
	return(1);
}
