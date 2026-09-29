/*-- Goal_Siege.c4d -- goal-framework object so the siege objective shows
    on the goals board and the round-results screen. The SiegeDirector in
    SiegeOfHighKeep.c4s resolves the match (win/loss + GameOver); this goal
    only mirrors the outcome for the goals display. --*/

#strict 2
#include GOAL

local iWinner; // 0 unresolved, 1 attackers, 2 defenders (Scenario.txt teams)

protected func Initialize()
{
	iWinner = 0;
	return _inherited();
}

/* The SiegeDirector records the outcome (call: pGoal->SiegeEnded(team))
   so the goals board, objectives dialog and results screen show the
   correct fulfilled state. */
public func SiegeEnded(int iWinningTeam)
{
	iWinner = iWinningTeam;
	return true;
}

public func IsFulfilled()
{
	// Objective ("slay the King") is fulfilled when the attackers win.
	return iWinner == 1;
}

public func IsFulfilledforPlr(int iPlr)
{
	// Fulfilled for a player whose team won the siege.
	return iWinner > 0 && iWinner == GetPlayerTeam(iPlr);
}

protected func Activate(int iPlr)
{
	// Goals board: announce the objective, or the outcome once resolved.
	if (IsFulfilledforPlr(iPlr)) return MessageWindow("$MsgGoalFulfilled$", iPlr);
	return MessageWindow("$MsgGoalSiege$", iPlr);
}
