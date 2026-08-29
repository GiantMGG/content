/*-- Bear Den -- territory anchor + loot chest for WBRS. --*/

#strict 3

protected func Initialize()
{
	SetAction("Idle");
	// Pre-stock the den with loot (the incentive to push in and kill the bear).
	CreateContents(BPLT, this());
	CreateContents(BCLW, this());
	CreateContents(MBIT, this());
	CreateContents(MBIT, this());
	return true;
}

// The den is destructible (low HP) -- destroying it breaks the bear's leash.
public func IsBearDen() { return true; }

protected func Damage(int iChange, int iPlr)
{
	if (GetDamage() > 50)
		RemoveObject();
	return true;
}
