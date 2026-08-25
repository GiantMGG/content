/*-- Fischernetz --*/

#strict

local durability; // decrements per catch; net removed at 0

protected func Construction()
{
	durability = 5;
	SetAction("Idle");
	return(1);
}

/* TimerCall — runs every 30 frames. Only acts when submerged. */
public func CheckCatch()
{
	if (!InLiquid()) return(1);
	var pFish;
	for (var pFish in FindObjects(Find_Distance(40), Find_ID(FISH), Find_Action("Swim")))
	{
		if (Random(4) != 0) continue; // 25% chance per matched fish
		if (durability <= 0) break;
		// Capture the fish into the net's contents; kill it so it becomes DFSH.
		Enter(this, pFish);
		Kill(pFish);
		durability--;
		Message("$TxtCaught$", this, ContentsCount(DFSH));
		Sound("Splash1");
	}
	if (durability <= 0)
	{
		// Eject any fish first so they aren't deleted with the net.
		var i = ContentsCount(), obj;
		while (i--) if (obj = Contents(i)) Exit(obj, RandomX(-4, 4), -4);
		RemoveObject();
	}
	return(1);
}

/* Player can pick up the net when dry. */
public func IsNet() { return(1); }
