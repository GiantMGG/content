/*-- Angel (pack-lokal, unabhaengig vom Trapper) --*/

#strict

/* Same bait contract as Western FishingPole (FIPL), but the
   ReadyToFish() check is self-contained so any crew can fish. */

private func FindBaitInContents(object pClonk)
{
	var pContents;
	for (var i = 0; pContents = Contents(i, pClonk); i++)
		if (pContents->~IsBait()) return(pContents);
	return(0);
}

public func Activate(object pClonk)
{
	[$TxtStartFishing$]
	// Clonk must be fully grown.
	if (GetCon(pClonk) < 100)
		return(Message("$TxtCantFish$", pClonk, GetName(pClonk)));
	// Self-contained ReadyToFish: any walking crew on a shoreline.
	if (!ReadyToFish(pClonk))
		return(0);
	if (!FindBaitInContents(pClonk))
		return(Message("$TxtNoBait$", pClonk));
	ObjectSetAction(pClonk, "ThrowFishingPole");
	return(1);
}

/* Decoupled ReadyToFish — does not require the Trapper crew type.
   Accepts any living, walking Clonk at a shoreline. Returns 1 if ready,
   0 otherwise (with a message explaining why). */
public func ReadyToFish(object pClonk)
{
	if (!pClonk) return(0);
	if (!GetAlive(pClonk)) return(0);
	if (GetAction(pClonk) ne "Walk") return(0);
	// Must be standing next to liquid (water).
	if (!GBackLiquid(0, 8) && !GBackLiquid(0, 16) && !GBackLiquid(-8, 8) && !GBackLiquid(8, 8))
	{
		Message("$TxtNowater$", pClonk);
		return(0);
	}
	return(1);
}

public func CreateBait(object pClonk)
{
	var pBait;
	if (!(pBait = FindBaitInContents(pClonk)))
		return(Message("$TxtNoBait$", pClonk));
	pBait->~StartBaiting();
	Exit(pBait, -15 + 31 * GetDir(pClonk), 18);
	if (GBackLiquid(-15 + 31 * GetDir(pClonk), 18)) Sound("Splash1");
	ObjectSetAction(pBait, "Bait", pClonk);
	return(1);
}

protected func Hit() { Sound("WoodHit*"); }
