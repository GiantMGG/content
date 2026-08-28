/*-- PirateDirector (PRDR) -- Phase-2 rule. --*/
/* Ticks every ~2000 frames, spawns PirateSloop waves scaled to the    */
/* total cargo value of in-transit Cogs, and spawns a boss every 3rd   */
/* wave. Mirrors RaidDirector.c4d.                                      */

#strict

local iWave;
local iBossEveryN;

protected func Initialize() {
	iWave = 0;
	iBossEveryN = 3;
	AddEffect("FxRaidTick", this, 1, 2000, this);
	return 1;
}

func FxRaidTickTimer(object pTarget, int fx) {
	// Sum cargo value across all active sea caravans.
	var iCargoValue = 0;
	var pCog;
	for (pCog in FindObjects(Find_ID(CGSH), Find_Func("IsSeaCaravanActive")))
		iCargoValue += GetCargoValue(pCog);

	// Edge case #4: no cargo in transit -> skip the wave (no target).
	if (iCargoValue <= 0) return 1;

	// Wave size = cargoValue / 100, capped at 6.
	var iCount = BoundBy(iCargoValue / 100, 1, 6);
	iWave++;

	// Pick the SLMR marker nearest the highest-value Cog.
	var pMarker = FindBestSeaLaneMarker();
	if (!pMarker) pMarker = FindObject(SLMR);
	if (!pMarker) return 1;

	SpawnPirateWave(iCount, pMarker);

	// Boss every Nth wave.
	if (iWave % iBossEveryN == 0) {
		var pBoss = CreateObject(PSLP, GetX(pMarker) + 20, GetY(pMarker) - 30, NO_OWNER);
		if (pBoss) pBoss->StartPatrol([GetX(pMarker), LandscapeWidth() - GetX(pMarker)]);
	}
	return 1;
}

global func SpawnPirateWave(int iCount, object pMarker) {
	var iX = GetX(pMarker);
	var iY = GetY(pMarker);
	for (var i = 0; i < iCount; i++) {
		var pPirate = CreateObject(PSLP, iX + Random(40) - 20, iY - Random(30), NO_OWNER);
		if (pPirate) pPirate->StartPatrol([iX, LandscapeWidth() - iX]);
	}
}

/* ---- Lose condition (called from scenario poll) ---- */
public func CheckSeaTradeDefeat() {
	// Any active sea caravan left? Then not defeated.
	var pCog;
	for (pCog in FindObjects(Find_ID(CGSH), Find_Func("IsSeaCaravanActive")))
		return;
	// No caravans. If pooled wealth < 100, no rebuild possible -> game over.
	var iPlr = GetAnyPlayer();
	if (iPlr >= 0 && GetWealth(iPlr) < 100)
		GameOver();
}

private func FindBestSeaLaneMarker() {
	// Find the Cog with the highest cargo value, then the nearest SLMR.
	var pBestCog = 0, iBestValue = -1, pCog;
	for (pCog in FindObjects(Find_ID(CGSH), Find_Func("IsSeaCaravanActive"))) {
		var iVal = GetCargoValue(pCog);
		if (iVal > iBestValue) { iBestValue = iVal; pBestCog = pCog; }
	}
	if (!pBestCog) return 0;
	var iX = GetX(pBestCog);
	var pBest = 0, iBestDist = 999999, pMarker;
	for (pMarker in FindObjects(Find_ID(SLMR))) {
		var iDist = Abs(GetX(pMarker) - iX);
		if (iDist < iBestDist) { iBestDist = iDist; pBest = pMarker; }
	}
	return pBest;
}
