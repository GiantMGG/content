/*-- Gray Wolf -- pack-flank predator. --*/

#strict 3

#include ANIM
#include WLFA

public func IsPossessible() { return true; }

protected func Initialize()
{
	// opts arrays: [Interval, ...behaviour-fields]. See WLFA.
	WLFA_AddBehavior(this, "HuntPrey",  [35, 500]);
	WLFA_AddBehavior(this, "PackFlank", [35, WOLF]);
	// Loot table: flat [id, chance, ...].
	WLFA_SetLootTable(this, [WPLT, 100, MBIT, 40]);
	return Birth();
}

protected func Activity()
{
	// Driven by WLF_* behaviour effects. Possessed creatures skip AI.
	if (GetEffect("PossessionSpell", this)) return;
}

protected func Death()
{
	// Local(0) is the WLFA_IsAlpha flag set by WLF_PackFlank. It survives
	// AssignDeath's effect clearing, so we can still tell whether this wolf
	// was the alpha here. Scatter the survivors BEFORE ChangeDef (after
	// ChangeDef the script scope is DWLF, which doesn't #include WLFA).
	var wasAlpha = Local(0);
	WLFA_DropLoot(this, WLFA_GetLootTable(this));
	if (wasAlpha) WLFA_ScatterPack(WOLF);
	ChangeDef(DWLF);
	SetAction("Dead");
	return true;
}

/* Reproduction cap -- cooperates with REPR. */
private func MaxAnimalCount() { return 4; }
