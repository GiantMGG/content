/*-- STGT: shared destructible-wall interface (include library) --*/

#strict

/* The accumulated siege damage on this structure. Kept as a Local so it
   survives save/load (C4Object serialises Locals). */
local iSiegeDamage;
local fSiegeDestroyed;

/* Opt-in marker. Any structure that #include STGT is siege-damageable. */
public func IsSiegeTarget() { return true; }

/* Per-structure max HP override. Default 150 (matches Basement72 collapse
   threshold). Override in the including def. */
public func MaxSiegeHP() { return 150; }

/* Wooden-structure hook. Override to true on wooden gates so FPOT does x3. */
public func IsWoodenStructure() { return false; }

/* Apply siege damage. Called by ammunition Hit() as:
     pTarget->~SiegeDamage(iDmg, GetController(), GetID());
   Per-ammo vulnerabilities:
   - FPOT does x3 damage to wooden structures.
   - SBLD ignores 50% of MaxSiegeHP (destruction threshold halved).
   - SROK and BOMB apply flat damage. */
public func SiegeDamage(int iDmg, int iByPlayer, id idAmmo) {
	if (iDmg <= 0) return;
	// Per-ammo vulnerability: FPOT x3 vs wooden structures
	if (idAmmo == FPOT)
		if (~IsWoodenStructure())
			iDmg *= 3;
	// Accumulate
	iSiegeDamage += iDmg;
	// Crack-state graphics (thresholds based on MaxSiegeHP)
	var iMax = MaxSiegeHP();
	if (iSiegeDamage >= iMax * 2 / 3)
		SetGraphics("Crack2", GetID(), 1, 3);
	else if (iSiegeDamage >= iMax / 3)
		SetGraphics("Crack1", GetID(), 1, 3);
	// Destruction threshold: SBLD ignores 50% of MaxSiegeHP
	var iEffMax = iMax;
	if (idAmmo == SBLD) iEffMax = iMax / 2;
	if (iSiegeDamage >= iEffMax) OnSiegeDestroyed(iByPlayer);
}

/* Called when the structure reaches its siege-damage threshold. Clears the
   SolidMask so clonks can walk through, swaps to Ruin graphics, casts debris,
   and removes the object after 1 frame so any Hit() chain completes. */
public func OnSiegeDestroyed(int iByPlayer) {
	if (fSiegeDestroyed) return;
	fSiegeDestroyed = true;
	CastObjects(ROCK, 8, 20);
	SetGraphics("Ruin", GetID(), 0, 5);
	SetSolidMask(0, 0, 0, 0);
	Schedule("RemoveObject()", 1, 0, this());
}
