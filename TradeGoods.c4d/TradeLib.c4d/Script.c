/*-- TradeLib (TRDL) -- shared trade-economy library. --*/
/* All functions are global; call from any stall or scenario script.    */
/* A stall must implement RegisterTradeGoodAt / GetTradeGoods /         */
/* GetTradeBaselines so TradeLib can track its registered goods.        */

#strict

/* ---- Price model ---- */

global func GetMarketPriceAt(id idGood, object pStall) {
	if (!pStall) return 0;
	var iBase = GetValue(0, idGood);
	var iStock = ContentsCount(idGood, pStall);
	var iBaseline = GetStockBaseline(idGood, pStall);
	return BoundBy(iBase * PriceFactor(iStock, iBaseline), 1, 9999);
}

global func PriceFactor(int iStock, int iBaseline) {
	if (iBaseline <= 0) return 1;
	var iDelta = iBaseline - iStock;
	return BoundBy(100 + (iDelta * 100) / iBaseline, 50, 200) / 100;
}

global func GetStockBaseline(id idGood, object pStall) {
	var aGoods = pStall->~GetTradeGoods();
	var aBases = pStall->~GetTradeBaselines();
	if (!aGoods) return 10;
	var iIdx = GetIndexOf(aGoods, idGood);
	if (iIdx < 0) return 10;
	return aBases[iIdx];
}

/* ---- Registration ---- */

global func RegisterTradeGood(id idGood, object pStall, int iBaseline) {
	if (!pStall) return;
	if (iBaseline <= 0) iBaseline = 10;
	pStall->~RegisterTradeGoodAt(idGood, iBaseline);
	for (var i = 0; i < iBaseline; i++)
		CreateContents(idGood, pStall);
}

/* ---- Restock (call from stall's Timer) ---- */

global func RestockTradeGoods(object pStall) {
	if (!pStall) return;
	var aGoods = pStall->~GetTradeGoods();
	var aBases = pStall->~GetTradeBaselines();
	if (!aGoods) return;
	for (var i = 0; i < GetLength(aGoods); i++) {
		var idGood = aGoods[i];
		var iBase = aBases[i];
		var iStock = ContentsCount(idGood, pStall);
		if (iStock < iBase) CreateContents(idGood, pStall);
		else if (iStock > iBase * 2) {
			var pExcess = FindContents(idGood, pStall);
			if (pExcess) RemoveObject(pExcess);
		}
	}
}

/* ---- Buy / Sell ---- */

global func BuyGoodAt(id idGood, object pClonk, object pStall) {
	var iPrice = GetMarketPriceAt(idGood, pStall);
	var iPlr = GetOwner(pClonk);
	if (iPlr < 0) iPlr = GetAnyPlayer();
	if (iPlr < 0) return false;
	if (GetWealth(iPlr) < iPrice) return false;
	var pObj = FindContents(idGood, pStall);
	if (!pObj) return false;
	DoWealth(iPlr, -iPrice);
	pObj->Exit();
	pClonk->Collect(pObj);
	return true;
}

global func SellGoodAt(id idGood, object pClonk, object pStall) {
	var iPrice = GetMarketPriceAt(idGood, pStall);
	var iPlr = GetOwner(pClonk);
	if (iPlr < 0) iPlr = GetAnyPlayer();
	if (iPlr < 0) return false;
	var pObj = FindContents(idGood, pClonk);
	if (!pObj) return false;
	RemoveObject(pObj);
	DoWealth(iPlr, +iPrice);
	return true;
}

/* ---- Caravan cargo value (mirrors Mule.c4d's GetCargoValue) ---- */

global func GetCargoValue(object pWagon) {
	if (!pWagon) return 0;
	var iValue = 0;
	var pObj;
	for (pObj in FindObjects(Find_Container(pWagon), Find_Func("IsTradeGood")))
		iValue += GetValue(pObj, 0, 0, true);
	return iValue;
}

/* ---- Helper: first valid player index, or -1 if none ---- */

global func GetAnyPlayer() {
	if (GetPlayerCount() <= 0) return -1;
	return GetPlayerByIndex(0);
}
