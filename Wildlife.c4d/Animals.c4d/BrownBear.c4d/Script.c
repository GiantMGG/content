/*-- Brown Bear -- territorial mini-boss, leashed to a BearDen (BDEN). --*/

#strict 3

#include ANIM
#include WLFA

public func IsPossessible() { return true; }

protected func Initialize()
{
	// opts: [Interval, DenID, TerritoryRadius, FleeHP, HealHP]. See WLFA.
	WLFA_AddBehavior(this, "Territorial", [35, BDEN, 350, 30, 60]);
	// Loot table: flat [id, chance, ...].
	WLFA_SetLootTable(this, [BPLT, 100, BCLW, 100, MBIT, 60]);
	return Birth();
}

protected func Activity()
{
	if (GetEffect("PossessionSpell", this)) return;
}

protected func Death()
{
	WLFA_DropLoot(this, WLFA_GetLootTable(this));
	ChangeDef(DWBR);
	SetAction("Dead");
	return true;
}

private func MaxAnimalCount() { return 2; }
