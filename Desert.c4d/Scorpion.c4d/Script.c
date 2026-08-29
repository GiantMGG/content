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

/* Sting: the base attack plus a Webbed-style venom slow on the victim
   (spec §3: WLFA_Attack + reused physical reduction). Overrides the
   WLFA framework attack; called via object call by WLF_HuntPrey. */
public func WLFA_Attack(object prey, int damage)
{
	if (!inherited(prey, damage)) return false;
	// Venom slow: Webbed-style Walk cut that wears off after ~5s.
	if (prey && !GetEffect("ScorpionVenom", prey))
		AddEffect("ScorpionVenom", prey, 1, 35, this);
	return true;
}

func FxScorpionVenomStart(object target, int effect, bool temp)
{
	if (temp) return;
	// Record the baseline Walk physical, then cut it (Webbed-style).
	EffectVar(0, target, effect) = GetPhysical("Walk", 0, target);
	SetPhysical("Walk", 30000, 2, target);
}

func FxScorpionVenomTimer(object target, int effect, int time)
{
	// Venom wears off after ~5 seconds.
	if (time >= 175) return FX_Execute_Kill;
	return FX_OK;
}

func FxScorpionVenomStop(object target, int effect, int reason, bool temp)
{
	if (temp) return;
	// Restore the baseline Walk physical.
	SetPhysical("Walk", EffectVar(0, target, effect), 2, target);
}

protected func Death()
{
	WLFA_DropLoot(this, WLFA_GetLootTable(this));
	RemoveObject();
	return true;
}

private func MaxAnimalCount() { return 6; }
