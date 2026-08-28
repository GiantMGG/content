/*-- StormLane -- Phase-1 playable scenario. --*/
/* One west island (producer port, sells SILK) and one east island       */
/* (consumer port, buys SILK). A storm-prone sea lane runs between them. */
/* The Cog auto-runs the lane via a FxCogRun effect (spec lines 226-228):*/
/* buy SILK at west -> drive east -> sell at east -> register delivery   */
/* with STLD -> repeat. PirateSloops patrol and grapple.                 */

#strict

static pWestPost, pEastPost, pCog, pEscort;

func Initialize() {
	SetWind(0);

	var iW = LandscapeWidth();
	var iWestX = iW / 6;
	var iEastX = 5 * iW / 6;
	var iGroundY = LandscapeHeight() - 120;

	// 2 island MTIP posts (west=producer SILK, east=consumer).
	// MKTS (MarketStall) has no ActMap, so no SetAction("Wait") is needed
	// (unlike SilkRoad's MTIP Tipi, which has a Construction action).
	pWestPost = CreateObject(MKTS, iWestX, iGroundY, NO_OWNER);
	pEastPost = CreateObject(MKTS, iEastX, iGroundY, NO_OWNER);
	RegisterTradeGood(SILK, pWestPost, 10);
	RegisterTradeGood(SILK, pEastPost, 10);

	// Player Cog (CGSH) at west dock; escort Sailboat.
	pCog = CreateObject(CGSH, iWestX + 40, iGroundY - 20, NO_OWNER);
	pEscort = CreateObject(SLBT, iWestX + 80, iGroundY - 20, NO_OWNER);

	// 2 PirateSloops on patrol across the lane.
	CreateObject(PSLP, iW / 2, iGroundY - 40, NO_OWNER)->StartPatrol([iWestX, iEastX]);
	CreateObject(PSLP, iW / 2 + 60, iGroundY - 40, NO_OWNER)->StartPatrol([iWestX, iEastX]);

	// StormLaneDirector rule (STLD).
	CreateObject(STLD, 50, 50, NO_OWNER);

	// Auto-run the Cog along the west<->east lane.
	var fx = AddEffect("FxCogRun", pCog, 1, 35, pCog);
	EffectVar(0, pCog, fx) = pWestPost;
	EffectVar(1, pCog, fx) = pEastPost;
	EffectVar(2, pCog, fx) = 0;  // loaded flag (0=empty, 1=loaded)

	// Defeat-poll effect: every 200 frames, run CheckStormLaneDefeat.
	AddEffect("FxDefeatPoll", this(), 1, 200, this());
	return 1;
}

/* ---- Cog auto-run: buy at west, sell at east, register delivery ---- */
func FxCogRunTimer(object pCog, int fx) {
	var pWest = EffectVar(0, pCog, fx);
	var pEast = EffectVar(1, pCog, fx);
	var bLoaded = EffectVar(2, pCog, fx);

	var iTargetX;
	if (!bLoaded) {
		// Empty: head to west to buy.
		iTargetX = GetX(pWest);
		if (Abs(GetX(pCog) - GetX(pWest)) < 30) {
			BuyGoodAt(SILK, pCog, pWest);
			EffectVar(2, pCog, fx) = 1;
		}
	} else {
		// Loaded: head to east to sell.
		iTargetX = GetX(pEast);
		if (Abs(GetX(pCog) - GetX(pEast)) < 30) {
			SellGoodAt(SILK, pCog, pEast);
			EffectVar(2, pCog, fx) = 0;
			var pDir = FindObject(STLD);
			if (pDir) pDir->RegisterDelivery();
		}
	}
	if (GetCommand(pCog) ne "MoveTo")
		SetCommand(pCog, "MoveTo", 0, iTargetX, GetY(pCog));
	return 1;
}

func FxDefeatPollTimer(object pTarget, int fx) {
	var pDir = FindObject(STLD);
	if (pDir) pDir->CheckStormLaneDefeat();
	return 1;
}
