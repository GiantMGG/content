/*-- TidalFlats (TFLT) -- shoreline foraging band. --*/
/* IsExposed() flips on IsLowTide(); spawns forageables (Salt/Spice/Gold)   */
/* when exposed and despawns them when submerged at high tide. Uses         */
/* C4Id("…") so it compiles even if TradeGoods.c4d is not loaded;            */
/* CreateObject returns 0 for an unloaded def, which is skipped.            */

#strict

local bExposed;
local aSpawned;

protected func Initialize()
{
	bExposed = 0;
	aSpawned = [];
	return 1;
}

protected func Timer()
{
	var bLow = IsLowTide();
	if (bLow && !bExposed)
	{
		bExposed = 1;
		SpawnForage();
	}
	else if (!bLow && bExposed)
	{
		bExposed = 0;
		DespawnForage();
	}
}

private func SpawnForage()
{
	// Existing TradeGoods.c4d collectibles: SALT, SPIC, GLDN.
	// (Spec named Salt/Amber/Clam; Amber/Clam defs do not exist in the
	// content tree, so v1 spawns the existing forageables instead.)
	var aGoodNames = ["SALT", "SPIC", "GLDN"];
	var iX = GetX();
	var iY = GetY();
	for (var i = 0; i < GetLength(aGoodNames); i++)
	{
		var idGood = C4Id(aGoodNames[i]);
		if (!idGood) continue;
		var pObj = CreateObject(idGood, iX + Random(20) - 10, iY, NO_OWNER);
		if (pObj) aSpawned[GetLength(aSpawned)] = pObj;
	}
}

private func DespawnForage()
{
	var pObj;
	for (pObj in aSpawned)
	{
		if (pObj) RemoveObject(pObj);
	}
	aSpawned = [];
}

public func IsExposed() { return bExposed; }
