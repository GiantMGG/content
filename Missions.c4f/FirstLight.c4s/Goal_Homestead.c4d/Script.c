/*-- Goal_Homestead.c4d -- the Homestead Ledger goal (cycle 127). --*/
/* IsFulfilled delegates to the scenario's ledger-audit state: the    */
/* ledger counts green only after two consecutive green audits.      */

#strict 2

#include GOAL

protected func Initialize()
{
	SetAction("Idle");
	return _inherited();
}

/* Ist das Ziel erfuellt? */

public func IsFulfilled()
{
	return HomesteadLedgerFulfilled();
}

protected func Activate(int iPlr)
{
	if (IsFulfilled())
		return MessageWindow("$MsgGoalFulfilled$", iPlr);
	return MessageWindow("$MsgGoalDesc$", iPlr);
}
