/*-- Camel (CAML) -- passive desert herd animal. --*/

#strict 3

#include ANIM
#include WLFA

public func IsPossessible() { return true; }

protected func Initialize()
{
	// No predator behaviour -- the camel idles via ANIM Activity.
	WLFA_SetLootTable(this, [MEAT, 100]);
	return Birth();
}

protected func Activity()
{
	// Driven by ANIM idle wander. Possessed creatures skip AI.
	if (GetEffect("PossessionSpell", this)) return;
}

protected func Death()
{
	WLFA_DropLoot(this, WLFA_GetLootTable(this));
	RemoveObject();
	return true;
}

private func MaxAnimalCount() { return 4; }
