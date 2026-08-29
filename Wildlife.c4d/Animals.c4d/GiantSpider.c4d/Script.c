/*-- Giant Spider -- cave web-trap predator. --*/

#strict 3

#include ANIM
#include WLFA

public func IsPossessible() { return true; }

protected func Initialize()
{
	// opts arrays: [Interval, ...behaviour-fields]. See WLFA.
	WLFA_AddBehavior(this, "HuntPrey", [35, 300]);
	WLFA_AddBehavior(this, "WebTrap",  [35, WEBB]);
	// Loot table: flat [id, chance, ...].
	WLFA_SetLootTable(this, [SSIL, 100, SFNG, 30]);
	return Birth();
}

protected func Activity()
{
	if (GetEffect("PossessionSpell", this)) return;
}

protected func Death()
{
	WLFA_DropLoot(this, WLFA_GetLootTable(this));
	ChangeDef(DSPR);
	SetAction("Dead");
	// Killing the spider clears all its webs (owned via Local(0)).
	var webs = FindObjects(Find_ID(WEBB));
	var w;
	for (var w in webs)
		if (Local(0, w) == this)
			RemoveObject(w);
	return true;
}

private func MaxAnimalCount() { return 6; }
