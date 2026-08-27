/*-- BoilingOilCauldron.c4d -- defender emplacement. --*/

#strict 2

#include STGT

local fCooldown;

protected func Initialize()
{
	return true;
}

/* Defender activates: pour boiling oil downward in a cone. */
protected func ControlDig(object pClonk)
{
	[$TxtPour$|Image=BOIL:0]
	if (fCooldown > 0) return;
	fCooldown = 35;  // 1s cooldown
	// Cast flaming debris downward.
	for (var i = -2; i <= 2; ++i)
		CastObjects(DFLM, 4, 30, i * 5, 5);
	Sound("Inflame");
	return true;
}

protected func Timer()
{
	if (fCooldown > 0) --fCooldown;
}
