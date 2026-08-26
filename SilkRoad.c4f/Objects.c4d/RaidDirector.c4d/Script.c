/*-- Überfall-Regisseur --*/
/* RDDR rule: ticks every ~60s, spawns bandit waves scaled to in-transit
   cargo value, spawns a boss every 3rd wave, and checks the lose condition. */

#strict

local iWave;        // current wave number
local iBossEveryN;  // boss every Nth wave

protected func Initialize() {
	iWave = 0;
	iBossEveryN = 3;
	AddEffect("FxRaidTick", this(), 1, 2000, this());
	return 1;
}

/* ---- Timer effect ---- */

func FxRaidTickTimer(object pTarget, int fx) {
	// Sum cargo value across all active caravans.
	var iCargoValue = 0;
	var pMule;
	for (pMule in FindObjects(Find_ID(HORS), Find_Func("IsCaravanActive"))) {
		var pWagon = GetCaravanWagon(pMule);
		iCargoValue += GetCargoValue(pWagon);
	}

	// No cargo in transit? Skip the wave (no target).
	if (iCargoValue <= 0) return 1;

	// Wave size = cargoValue / 100, capped at 6.
	var iCount = BoundBy(iCargoValue / 100, 1, 6);
	iWave++;

	// Pick the ambush marker nearest the highest-value caravan.
	var pMarker = FindBestAmbushMarker();
	if (!pMarker) pMarker = FindObject(AMB1); // fallback: any marker
	if (!pMarker) return 1;                   // no markers at all -> skip

	SpawnBanditWave(iCount, pMarker);

	// Boss every Nth wave.
	if (iWave % iBossEveryN == 0) {
		var pBoss = CreateObject(BNDT, GetX(pMarker) + 20, GetY(pMarker) - 30, NO_OWNER);
		pBoss->SetAI("BanditRevolver", 3);
		pBoss->MakeBoss();
		pBoss->SetColorDw(RGB(150));
	}
	return 1;
}

/* ---- Wave spawning ---- */

global func SpawnBanditWave(int iCount, object pMarker) {
	var iX = GetX(pMarker);
	var iY = GetY(pMarker);
	for (var i = 0; i < iCount; i++) {
		var pBandit = CreateObject(BNDT, iX + Random(40) - 20, iY - Random(30), NO_OWNER);
		pBandit->SetAI("BanditNoMove", 3);
		pBandit->SetColorDw(RGB(150));
		// Prioritise attacking the nearest caravan.
		var pCaravan = GetNearestCaravan(iX, iY);
		if (pCaravan) pBandit->SetCommand("Attack", pCaravan);
	}
}

/* ---- Lose condition check (called from Script.c after each wave ends) ---- */

global func CheckRaidDefeat() {
	var pDirector = FindObject(RDDR);
	if (!pDirector) return;
	// Any active caravan left? Then not defeated.
	var pMule;
	for (pMule in FindObjects(Find_ID(HORS), Find_Func("IsCaravanActive")))
		return;
	// No caravans. If pooled wealth < 100, no rebuild possible -> game over.
	var iPlr = GetAnyPlayer();
	if (iPlr >= 0 && GetWealth(iPlr) < 100)
		GameOver();
}

/* ---- Helpers ---- */

private func FindBestAmbushMarker() {
	// Find the caravan with the highest cargo value, then the nearest ambush marker to it.
	var pBestMule = 0;
	var iBestValue = -1;
	var pMule;
	for (pMule in FindObjects(Find_ID(HORS), Find_Func("IsCaravanActive"))) {
		var pWagon = GetCaravanWagon(pMule);
		var iVal = GetCargoValue(pWagon);
		if (iVal > iBestValue) { iBestValue = iVal; pBestMule = pMule; }
	}
	if (!pBestMule) return 0;
	var iX = GetX(pBestMule);
	// Nearest AMB1 marker to iX.
	var pBest = 0, iBestDist = 999999;
	var pMarker;
	for (pMarker in FindObjects(Find_ID(AMB1))) {
		var iDist = Abs(GetX(pMarker) - iX);
		if (iDist < iBestDist) { iBestDist = iDist; pBest = pMarker; }
	}
	return pBest;
}
