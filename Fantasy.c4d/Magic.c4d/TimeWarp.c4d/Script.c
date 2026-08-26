/* Time Warp - slow all hostile clonks in range */

#strict

func Activate(pCaster, pRealcaster) {
	var pClonk = pRealcaster;
	if (!pClonk) pClonk = pCaster;
	// Effekt pruefen
	var iResult;
	if (iResult = CheckEffect("TimeWarpNSpell", 0, 125)) return(iResult != -1 && RemoveObject());
	Sound("Magic*");
	// Attach a slow effect to every non-allied, alive crewmember in radius 150
	var iRange = 150;
	var pVictim;
	while (pVictim = FindObject(0,0,0,-1,-1,OCF_CrewMember, 0,0, NoContainer(), pVictim)) {
		if (GetOCF(pVictim) & OCF_Dead) continue;
		if (!Hostile(GetOwner(pVictim), GetOwner(pClonk))) continue;
		if (ObjectDistance(pVictim, pClonk) > iRange) continue;
		AddEffect("TimeWarpNSpell", pVictim, 200, 1, 0, GetID());
	}
	// Visual: blue shockwave
	CastParticles("PSpark", 20, 60, GetX(pClonk), GetY(pClonk), 150, 150, RGB(80, 150, 255), RGB(80, 150, 255));
	RemoveObject();
	return(1);
}

func FxTimeWarpNSpellStart(pTarget, iNumber, iTemp) {
	if (iTemp) return();
	// Duration: 120 frames (4s)
	EffectVar(0, pTarget, iNumber) = 120;
}

func FxTimeWarpNSpellTimer(pTarget, iNumber, iEffectTime) {
	if (EffectVar(0, pTarget, iNumber) <= 0) return(-1);
	EffectVar(0, pTarget, iNumber)--;
	// Halve movement
	SetXDir(GetXDir(pTarget) / 2, pTarget);
	SetYDir(GetYDir(pTarget) / 2, pTarget);
	// Force out of jump action (ObjectSetAction mirrors Meditation.c4d's verified pattern)
	if (GetAction(pTarget) eq "Jump") ObjectSetAction(pTarget, "Walk", 0, 0, 1);
	// Visual trickle
	if (iEffectTime % 10 == 0)
		CreateParticle("PSpark", GetX(pTarget), GetY(pTarget), 0, -1, 20, RGB(80, 150, 255), pTarget);
	return(1);
}

func FxTimeWarpNSpellStop(pTarget, iNumber, iReason, iTemp) {
	if (iTemp) return();
}

/* Zaubercombo */
public func GetSpellClass() { return(WATR); }
public func GetSpellCombo() { return("165"); }
public func GetSpellCombine() { return([MMED, GVTY]); } // Meditation + Float
