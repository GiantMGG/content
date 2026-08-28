/*-- StormLaneDirector (STLD) -- Phase-1 rule. --*/
/* Launches a STRM storm every ~2500 frames (guarded against overlap).  */
/* Tracks completed deliveries and ends the game on win/lose.           */

#strict

local iDeliveries;
local iGoalDeliveries;

protected func Initialize() {
	iDeliveries = 0;
	iGoalDeliveries = 3;
	AddEffect("FxStormTick", this, 1, 2500, this);
	return 1;
}

/* ---- Storm timer ---- */
func FxStormTickTimer(object pTarget, int fx) {
	// Spec edge case #10: skip if a weather event is already active.
	if (GetActiveWeatherEvent() != nil) return 1;
	LaunchWeatherEvent(STRM, 50, 200);
	return 1;
}

/* ---- Called by the scenario when a Cog completes a delivery ---- */
public func RegisterDelivery() {
	iDeliveries++;
	if (iDeliveries >= iGoalDeliveries) {
		Log("StormLane WIN: %d deliveries complete.", iDeliveries);
		GameOver();
	}
	return 1;
}

/* ---- Lose condition (called from scenario poll) ---- */
public func CheckStormLaneDefeat() {
	// Any Cog left? Then not defeated.
	var pCog;
	for (pCog in FindObjects(Find_ID(CGSH)))
		return;
	// No Cogs. If pooled wealth < 100, game over.
	var iPlr = GetAnyPlayer();
	if (iPlr >= 0 && GetWealth(iPlr) < 100)
		GameOver();
}
