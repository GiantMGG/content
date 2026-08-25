/*-- Apfelbaum --*/

#strict

#include TREE

local is_sapling;     // TRUE while still maturing
local drop_apples;    // FALSE while maturing, TRUE once grown

protected func Construction()
{
	is_sapling = 1;
	drop_apples = 0;
	SetAction("Idle");
	return(1);
}

/* Called by AppleSeed after planting to mark the new object as a sapling. */
public func SetAsSapling()
{
	is_sapling = 1;
	drop_apples = 0;
	SetCon(10);
	return(1);
}

/* TREE library overrides — keep orchards small and slow-spreading. */
private func ReproductionAreaSize() { return(300); }
private func ReproductionRate()     { return(800); }
private func MaxTreeCount()         { return(8); }

/* Mature after ~5 game-minutes: turn on apple dropping. */
public func Mature()
{
	is_sapling = 0;
	drop_apples = 1;
	SetAction("Idle");
	Sound("Chop?");
	return(1);
}

/* TimerCall — drops an apple under the canopy every ~2 game-minutes. */
public func DropApple()
{
	// Still maturing?
	if (is_sapling && GetCon() < 100)
	{
		DoCon(2);
		if (GetCon() >= 100) Mature();
		return(1);
	}
	if (!drop_apples) return(1);
	if (Random(2)) return(1); // 50% chance per tick
	var pApple = CreateObject(AGAP, RandomX(-ReproductionAreaSize()/4, ReproductionAreaSize()/4), 0, GetOwner());
	if (pApple) pApple->Exit(RandomX(-6, 6), -4);
	return(1);
}
