/*-- SaltPan (SLTP) -- coastal salt works. --*/
/* Seawater evaporation: while the pan's shoreline-contact samples hit */
/* Water material, its Timer adds 1 SALT to the hold (cap 8).          */
/* Implements the MarketStall trade contract (RegisterTradeGoodAt /    */
/* GetTradeGoods / GetTradeBaselines) so a scenario can register it as */
/* a SALT vendor via RegisterTradeGood(SALT, pan, 8) -- TradeLib       */
/* buy/sell/price slots in unchanged.                                   */

#strict

local aTradeGoods;
local aBaselines;

protected func Initialize() {
	aTradeGoods = [];
	aBaselines = [];
	return 1;
}

/* ---- MarketStall trade contract ---- */

public func RegisterTradeGoodAt(id idGood, int iBaseline) {
	aTradeGoods[GetLength(aTradeGoods)] = idGood;
	aBaselines[GetLength(aBaselines)] = iBaseline;
}

public func GetTradeGoods()     { return aTradeGoods; }
public func GetTradeBaselines() { return aBaselines;  }

public func GetMarketPrice(id idGood) {
	return GetMarketPriceAt(idGood, this());
}

public func BuyGood(id idGood, object pClonk) {
	return BuyGoodAt(idGood, pClonk, this());
}

public func SellGood(id idGood, object pClonk) {
	return SellGoodAt(idGood, pClonk, this());
}

/* ---- Production: seawater evaporation -> 1 SALT per Timer cycle ---- */

protected func Timer() {
	if (ContentsCount(SALT) >= 8) return 1;
	if (!ShoreContact()) return 1;
	CreateContents(SALT);
	return 1;
}

/* Seawater within reach of the pan's base and flank edges. Offsets are
   relative to the pan's center (shape spans x -14..+14, y -6..+6). */
private func ShoreContact() {
	var iWater = Material("Water");
	return GetMaterial(0, 10) == iWater
		|| GetMaterial(-17, 4) == iWater
		|| GetMaterial(17, 4) == iWater
		|| GetMaterial(-17, -2) == iWater
		|| GetMaterial(17, -2) == iWater;
}
