/*-- BoilingOilCauldron.c4d -- defender emplacement. --*/

#strict 2

#include STGT  // siege-damageable so attackers can destroy it

public func MaxSiegeHP() { return 80; }
public func IsWoodenStructure() { return true; }  // FPOT x3

local fCooldown;

protected func Initialize()
{
	SetAction("Idle");
	return true;
}

/* Defender activates: pour boiling oil downward in a cone. */
protected func ControlDig(object pClonk)
{
	[$TxtPour$|Image=BOIL:0]
	if (fCooldown > 0) return;
	fCooldown = 35;  // 1s cooldown
	// Cast flaming debris + small Lava PXS downward.
	for (var i = -2; i <= 2; ++i)
		CastObjects(DFLM, 4, 30, i * 5, 5);
	CastPXS(Lava, 30, 30, 0, 10);
	Sound("Inflame");
	return true;
}

protected func Timer()
{
	if (fCooldown > 0) --fCooldown;
}
