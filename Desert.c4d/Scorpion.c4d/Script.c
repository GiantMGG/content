/*-- Scorpion (SCRP) -- nocturnal desert predator. --*/

#strict 3

#include ANIM
#include WLFA

public func IsPossessible() { return true; }

protected func Initialize()
{
	WLFA_AddBehavior(this, "HuntPrey", [35, 300]);
	WLFA_SetLootTable(this, [MEAT, 100]);
	return Birth();
}

protected func Activity()
{
	// Driven by WLF_* behaviour effects. Possessed creatures skip AI.
	if (GetEffect("PossessionSpell", this)) return;
}

protected func Death()
{
	WLFA_DropLoot(this, WLFA_GetLootTable(this));
	RemoveObject();
	return true;
}

private func MaxAnimalCount() { return 6; }
