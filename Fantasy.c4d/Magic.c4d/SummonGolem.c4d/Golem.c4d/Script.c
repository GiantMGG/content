/* Summoned rock golem - temporary ally construct */

#strict

protected func Initialize() {
	// 50 energy via [Physical] Energy=50000 in DefCore; allied to caster's
	// controller (owner already set on CreateObject). Start walking.
	SetAction("Walk");
}

public func IsStoneGolem() { return(true); }

/* The golem is owned by the caster's controller; the engine treats it as a
   C4D_Living object with Walk/Push/Fight physicals and 50 energy. It crumbles
   via the SummonGolemUSpell effect's Stop callback after 30s, or here when its
   energy is depleted. */

protected func Damage(int iDmg, int iCause) {
	// Engine has already applied the damage to Energy
	if (GetEnergy() <= 0) {
		CastParticles("PSpark", 10, 30, GetX(), GetY(), 20, 30, RGB(120, 100, 80), RGB(120, 100, 80));
		RemoveObject();
	}
}
