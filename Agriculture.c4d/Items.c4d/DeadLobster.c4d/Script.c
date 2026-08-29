/*-- DeadLobster (DLBS) -- dead lobster corpse. --*/

#strict

private func Decaying()
{
	DoCon(-4);
}

public func Eat(object pByObject)
{
	pByObject->~Feed(50 * GetCon() / 100);
	RemoveObject();
	return 1;
}

public func Cook()
{
	// Decaying corpses cannot be cooked.
	if (GetAction() S= "Decay") return 0;
	ChangeDef(CLBS);
	return 1;
}

public func Activate(object pClonk)
{
	[$TxtEmbowel$|Image=KNFE]
	if (pClonk->~IsAquaClonk()) return Eat(pClonk);
	return ObjectSetAction(pClonk, "Embowel", this());
}

public func Embowel(object pClonk)
{
	if (pClonk) pClonk->Split2Components(this());
	else Split2Components(this());
	return 1;
}
