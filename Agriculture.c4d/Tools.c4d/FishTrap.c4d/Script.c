/*-- Fischreuse --*/

#strict

local bait_obj;     // temporary bait object spawned by Attract
local fish_count;   // tracks held live fish

protected func Construction()
{
	fish_count = 0;
	SetAction("Idle");
	return(1);
}

/* TimerCall (every 60 frames) — emits a weak bait signal so nearby
   fish route towards the trap, then harvests any fish within 15 px.
   When the trap is full, auto-empty so the cycle keeps running. */
public func Attract()
{
	if (!InLiquid()) return(1);
	// Auto-empty once full so the trap keeps catching.
	if (fish_count >= 4) EmptyTrap();
	// Spawn bait signal if not already active.
	if (!bait_obj || !GetAlive(bait_obj))
	{
		bait_obj = CreateObject(MBAT, 0, 0, GetOwner());
		if (bait_obj)
		{
			bait_obj->~StartBaiting();
			ObjectSetAction(bait_obj, "Bait", this());
		}
	}
	// Harvest any fish within 15 px (alive; EmptyTrap live-releases them).
	var pFish;
	for (var pFish in FindObjects(Find_Distance(15), Find_ID(FISH), Find_Action("Swim")))
	{
		if (fish_count >= 4) break;
		// Skip fish already inside a container (e.g. this trap): a wild
		// fish swimming past has Contained() == nil and still gets caught.
		if (pFish->Contained()) continue;
		Enter(this, pFish);
		fish_count++;
		// Positional splash as the fish slips into the trap
		Sound("FishCatch", false, this);
	}
	return(1);
}

public func EmptyTrap()
{
	// Live-release (cycle 183): every contents exits alive - no Kill.
	// Fish released into liquid immediately swim; ashore they are left
	// to natural fish behavior. Banking happens by collecting the fish
	// out of the trap before it auto-empties at 4.
	var i = ContentsCount(), obj;
	while (i--) if (obj = Contents(i))
	{
		Exit(obj, RandomX(-4, 4), -4);
		if (GetID(obj) == FISH)
		{
			if (GBackLiquid(GetX(obj) - GetX(), GetY(obj) - GetY())) obj->SetAction("Swim");
			fish_count--;
		}
	}
	// Safety: never leave a stale count.
	fish_count = 0;
	return(1);
}

public func ControlDigDouble(object pClonk)
{
	[$TxtEmpty$]
	return(EmptyTrap());
}

public func IsTrap() { return(1); }
