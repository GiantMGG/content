/*-- CargoShipCaravan (CSHP) -- maritime caravan API. --*/
/* Mirrors Mule.c4d's FxCaravanAI but drives a CGSH Cog along waypoint  */
/* X coordinates, buying SILK at the west post and selling at the east.  */
/* NOTE: do NOT redefine GetCargoValue -- TradeLib.c4d already defines  */
/* it (spec edge case #8).                                               */

#strict

protected func Initialize() { return 1; }

/* ---- Global caravan API ---- */
global func StartSeaCaravan(object pCog, object pWestPost, object pEastPost, array aWaypoints) {
	if (!pCog) return;
	var fx = AddEffect("FxSeaCaravanAI", pCog, 1, 35, pCog);
	EffectVar(0, pCog, fx) = pWestPost;
	EffectVar(1, pCog, fx) = pEastPost;
	EffectVar(2, pCog, fx) = aWaypoints;
	EffectVar(3, pCog, fx) = 0;  // current waypoint index
	EffectVar(4, pCog, fx) = 0;  // stop-until frame counter
	EffectVar(5, pCog, fx) = 0;  // loaded flag (0=empty, 1=loaded)
}

global func IsSeaCaravanActive() {
	return GetEffect("FxSeaCaravanAI", this) != 0;
}

/* ---- CaravanAI effect (lives on the Cog) ---- */
global func FxSeaCaravanAITimer(object pTarget, int fx) {
	var iStopUntil = EffectVar(4, pTarget, fx);
	if (FrameCounter() < iStopUntil) return 1;

	var aWP   = EffectVar(2, pTarget, fx);
	var iWP   = EffectVar(3, pTarget, fx);
	var iTargetX = aWP[iWP];

	// Arrived at waypoint (within 30px)?
	if (Abs(GetX(pTarget) - iTargetX) < 30) {
		var pWest = EffectVar(0, pTarget, fx);
		var pEast = EffectVar(1, pTarget, fx);
		var bLoaded = EffectVar(5, pTarget, fx);
		if (!bLoaded) {
			BuyGoodAt(SILK, pTarget, pWest);
			EffectVar(5, pTarget, fx) = 1;
		} else {
			SellGoodAt(SILK, pTarget, pEast);
			EffectVar(5, pTarget, fx) = 0;
			// Register a delivery with the StormLaneDirector.
			var pDir = FindObject(STLD);
			if (pDir) pDir->RegisterDelivery();
		}
		// Advance waypoint (cycle).
		iWP = (iWP + 1) % GetLength(aWP);
		EffectVar(3, pTarget, fx) = iWP;
		// Pause ~5s (150 frames) for loading/unloading.
		EffectVar(4, pTarget, fx) = FrameCounter() + 150;
		SetCommand(pTarget, "None");
		return 1;
	}

	// En-route: drive toward next waypoint.
	if (GetCommand(pTarget) ne "MoveTo")
		SetCommand(pTarget, "MoveTo", 0, iTargetX, GetY(pTarget));
	return 1;
}
