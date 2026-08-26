/* Inferno - sustained flamethrower cone */

#strict

func Activate(pCaster, pRealcaster) {
	var pClonk = pRealcaster;
	if (!pClonk) pClonk = pCaster;
	// Effekt pruefen
	var iResult;
	if (iResult = CheckEffect("InfernoPSpell", 0, 125)) return(iResult != -1 && RemoveObject());
	Sound("Magic*");
	// Sustained flamethrower effect on the caster
	AddEffect("InfernoPSpell", pClonk, 250, 1, 0, GetID());
	RemoveObject();
	return(1);
}

func FxInfernoPSpellStart(pTarget, iNumber, iTemp) {
	if (iTemp) return();
	// Duration cap: 90 frames
	EffectVar(0, pTarget, iNumber) = 90;
}

func FxInfernoPSpellTimer(pTarget, iNumber, iEffectTime) {
	// Duration expired
	if (EffectVar(0, pTarget, iNumber) <= 0) return(-1);
	EffectVar(0, pTarget, iNumber)--;
	// Mana drain: 1 per frame; auto-end at 0 mana
	if (GetMagicEnergy(pTarget) <= 0) return(-1);
	DoMagicEnergy(-1, pTarget);
	// Flamethrower cone: 40px forward in GetDir
	var iDir = GetDir(pTarget);
	var iSign = iDir * 2 - 1; // -1 left, +1 right
	var iX = GetX(pTarget), iY = GetY(pTarget);
	// Fire particle burst biased forward
	CastParticles("Fire", 8, 20, iX + iSign * 15, iY, iSign * 30, 0, RGB(255, 120, 0), RGB(255, 200, 0));
	// Ignite flammable objects within 40px in front
	var pObj;
	while (pObj = FindObject(0,0,0,-1,-1,OCF_Inflammable, 0,0, NoContainer(), pObj)) {
		if ((GetX(pObj) - iX) * iSign > 0)
			if (ObjectDistance(pObj, pTarget) <= 40)
				Incinerate(pObj);
	}
	return(1);
}

func FxInfernoPSpellStop(pTarget, iNumber, iReason, iTemp) {
	if (iTemp) return();
}

/* Zaubercombo */
public func GetSpellClass() { return(FIRE); }
public func GetSpellCombo() { return("333"); }
public func GetSpellCombine() { return([MFRB, MFWL]); } // Fireball + Firewall
