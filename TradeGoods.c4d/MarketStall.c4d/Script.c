/*-- MarketStall (MKTS) -- standalone trade stall. --*/
/* Uses TradeLib globals for the price-supply economy.  */

#strict

local aTradeGoods;
local aBaselines;

protected func Initialize() {
	aTradeGoods = [];
	aBaselines = [];
	return 1;
}

/* ---- Registration accessors (called by TradeLib) ---- */

public func RegisterTradeGoodAt(id idGood, int iBaseline) {
	aTradeGoods[GetLength(aTradeGoods)] = idGood;
	aBaselines[GetLength(aBaselines)] = iBaseline;
}

public func GetTradeGoods()     { return aTradeGoods; }
public func GetTradeBaselines() { return aBaselines;  }

/* ---- Price model (delegate to TradeLib) ---- */

public func GetMarketPrice(id idGood) {
	return GetMarketPriceAt(idGood, this());
}

/* ---- Buy / Sell (delegate to TradeLib) ---- */

public func BuyGood(id idGood, object pClonk) {
	return BuyGoodAt(idGood, pClonk, this());
}

public func SellGood(id idGood, object pClonk) {
	return SellGoodAt(idGood, pClonk, this());
}

/* ---- Timer stub (cycle 193: void-refill removed; goods produce at ---- */
/* ---- source -- SaltPan / scenario producer effects). DefCore Timer  ---- */
/* ---- entries stay per spec.                                          ---- */
protected func Timer() {}
