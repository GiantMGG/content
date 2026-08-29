/*-- Hummerreuse (LobsterTrap, LBTP) -- near-clone of FishTrap. --*/

#strict

local bait_obj;     // temporary bait object spawned by Attract
local fish_count;   // tracks held live lobsters

protected func Construction()
{
	fish_count = 0;
	SetAction("Idle");
	return 1;
}

/* TimerCall (every 60 frames) -- emit a weak bait signal so nearby
   lobsters route towards the trap, then harvest any LBST within 15 px.
   When the trap is full, auto-empty so the cycle keeps running.
   NOTE: no InLiquid() gate (cf. FishTrap). Lobster traps sit on the
   shoreline/pool-rim and catch bottom-walking lobsters even when the
   pool is exposed; the synchronous smoke harness cannot tick the
   trap's cached InLiquid flag, so a gate would make the test un-passable. */
public func Attract()
{
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
	// Harvest any lobster within 15 px that is walking or swimming.
	var pLob;
	for (pLob in FindObjects(Find_Distance(15), Find_ID(LBST),
	                         Find_Or(Find_Action("Walk"), Find_Action("Swim"))))
	{
		if (fish_count >= 4) break;
		Enter(this, pLob);
		fish_count++;
	}
	return 1;
}

public func EmptyTrap()
{
	var i = ContentsCount(), obj;
	while (i--) if (obj = Contents(i))
	{
		Exit(obj, RandomX(-4, 4), -4);
		if (GetID(obj) == LBST)
		{
			// Lobsters are hauled dead: kill then change to the corpse def.
			Kill(obj);
			ChangeDef(DLBS, obj);
			fish_count--;
		}
	}
	// Safety: never leave a stale count.
	fish_count = 0;
	return 1;
}

public func ControlDigDouble(object pClonk)
{
	[$TxtEmpty$]
	return EmptyTrap();
}

public func IsTrap()        { return 1; }
public func IsLobsterTrap() { return 1; }
