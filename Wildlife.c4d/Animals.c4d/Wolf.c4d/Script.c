/*-- Gray Wolf -- pack-flank predator. --*/

#strict 3

#include ANIM
#include WLFA

public func IsPossessible() { return true; }

protected func Initialize()
{
	WLFA_AddBehavior(this, "HuntPrey",  { Range: 500, Interval: 35 });
	WLFA_AddBehavior(this, "PackFlank", { PackID: WOLF, Interval: 35 });
	WLFA_SetLootTable(this, { items: [WPLT, 100, MBIT, 40] });
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
	ChangeDef(DWLF);
	SetAction("Dead");
	return true;
}

/* Alpha death scatters the pack. Called by the engine via Kill/Death. */
protected func Destruction()
{
	// If this wolf was the alpha, scatter the survivors.
	if (GetEffect("WLFA_IsAlpha", this))
		WLFA_ScatterPack(WOLF);
	return true;
}

/* Reproduction cap -- cooperates with REPR. */
private func MaxAnimalCount() { return 4; }
