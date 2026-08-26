/*-- Cave Bat --*/

#strict
#include BIRD

local pRoost;     // ceiling roost object the bat returns to
local fSwarming;  // swarm-state flag

public func IsPossessible() { return 1; }

protected func Initialize()
{
	SetAction("Fly");
	SetComDir(Random(2) ? COMD_Right : COMD_Left);
	Birth();
}

/* BIRD TimerCall override: roost AI + swarm trigger */
protected func Activity()
{
	if (GetEffect("PossessionSpell", this())) return;
	if (Contained()) return;

	// Echolocation chirp
	if (!Random(50)) Sound("Bat*");

	// If swarming, continue dive-attacking for ~2s (120 frames) then return to roost
	if (fSwarming)
	{
		if (!Random(20)) Attack();
		fSwarming--;
		if (fSwarming <= 0 && pRoost)
			SetCommand(this(), "MoveTo", pRoost);
		return;
	}

	// Roosting: stay attached near ceiling; if roost destroyed, swarm and flee
	if (pRoost && !GetAlive(pRoost))
	{
		fSwarming = 120;
		pRoost = 0;
		return;
	}

	// Disturbance trigger: a living Clonk walks under the roost within 80 px
	if (pRoost && !GetCommand())
	{
		var intruder = FindObject(0, -80, -200, 160, 220, OCF_Alive, 0, 0, NoContainer());
		if (intruder && GetCategory(intruder) & C4D_Living)
		{
			fSwarming = 120;
			SetCommand(this(), "Follow", intruder);
			return;
		}
	}

	// Default BIRD flight drift
	if (GetAction() ne "Fly") return;
	if (!Random(4)) SetComDir(COMD_Up);
	if (!Random(4)) SetComDir(COMD_Down);
}

public func Attack(object pAttack)
{
	if (GetAction() ne "Fly") return;
	if (!pAttack) pAttack = FindObject(0, -20, -20, 40, 40, OCF_Alive, 0, 0, NoContainer());
	if (!pAttack) return;
	Punch(pAttack, 3);
	SetAction("Attack");
	Sound("Bat*");
}

protected func Survive()
{
	if (GetEffect("PossessionSpell", this())) return;
	if (InLiquid() || GBackLiquid(GetXDir()*2, GetYDir()*2))
		SetComDir(COMD_Up);
}

protected func Death()
{
	ChangeDef(DBAT);
	return 1;
}

/* Roost assignment API (called by scenario Script.c when placing bats) */
public func SetRoost(object roost) { pRoost = roost; }
