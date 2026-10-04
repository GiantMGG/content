/*-- SilkRoadOfTheSea -- Phase-2 playable scenario. --*/
/* Three island MTIP ports -- east / center / west. Two Cog convoys    */
/* cycle the lane; PirateDirector spawns pirate waves scaled to the    */
/* in-transit cargo value; a storm timer periodically storms the sea.  */

#strict

static pEastPost, pCenterPost, pWestPost;

func Initialize() {
	SetWind(0);

	var iW = LandscapeWidth();
	var iEastX   = iW / 6;
	var iCenterX = iW / 2;
	var iWestX   = 5 * iW / 6;
	var iGroundY = LandscapeHeight() - 120;

	// 3 island MTIP posts.
	// MKTS (MarketStall) has no ActMap, so no SetAction("Wait") is needed.
	pEastPost   = CreateObject(MKTS, iEastX,   iGroundY, NO_OWNER);
	pCenterPost = CreateObject(MKTS, iCenterX, iGroundY, NO_OWNER);
	pWestPost   = CreateObject(MKTS, iWestX,   iGroundY, NO_OWNER);

	// Register trade goods per port.
	RegisterTradeGood(SILK, pEastPost, 10);
	RegisterTradeGood(GLDN, pEastPost, 10);
	RegisterTradeGood(SPIC, pCenterPost, 10);
	RegisterTradeGood(INCN, pCenterPost, 10);
	RegisterTradeGood(SILK, pWestPost, 10);
	RegisterTradeGood(SPIC, pWestPost, 10);

	// --- Salt Pan: the center port's SALT source (cycle-193 chain). ---
	PlaceSaltPanNear(iCenterX);

	// 3 SeaLaneMarkers at sea edges + mid-lane.
	CreateObject(SLMR, (iEastX + iCenterX) / 2, iGroundY - 80, NO_OWNER);
	CreateObject(SLMR, iCenterX, iGroundY - 80, NO_OWNER);
	CreateObject(SLMR, (iCenterX + iWestX) / 2, iGroundY - 80, NO_OWNER);

	// Rules: CargoShipCaravan (CSHP) + PirateDirector (PRDR).
	CreateObject(CSHP, 50, 50, NO_OWNER);
	CreateObject(PRDR, 50, 50, NO_OWNER);

	// Storm timer (~3000 frames), guarded against overlap.
	AddEffect("FxStormTimer", this(), 1, 3000, this());

	// Start 2 Cog convoys. Each convoy cycles west<->east.
	// (The center port is registered for manual trade but the auto-convoys
	// run the west<->east lane -- a documented simplification of the spec's
	// 3-port cycling ideal; full balance is the manual playtest gate.)
	var pCog1 = CreateObject(CGSH, iWestX + 40, iGroundY - 20, NO_OWNER);
	var pCog2 = CreateObject(CGSH, iEastX + 40, iGroundY - 20, NO_OWNER);
	StartSeaCaravan(pCog1, pWestPost, pEastPost, [GetX(pWestPost), GetX(pEastPost)]);
	StartSeaCaravan(pCog2, pWestPost, pEastPost, [GetX(pEastPost), GetX(pWestPost)]);

	// Defeat-poll effect: every 200 frames, run CheckSeaTradeDefeat.
	AddEffect("FxDefeatPoll", this(), 1, 200, this());
	return 1;
}

func FxStormTimerTimer(object pTarget, int fx) {
	// Spec edge case #10: skip if a weather event is already active.
	if (GetActiveWeatherEvent() != 0) return 1;
	LaunchWeatherEvent(STRM, 50, 200);
	return 1;
}

func FxDefeatPollTimer(object pTarget, int fx) {
	var pDir = FindObject(PRDR);
	if (pDir) pDir->CheckSeaTradeDefeat();
	return 1;
}

/* ---- Salt Pan placement: first shoreline within 200px of iCx ---- */
/* Bench a starter SaltPan where solid ground sits directly beside  */
/* surface water, and register it as the SALT vendor.               */
private func PlaceSaltPanNear(int iCx) {
	var iWater = Material("Water");
	var pPan = 0, iX, iY;
	for (var iDir = -1; iDir <= 1; iDir += 2) {
		for (var iOff = 8; iOff <= 200; iOff += 8) {
			iX = iCx + iDir * iOff;
			iY = 20;
			// SilkRoadOfTheSea's landscape is 1600x800 (MapWidth=160 x
			// MapZoom=10); the posts sit near y = height-120, so scan deep.
			while (iY < 700 && !GBackSolid(iX, iY)) iY++;
			if (iY >= 700) continue;
			if (GetMaterial(iX + 18, iY) == iWater) {
				pPan = CreateObject(SLTP, iX, iY - 6, NO_OWNER);
				break;
			}
		}
		if (pPan) break;
	}
	if (!pPan) return 0;
	RegisterTradeGood(SALT, pPan, 8);
	return pPan;
}
