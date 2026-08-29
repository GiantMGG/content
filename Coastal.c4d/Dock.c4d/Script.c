/*-- Dock (DKST) -- mooring post + MarketStall-compatible trade post. --*/
/* Implements the MarketStall trade contract (RegisterTradeGoodAt/          */
/* GetTradeGoods/GetTradeBaselines) so TradeLib treats it as a stall.       */
/* CanMooreShip() is tide-gated (high tide only); with no active TIDE event */
/* it always returns true (spec edge case: dock works without the tide).    */
/* BuyGood/SellGood increment the throughput tally that HarborMaster sums.  */

#strict

local aTradeGoods;
local aBaselines;
local pMooredShip;
local iThroughput;
local iDockThreshold;

protected func Initialize()
{
	aTradeGoods = [];
	aBaselines = [];
	pMooredShip = 0;
	iThroughput = 0;
	iDockThreshold = GetY() + 10;
	return 1;
}

/* ---- MarketStall contract ---- */

public func RegisterTradeGoodAt(id idGood, int iBaseline)
{
	aTradeGoods[GetLength(aTradeGoods)] = idGood;
	aBaselines[GetLength(aBaselines)] = iBaseline;
}

public func GetTradeGoods()     { return aTradeGoods; }
public func GetTradeBaselines() { return aBaselines;  }

public func GetMarketPrice(id idGood)
{
	return GetMarketPriceAt(idGood, this());
}

public func BuyGood(id idGood, object pClonk)
{
	var bOk = BuyGoodAt(idGood, pClonk, this());
	if (bOk) RegisterShipment();
	return bOk;
}

public func SellGood(id idGood, object pClonk)
{
	var bOk = SellGoodAt(idGood, pClonk, this());
	if (bOk) RegisterShipment();
	return bOk;
}

/* ---- Auto-restock timer (MarketStall pattern) ---- */

protected func Timer()
{
	RestockTradeGoods(this());
}

/* ---- Mooring post ---- */

public func CanMooreShip(object pCog)
{
	// No active tide -> always moorable (graceful when TIDE absent).
	if (!FindObject(TIDE)) return 1;
	// Tide active: moor only on high tide.
	return IsHighTide();
}

public func MooreShip(object pCog)
{
	pMooredShip = pCog;
	iThroughput++;
}

public func GetMooredShip() { return pMooredShip; }

/* ---- Throughput tally (HarborMaster sums this) ---- */

public func RegisterShipment()
{
	iThroughput++;
	return iThroughput;
}

public func GetThroughput() { return iThroughput; }

/* ---- Edge case #2: dock destroyed while a ship is moored ---- */

protected func Destruction()
{
	if (pMooredShip)
	{
		pMooredShip->SetCommand("None");
		pMooredShip = 0;
	}
	return 1;
}
