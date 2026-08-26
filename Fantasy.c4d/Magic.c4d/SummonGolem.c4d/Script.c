/* Summon Golem - temporary rock construct */

#strict

func Activate(pCaster, pRealcaster) {
	var pClonk = pRealcaster;
	if (!pClonk) pClonk = pCaster;
	// Effekt pruefen
	var iResult;
	if (iResult = CheckEffect("SummonGolemNSpell", 0, 125)) return(iResult != -1 && RemoveObject());
	Sound("Magic*");
	// Find a free spawn position at/near the caster.
	// NOTE: the spell object (this()) sits at the caster's position because
	// ExecMagic creates it via CreateObject(idSpell, 0, 0, owner) and
	// CreateObject offsets by the calling object's position (C4Script.cpp:1992).
	// GBackSolid(x, y) offsets by cthr->Obj (the spell), so GBackSolid(0, 0)
	// probes the caster's tile (verified at C4Script.cpp:2473).
	var iX = GetX(), iY = GetY(); // spell == caster position
	var iSpawnX = iX, iSpawnY = iY;
	if (GBackSolid(0, 0)) {
		var bFound = false;
		for (var iOff = 1; iOff <= 20 && !bFound; iOff++) {
			for (var iAngle = 0; iAngle < 360 && !bFound; iAngle += 45) {
				var iDX = Sin(iAngle, iOff), iDY = -Cos(iAngle, iOff);
				if (!GBackSolid(iDX, iDY)) {
					iSpawnX = iX + iDX; iSpawnY = iY + iDY;
					bFound = true;
				}
			}
		}
		if (!bFound) {
			PlayerMessage(GetOwner(pClonk), "$MsgNoSpace$", pClonk);
			Sound("Error", 0, pClonk, 100, GetOwner(pClonk)+1);
		}
	}
	// Spawn the golem, owned by the caster's controller
	var pGolem = CreateObject(GOLE, AbsX(iSpawnX), AbsY(iSpawnY), GetController(pClonk));
	if (pGolem) {
		pGolem->SetController(GetController(pClonk));
		// Lifetime: 900 frames (30s) — interval 1 so FxSummonGolemUSpellTimer fires every frame and counts down EffectVar(0)
		AddEffect("SummonGolemUSpell", pGolem, 100, 1, 0, GetID());
	}
	RemoveObject();
	return(1);
}

func FxSummonGolemUSpellStart(pTarget, iNumber, iTemp) {
	if (iTemp) return();
	// Lifetime: 900 frames (30s)
	EffectVar(0, pTarget, iNumber) = 900;
}

func FxSummonGolemUSpellTimer(pTarget, iNumber, iEffectTime) {
	if (EffectVar(0, pTarget, iNumber) <= 0) return(-1);
	EffectVar(0, pTarget, iNumber)--;
	// Crumble-dust visual every 30 frames
	if (iEffectTime % 30 == 0)
		CastParticles("PSpark", 2, 10, GetX(pTarget), GetY(pTarget), 10, 15, RGB(120, 100, 80), RGB(120, 100, 80));
	return(1);
}

func FxSummonGolemUSpellStop(pTarget, iNumber, iReason, iTemp) {
	if (iTemp) return();
	// Crumble
	CastParticles("PSpark", 10, 30, GetX(pTarget), GetY(pTarget), 20, 30, RGB(120, 100, 80), RGB(120, 100, 80));
	RemoveObject(pTarget);
}

/* Zaubercombo */
public func GetSpellClass() { return(EART); }
public func GetSpellCombo() { return("333"); }
public func GetSpellCombine() { return([MARK, MFBL]); } // Rockstrike + Firelump
