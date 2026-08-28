/*-- Seidenstraße --*/
/* Cooperative 2-team trade-route scenario. */

#strict

static pEastPost, pCenterPost, pWestPost;

func Initialize() {
	SetWind(0);

	// --- 3 Trade Posts (MTIP stalls) at east / center / west. ---
	// Role flag Local(0): 0=east, 1=center, 2=west.
	var iW = LandscapeWidth();
	var iEastX = iW / 6;
	var iCenterX = iW / 2;
	var iWestX = 5 * iW / 6;
	var iGroundY = LandscapeHeight() - 120;

	pEastPost   = CreateObject(MTIP, iEastX,   iGroundY, NO_OWNER);
	pCenterPost = CreateObject(MTIP, iCenterX, iGroundY, NO_OWNER);
	pWestPost   = CreateObject(MTIP, iWestX,   iGroundY, NO_OWNER);
	// Finalize posts: CreateObject leaves them in "Construction" action;
	// set to "Wait" (the built-idle action) so they're functional containers.
	pEastPost->SetAction("Wait");
	pCenterPost->SetAction("Wait");
	pWestPost->SetAction("Wait");
	Local(0, pEastPost)   = 0;
	Local(0, pCenterPost) = 1;
	Local(0, pWestPost)   = 2;

	// --- Register trade goods at each post (seeds stock + baseline) ---
	RegisterTradeGood(SILK, pEastPost, 10);
	RegisterTradeGood(GLDN, pEastPost, 10);
	RegisterTradeGood(SPIC, pCenterPost, 10);
	RegisterTradeGood(INCN, pCenterPost, 10);
	RegisterTradeGood(SILK, pWestPost, 10);
	RegisterTradeGood(SPIC, pWestPost, 10);

	// --- Team chests + respawners per settlement. ---
	CreateObject(TCHS, iEastX, iGroundY, NO_OWNER);
	CreateObject(TCHS, iWestX, iGroundY, NO_OWNER);
	CreateObject(RSPN, iEastX, iGroundY, NO_OWNER);
	CreateObject(RSPN, iWestX, iGroundY, NO_OWNER);

	// --- 3 Ambush markers along the route. ---
	CreateObject(AMB1, (iEastX + iCenterX) / 2, iGroundY, NO_OWNER);
	CreateObject(AMB1, iCenterX, iGroundY - 80, NO_OWNER);
	CreateObject(AMB1, (iCenterX + iWestX) / 2, iGroundY, NO_OWNER);

	// --- Boundary objects at map edges. ---
	CreateObject(BNDR, 0, 0, NO_OWNER);
	CreateObject(BNDR, iW, 0, NO_OWNER);
	CreateObject(BNDR, 0, LandscapeHeight(), NO_OWNER);
	CreateObject(BNDR, iW, LandscapeHeight(), NO_OWNER);

	// --- Rule objects: MULE (caravan API) + RDDR (raid director). ---
	CreateObject(MULE, 50, 50, NO_OWNER);
	CreateObject(RDDR, 50, 50, NO_OWNER);

	// --- Start 2 caravans: east-start and west-start. ---
	// Each caravan cycles waypoints: home -> center -> opposite -> center -> home ...
	StartNewCaravan(iEastX, iGroundY, [iCenterX, iWestX, iCenterX, iEastX]);
	StartNewCaravan(iWestX, iGroundY, [iCenterX, iEastX, iCenterX, iWestX]);

	// --- Production timers: spawn 1 raw good per producer post every 70 frames. ---
	AddEffect("FxProduce", pEastPost, 1, 70, pEastPost);
	AddEffect("FxProduce", pWestPost, 1, 70, pWestPost);

	// --- Defeat-check poll: every 200 frames, run CheckRaidDefeat. ---
	AddEffect("FxDefeatPoll", this(), 1, 200, this());

	return 1;
}

/* ---- Caravan factory ---- */

func StartNewCaravan(int iX, int iY, array aWaypoints) {
	var pWagon = CreateObject(COAC, iX + 40, iY, NO_OWNER);
	var pHorse = CreateObject(HORS, iX, iY, NO_OWNER);
	if (pHorse && pWagon) {
		pHorse->ConnectWagon(pWagon);
		SetDir(1, pWagon);
		SetDir(1, pHorse);
		StartCaravan(pHorse, pWagon, aWaypoints);
	}
	return pHorse;
}

/* ---- Player setup ---- */

func InitializePlayer(int iPlr, int x, int y, object pBase, int iTeam) {
	// Route by team to the matching settlement.
	if (iTeam == 1) PlaceAtEast(iPlr);
	else if (iTeam == 2) PlaceAtWest(iPlr);
	return 1;
}

private func PlaceAtEast(int iPlr) {
	var iW = LandscapeWidth();
	var iX = iW / 6;
	var iY = LandscapeHeight() - 120;
	for (var i = 0; i < GetCrewCount(iPlr); i++) {
		var pCrew = GetCrew(iPlr, i);
		SetPosition(iX + Random(40) - 20, iY - 20, pCrew);
	}
	Log("$TeamEastJoin$", GetPlayerName(iPlr));
}

private func PlaceAtWest(int iPlr) {
	var iW = LandscapeWidth();
	var iX = 5 * iW / 6;
	var iY = LandscapeHeight() - 120;
	for (var i = 0; i < GetCrewCount(iPlr); i++) {
		var pCrew = GetCrew(iPlr, i);
		SetPosition(iX + Random(40) - 20, iY - 20, pCrew);
	}
	Log("$TeamWestJoin$", GetPlayerName(iPlr));
}

/* ---- Shared-economy alliance hook ---- */
// Returning 1 makes TACC form the alliance cleanly on scenario start
// instead of mid-game (template: Frontier.c4s/Script.c:26).
func TACC_BlockFirstAllianceAdd() { return 1; }

/* ---- Production effect: spawn raw goods at the producer post ---- */

func FxProduceTimer(object pTarget, int fx) {
	// Local(0) on the MTIP post = role. 0=east produces SILK; 2=west produces GLDN.
	var iRole = Local(0, pTarget);
	var idGood = 0;
	if (iRole == 0) idGood = SILK;
	else if (iRole == 2) idGood = GLDN;
	if (!idGood) return 1;
	// Cap stock at 15 goods in the post so production doesn't flood.
	if (ContentsCount(idGood, pTarget) >= 15) return 1;
	CreateContents(idGood, pTarget);
	return 1;
}

/* ---- Defeat-poll effect ---- */

func FxDefeatPollTimer(object pTarget, int fx) {
	CheckRaidDefeat();
	return 1;
}
