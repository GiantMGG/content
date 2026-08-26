/*-- Hilfsfunktionen --*/

#strict

// First valid player index, or -1 if none. DoWealth targets this; TACC pools.
global func GetAnyPlayer() {
	if (GetPlayerCount() <= 0) return -1;
	return GetPlayerByIndex(0);
}

// Count items of idDef inside pWagon.
global func CountInWagon(object pWagon, id idDef) {
	var iCount = 0;
	var pObj;
	for (pObj in FindObjects(Find_Container(pWagon), Find_ID(idDef))) iCount++;
	return iCount;
}

// Remove up to iCount items of idDef from pWagon.
global func RemoveFromWagon(object pWagon, id idDef, int iCount) {
	var pObj;
	for (pObj in FindObjects(Find_Container(pWagon), Find_ID(idDef))) {
		RemoveObject(pObj);
		if (--iCount <= 0) break;
	}
}

// Auto-barter a caravan's wagon at a trade post.
// pPost role is read from Local(0, pPost): 0=east, 1=center, 2=west.
global func DoCaravanTrade(object pPost, object pWagon) {
	if (!pPost || !pWagon) return;
	var iRole = Local(0, pPost);
	var iPlr = GetAnyPlayer();

	if (iRole == 1) {
		// Center refiner.
		if (CountInWagon(pWagon, SILK) >= 5) {
			RemoveFromWagon(pWagon, SILK, 5);
			CreateContents(SPIC, pWagon, 5);
			if (iPlr >= 0) DoWealth(iPlr, 50);
			Sound("Cash", 0, pWagon);
		}
		if (CountInWagon(pWagon, GLDN) >= 5) {
			RemoveFromWagon(pWagon, GLDN, 5);
			CreateContents(INCN, pWagon, 5);
			if (iPlr >= 0) DoWealth(iPlr, 50);
			Sound("Cash", 0, pWagon);
		}
	} else if (iRole == 2) {
		// West post buys Spice.
		if (CountInWagon(pWagon, SPIC) >= 5) {
			RemoveFromWagon(pWagon, SPIC, 5);
			if (iPlr >= 0) DoWealth(iPlr, 50);
			Sound("Cash", 0, pWagon);
		}
	} else {
		// East post (role 0) buys Incense.
		if (CountInWagon(pWagon, INCN) >= 5) {
			RemoveFromWagon(pWagon, INCN, 5);
			if (iPlr >= 0) DoWealth(iPlr, 50);
			Sound("Cash", 0, pWagon);
		}
	}
}
