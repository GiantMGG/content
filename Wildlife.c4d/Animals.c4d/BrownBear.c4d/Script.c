/*-- Brown Bear -- territorial mini-boss, leashed to a BearDen (BDEN). --*/

#strict 3

#include ANIM
#include WLFA

public func IsPossessible() { return true; }

protected func Initialize()
{
	WLFA_AddBehavior(this, "Territorial",
		{ DenID: BDEN, TerritoryRadius: 350, FleeHP: 30, HealHP: 60, Interval: 35 });
	WLFA_SetLootTable(this, { items: [BPLT, 100, BCLW, 100, MBIT, 60] });
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
