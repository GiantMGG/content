/*-- Meat Bait -- lures predators (IsBait contract, cf. Wipf.c4d:52) --*/

#strict 3

local iBait;

protected func Initialize()
{
	SetAction("Idle");
	iBait = 50;
	return true;
}

protected func Hit() { Sound("WoodHit*"); }

public func IsBait() { return iBait; }

public func BaitPower(int iPower) { iBait = iPower; }
