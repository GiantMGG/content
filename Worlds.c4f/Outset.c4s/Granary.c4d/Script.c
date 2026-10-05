/*-- Granary.c4d -- scenario-local granary (GRNY) for Outset.c4s, twin of
  the AutoTasksAccept granary: cap-10 RejectCollect for AGSH/AGAP, the
  headless Deposit banking pair, and the Eat hunger counterplay (Feed 40,
  the Apple.Eat energy shape). --*/

#strict 3

protected func IsBuilt()
{
	return GetCon() >= 100;
}

/* Engine-collect face: accept AGSH/AGAP while under their caps, reject
   everything else (Windmill.c4d/Script.c:91-99 shape, cap 10). */
protected func RejectCollect(id def, object obj)
{
	if (def == AGSH)
	{
		if (ContentsCount(AGSH) < 10) return(0);
		return(1);
	}
	if (def == AGAP)
	{
		if (ContentsCount(AGAP) < 10) return(0);
		return(1);
	}
	return(1);
}

/* Headless banking pair: move one carried sheaf into the granary.
   Script-side cap mirrors RejectCollect: no room, the sheaf stays carried
   and we return 0 (the smoke cap-pin assert's target). */
public func Deposit(object clonk)
{
	if (ContentsCount(AGSH) + ContentsCount(AGAP) >= 10) return 0;
	var sheaf = clonk->FindContents(AGSH);
	if (!sheaf) return 0;
	RemoveObject(sheaf);
	CreateContents(AGSH, this, 1);
	Log("FRMT:deposit=1");
	return 1;
}

/* Hunger counterplay: consume one edible, restore crew energy. */
public func Eat(object clonk)
{
	var pFood = FindContents(AGSH);
	if (!pFood) pFood = FindContents(AGAP);
	if (!pFood) return 0;
	RemoveObject(pFood);
	clonk->~Feed(40);
	Log("FRMT:eat=1");
	return 1;
}

public func ContextDeposit(object clonk)
{
	[$Deposit$|Condition=IsBuilt]
	return Deposit(clonk);
}

public func ContextEat(object clonk)
{
	[$Eat$|Condition=CanEat]
	return Eat(clonk);
}

public func CanEat()
{
	return FindContents(AGSH) || FindContents(AGAP);
}
