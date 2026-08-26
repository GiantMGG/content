/* ManaAura rule - mage-identity mana regen aura */

#strict

protected func Activate(iPlr) { MessageWindow(GetDesc(), iPlr); }

/* Called every 10 frames (TimerCall). Attaches the aura effect to every
   alive clonk with a Magic physical that does not already have one. This
   replaces the spec's non-existent OnClonkRecruited hook with a working
   scan, mirroring MagicEnergySupply.c4d's TimerCall=CheckMana pattern. */
func AttachAuras() {
	var pClonk;
	while (pClonk = FindObject(0,0,0,0,0,OCF_Alive(), 0,0,0, pClonk)) {
		if (!GetPhysical("Magic", 0, pClonk)) continue;
		if (GetEffect("ManaAuraPSpell", pClonk)) continue;
		AddEffect("ManaAuraPSpell", pClonk, 150, 30, 0, GetID());
	}
}

func FxManaAuraPSpellStart(pTarget, iNumber, iTemp) {
	if (iTemp) return();
	// Range: 80px (tunable)
	EffectVar(0, pTarget, iNumber) = 80;
}

func FxManaAuraPSpellTimer(pTarget, iNumber, iEffectTime) {
	// Self regen
	DoMagicEnergy(+1, pTarget);
	// Visual: blue aura ring
	var range = EffectVar(0, pTarget, iNumber);
	CreateParticle("Aura", GetX(pTarget), GetY(pTarget), 0, 0, 12 * range, RGB(50, 180, 255));
	// Allied clonks (with a Magic physical) within range get +1 mana
	var pAlly;
	while (pAlly = FindObject(0,0,0,-1,-1,OCF_CrewMember, 0,0, NoContainer(), pAlly)) {
		if (pAlly == pTarget) continue;
		if (ObjectDistance(pAlly, pTarget) > range) continue;
		if (Hostile(GetOwner(pAlly), GetOwner(pTarget))) continue;
		if (!GetPhysical("Magic", 0, pAlly)) continue;
		if (GetMagicEnergy(pAlly) >= GetPhysical("Magic", 0, pAlly) / 1000) continue;
		DoMagicEnergy(+1, pAlly);
	}
	return(1);
}

/* Overlapping auras: reject the new effect (return -1) so regen rate does
   not double-stack. The existing effect is kept. */
func FxManaAuraPSpellAdd(pTarget, iNumber, szNewName, iNewTimer, c0, c1, c2, iTime) {
	return(-1);
}

/* Bearer-mitigation: reduce incoming damage to the aura bearer by a flat
   2, capped at 50% of the hit. (Ally mitigation is out of scope per spec.) */
func FxManaAuraPSpellDamage(pTarget, iNumber, iDmg, iCause, pTarget2) {
	var iReduce = Min(iDmg / 2, 2);
	return(iDmg - iReduce);
}

func FxManaAuraPSpellStop(pTarget, iNumber, iReason, iTemp) {
	if (iTemp) return();
}
