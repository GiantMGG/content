/*-- Cave Expedition goal: retrieve the Selenite Heart and sell it at homebase --*/

#strict
#include GOAL

local Sold;  // count of SeleniteHearts sold

public func IsFulfilled()
{
	return Sold > 0;
}

public func SellID() { return SLNH; }

public func PlayerHasSold(int plr, object obj)
{
	if (GetID(obj) == SellID())
	{
		Sold++;
		Local(plr)++;  // per-player tally (Oremine pattern)
	}
}

public func PlayerHasBought(int plr, object obj)
{
	if (GetID(obj) == SellID())
	{
		Sold--;
		Local(plr)--;
	}
}

protected func Activate(int iPlayer)
{
	if (IsFulfilled())
		return MessageWindow("$MsgGoalFulfilled$", iPlayer);
	return MessageWindow("$MsgGoalUnfulfilled$", iPlayer);
}
