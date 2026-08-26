/*-- Handel --*/
/* Ported from Western.c4f/Goldrush.c4s/System.c4g/Trade.c.
   #appendto MTIP adds the barter menu to MTIP trade-post stalls.
   Post role is Local(0, this): 0=east, 1=center, 2=west. */

#strict

#appendto MTIP

protected func ContainedUp(object pClonk) {
	// Build the barter menu. Each post offers only its own conversions.
	CreateMenu(GetID(), pClonk, this(), C4MN_Extra_None, "$MsgTrade$", 0, C4MN_Style_Context);
	var iRole = Local(0);
	if (iRole == 1) {
		// Center refiner.
		AddMenuItem("{{SILK}} $MsgRefineSilk$", "Trade(1)", SILK, pClonk);
		AddMenuItem("{{GLDN}} $MsgRefineGold$", "Trade(2)", GLDN, pClonk);
	} else if (iRole == 2) {
		// West post buys Spice.
		AddMenuItem("{{SPIC}} $MsgSellSpice$", "Trade(3)", SPIC, pClonk);
	} else {
		// East post buys Incense.
		AddMenuItem("{{INCN}} $MsgSellIncense$", "Trade(4)", INCN, pClonk);
	}
	AddMenuItem("$MsgCancel$", "Cancel", MCMX, pClonk);
	return 1;
}

public func ContainedDig(object pClonk) { }
protected func ContextJoinClan(object pClonk) { [Condition=Never] }
protected func ContextClanInfo(object pClonk) { [Condition=Never] }
public func Never() { return 0; }
public func Cancel() { return 1; }

func Trade(int iIndex) {
	// Consume raw goods from this stall's contents, produce refined goods / wealth.
	if (iIndex == 1) {
		// SILK x5 -> SPIC x5 + 50 wealth.
		if (ContentsCount(SILK) < 5) return Message("$MsgTooFewGoods$", this());
		for (var i = 0; i < 5; i++) RemoveObject(FindContents(SILK));
		CreateContents(SPIC, this(), 5);
		var iPlr = GetAnyPlayer();
		if (iPlr >= 0) DoWealth(iPlr, 50);
		Sound("Cash");
		return Message("$MsgSilkRefine$", this());
	}
	if (iIndex == 2) {
		// GLDN x5 -> INCN x5 + 50 wealth.
		if (ContentsCount(GLDN) < 5) return Message("$MsgTooFewGoods$", this());
		for (var i = 0; i < 5; i++) RemoveObject(FindContents(GLDN));
		CreateContents(INCN, this(), 5);
		var iPlr = GetAnyPlayer();
		if (iPlr >= 0) DoWealth(iPlr, 50);
		Sound("Cash");
		return Message("$MsgGoldRefined$", this());
	}
	if (iIndex == 3) {
		// SPIC x5 -> +50 wealth.
		if (ContentsCount(SPIC) < 5) return Message("$MsgTooFewGoods$", this());
		for (var i = 0; i < 5; i++) RemoveObject(FindContents(SPIC));
		var iPlr = GetAnyPlayer();
		if (iPlr >= 0) DoWealth(iPlr, 50);
		Sound("Cash");
		return Message("$MsgSpiceSold$", this());
	}
	if (iIndex == 4) {
		// INCN x5 -> +50 wealth.
		if (ContentsCount(INCN) < 5) return Message("$MsgTooFewGoods$", this());
		for (var i = 0; i < 5; i++) RemoveObject(FindContents(INCN));
		var iPlr = GetAnyPlayer();
		if (iPlr >= 0) DoWealth(iPlr, 50);
		Sound("Cash");
		return Message("$MsgIncenseSold$", this());
	}
}
