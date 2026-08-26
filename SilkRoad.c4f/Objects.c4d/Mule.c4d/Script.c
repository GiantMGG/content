/*-- Maultier-Karawane --*/
/* MULE rule: provides global caravan helpers + FxCaravanAI effect. */

#strict

protected func Initialize() { return 1; }

/* ---- Global caravan API (called from Script.c) ---- */

// Attach a CaravanAI to pTarget (the horse). aWaypoints is an array of x coords
// the horse will cycle through. pWagon is the connected COAC wagon.
global func StartCaravan(object pTarget, object pWagon, array aWaypoints) {
	if (!pTarget || !pWagon) return;
	var fx = AddEffect("FxCaravanAI", pTarget, 1, 35, pTarget);
	EffectVar(0, pTarget, fx) = pWagon;
	EffectVar(1, pTarget, fx) = aWaypoints;
	EffectVar(2, pTarget, fx) = 0; // current waypoint index
	EffectVar(3, pTarget, fx) = 0; // stop-until frame counter
}

global func IsCaravanActive() {
	return GetEffect("FxCaravanAI", this) != 0;
}

global func GetCaravanWagon(object pTarget) {
	var fx = GetEffect("FxCaravanAI", pTarget);
	if (!fx) return 0;
	return EffectVar(0, pTarget, fx);
}

// Sum of GetValue() over trade goods in the wagon — used by RaidDirector.
global func GetCargoValue(object pWagon) {
	if (!pWagon) return 0;
	var iValue = 0;
	var pObj;
	for (pObj in FindObjects(Find_Container(pWagon), Find_Func("IsTradeGood")))
		iValue += GetValue(pObj, 0, 0, true);
	return iValue;
}

global func GetNearestCaravan(int iX, int iY) {
	var pBest = 0, iBestDist = 999999;
	var pMule;
	for (pMule in FindObjects(Find_ID(HORS), Find_Func("IsCaravanActive"))) {
		var iDist = Abs(GetX(pMule) - iX) + Abs(GetY(pMule) - iY);
		if (iDist < iBestDist) { iBestDist = iDist; pBest = pMule; }
	}
	return pBest;
}

/* ---- CaravanAI effect (lives on the horse) ---- */

global func FxCaravanAITimer(object pTarget, int fx) {
	// Player riding? Let them drive.
	if (pTarget->GetRider()) return 1;

	var pWagon = EffectVar(0, pTarget, fx);
	if (!pWagon) return -1; // wagon destroyed -> end effect

	// Pausing at a post?
	var iStopUntil = EffectVar(3, pTarget, fx);
	if (FrameCounter() < iStopUntil) return 1;

	var aWaypoints = EffectVar(1, pTarget, fx);
	var iWP = EffectVar(2, pTarget, fx);
	var iTargetX = aWaypoints[iWP];

	// Arrived at waypoint (within 30px)?
	if (Abs(GetX(pTarget) - iTargetX) < 30) {
		// Find the MTIP trade post near this x.
		var pPost = FindObject(MTIP, iTargetX - 60, 0, 120, LandscapeHeight());
		if (pPost) DoCaravanTrade(pPost, pWagon);
		// Advance waypoint (wrap around to cycle continuously).
		iWP = (iWP + 1) % GetLength(aWaypoints);
		EffectVar(2, pTarget, fx) = iWP;
		// Pause ~4s (150 frames) at the post for loading/unloading.
		EffectVar(3, pTarget, fx) = FrameCounter() + 150;
		SetCommand(pTarget, "None");
		return 1;
	}

	// En-route: ensure a MoveTo command toward the next waypoint is active.
	if (GetCommand(pTarget) ne "MoveTo")
		SetCommand(pTarget, "MoveTo", 0, iTargetX, GetY(pTarget));
	return 1;
}

global func FxCaravanAIStop(object pTarget, int fx, int iReason) {
	// Caravan destroyed mid-route: nothing else to clean (wagon is independent).
	return 1;
}
