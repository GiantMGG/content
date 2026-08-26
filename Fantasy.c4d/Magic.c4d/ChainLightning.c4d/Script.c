/* Chain Lightning - instant arc, up to 5 targets */

#strict

func Activate(pCaster, pRealcaster) {
	var pClonk = pRealcaster;
	if (!pClonk) pClonk = pCaster;
	// Effekt pruefen
	var iResult;
	if (iResult = CheckEffect("ChainLightningNSpell", 0, 125)) return(iResult != -1 && RemoveObject());
	Sound("Magic*");
	// Gather hostile, alive crewmembers (array-append — no PushBack in this engine)
	var iChainRange = 80;
	var iMaxTargets = 5;
	var aTargets = [], pVictim;
	while (pVictim = FindObject(0,0,0,-1,-1,OCF_CrewMember, 0,0, NoContainer(), pVictim)) {
		if (GetOCF(pVictim) & OCF_Dead) continue;
		if (!Hostile(GetOwner(pVictim), GetOwner(pClonk))) continue;
		aTargets[GetLength(aTargets)] = pVictim;
	}
	// Chain from the caster through nearest not-yet-hit hostiles
	var pFrom = pClonk;
	var aHit = [];
	var iHits = 0;
	while (iHits < iMaxTargets) {
		var pNext = 0, iBestDist = iChainRange + 1;
		for (var pCand in aTargets) {
			if (GetIndexOf(aHit, pCand) != -1) continue;
			var iDist = ObjectDistance(pCand, pFrom);
			if (iDist < iBestDist) { iBestDist = iDist; pNext = pCand; }
		}
		if (!pNext) break;
		// 15 energy damage + spark burst
		DoEnergy(-15, pNext);
		CastParticles("PSpark", 8, 20, GetX(pNext), GetY(pNext), 20, 20, RGB(200, 220, 255), RGB(200, 220, 255));
		// Lightning bolt line from pFrom to pNext
		DrawLightningLine(GetX(pFrom), GetY(pFrom), GetX(pNext), GetY(pNext));
		aHit[GetLength(aHit)] = pNext;
		pFrom = pNext;
		iHits++;
	}
	// Visual feedback at the caster even on zero hits
	CastParticles("PSpark", 4, 15, GetX(pClonk), GetY(pClonk), 15, 15, RGB(200, 220, 255), RGB(200, 220, 255));
	RemoveObject();
	return(1);
}

/* Draws a spark line between two points (procedural particle chain). */
private func DrawLightningLine(int iX1, int iY1, int iX2, int iY2) {
	var iSteps = Max(Abs(iX2 - iX1), Abs(iY2 - iY1)) / 5 + 1;
	for (var i = 0; i <= iSteps; i++) {
		var iX = iX1 + (iX2 - iX1) * i / iSteps + RandomX(-2, 2);
		var iY = iY1 + (iY2 - iY1) * i / iSteps + RandomX(-2, 2);
		CreateParticle("PSpark", iX, iY, 0, 0, 15, RGB(200, 220, 255));
	}
}

/* Zaubercombo */
public func GetSpellClass() { return(AIR1); }
public func GetSpellCombo() { return("333"); }
public func GetSpellCombine() { return([LGCN, GZ9Z]); } // LightningConcealment + GuardingZaps
