/* Gravity Well - stationary attractor for 5s */

#strict

func Activate(pCaster, pRealcaster) {
	var pClonk = pRealcaster;
	if (!pClonk) pClonk = pCaster;
	// Effekt pruefen
	var iResult;
	if (iResult = CheckEffect("GravityWellNSpell", 0, 125)) return(iResult != -1 && RemoveObject());
	Sound("Magic*");
	// Spawn the attractor object at the caster's position
	var pWell = CreateObject(GWLW, AbsX(GetX(pClonk)), AbsY(GetY(pClonk)), GetOwner(pClonk));
	if (pWell) {
		pWell->SetController(GetController(pClonk));
		AddEffect("GravityWellNSpell", pWell, 200, 1, 0, GetID());
	}
	RemoveObject();
	return(1);
}

func FxGravityWellNSpellStart(pTarget, iNumber, iTemp) {
	if (iTemp) return();
	// Duration: 150 frames (5s)
	EffectVar(0, pTarget, iNumber) = 150;
}

func FxGravityWellNSpellTimer(pTarget, iNumber, iEffectTime) {
	if (EffectVar(0, pTarget, iNumber) <= 0) return(-1);
	EffectVar(0, pTarget, iNumber)--;
	// Hard-pull non-crew physics objects/projectiles toward the well
	var pObj;
	while (pObj = FindObject(0,0,0,-1,-1,0, 0,0, NoContainer(), pObj)) {
		if (pObj == pTarget) continue;
		if (GetOCF(pObj) & OCF_CrewMember) {
			// Crew: soft slow only, never yank
			SetXDir(GetXDir(pObj) / 2, pObj);
			SetYDir(GetYDir(pObj) / 2, pObj);
		} else {
			// Pull toward well center
			SetXDir((GetX(pTarget) - GetX(pObj)) / 5, pObj);
			SetYDir((GetY(pTarget) - GetY(pObj)) / 5, pObj);
		}
	}
	// Visual: purple vortex
	CreateParticle("PSpark", GetX(pTarget), GetY(pTarget), RandomX(-2,2), RandomX(-2,2), 20, RGB(160, 60, 200), pTarget);
	if (iEffectTime % 5 == 0)
		CastParticles("PSpark", 4, 15, GetX(pTarget), GetY(pTarget), 30, 30, RGB(160, 60, 200), RGB(160, 60, 200));
	return(1);
}

func FxGravityWellNSpellStop(pTarget, iNumber, iReason, iTemp) {
	if (iTemp) return();
	RemoveObject(pTarget);
}

/* Zaubercombo */
public func GetSpellClass() { return(EART); }
public func GetSpellCombo() { return("262"); }
public func GetSpellCombine() { return([GVTY, MARK]); } // Float + Rockstrike
