/*-- Handel --*/
/* Dynamic market via #appendto MTIP. Injects GetMarketPrice / BuyGood /  */
/* SellGood / Timer / ContainedUp into MTIP trade-post stalls. Uses       */
/* TradeLib globals from TradeGoods.c4d.                                  */

#strict

#appendto MTIP

/* ---- Per-stall registered-goods state ---- */

local aTradeGoods;
local aBaselines;

protected func Initialize() {
	aTradeGoods = [];
	aBaselines = [];
	return 1;
}

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

/* ---- Auto-restock timer ---- */

protected func Timer() {
	RestockTradeGoods(this());
}

/* ---- Market menu (dynamic buy/sell) ---- */

protected func ContainedUp(object pClonk) {
	CreateMenu(GetID(), pClonk, this(), C4MN_Extra_None, "$MsgTrade$", 0, C4MN_Style_Context);
	var aGoods = GetTradeGoods();
	for (var i = 0; i < GetLength(aGoods); i++) {
		var idGood = aGoods[i];
		var iPrice = GetMarketPrice(idGood);
		AddMenuItem(Format("{{%s}} $MsgBuy$ (%d)", C4IdText(idGood), iPrice),
			"BuyMenu", idGood, pClonk);
		AddMenuItem(Format("{{%s}} $MsgSell$ (%d)", C4IdText(idGood), iPrice),
			"SellMenu", idGood, pClonk);
	}
	AddMenuItem("$MsgCancel$", "Cancel", MCMX, pClonk);
	return 1;
}

public func BuyMenu(id idGood, object pClonk) {
	BuyGood(idGood, pClonk);
}

public func SellMenu(id idGood, object pClonk) {
	SellGood(idGood, pClonk);
}

public func Cancel() { return 1; }
