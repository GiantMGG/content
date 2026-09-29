/*-- STGT: shared destructible-wall interface (include library) --*/

#strict

/* The accumulated siege damage on this structure. Kept as a Local so it
   survives save/load (C4Object serialises Locals). */
local iSiegeDamage;
local fSiegeDestroyed;
local iSiegeTier;  // 0 none, 1 Crack1, 2 Crack2, 3 destroyed

/* Opt-in marker. Any structure that #include STGT is siege-damageable. */
public func IsSiegeTarget() { return true; }

/* Per-structure max HP override. Default 150 (matches Basement72 collapse
   threshold). Override in the including def. */
public func MaxSiegeHP() { return 150; }

/* Wooden-structure hook. Override to true on wooden gates so FPOT does x3. */
public func IsWoodenStructure() { return false; }

/* Current crack tier: 0 none, 1 Crack1, 2 Crack2, 3 destroyed. */
public func GetSiegeTier() { return iSiegeTier; }

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
	var iPrev = iSiegeDamage;
	iSiegeDamage += iDmg;
	// Crack-state graphics (thresholds based on MaxSiegeHP).
	// Slot-0 base swaps: SetGraphics(name, obj, id, 0, mode) bypasses the
	// overlay system and swaps the object's base graphics group
	// (C4Script.cpp SetGraphics overlay 0 -> C4Object::SetGraphics), so the
	// crack sheets actually render in-world. Picture overlays (slot != 0)
	// only draw in object pictures/HUD, never on the live object.
	var iMax = MaxSiegeHP();
	if (iSiegeDamage >= iMax * 2 / 3) {
		SetGraphics("Crack2", this(), GetID(), 0, 0);
		iSiegeTier = 2;
		if (iPrev < iMax * 2 / 3 && SoundExists("SiegeCrack")) Sound("SiegeCrack");
	} else if (iSiegeDamage >= iMax / 3) {
		SetGraphics("Crack1", this(), GetID(), 0, 0);
		iSiegeTier = 1;
		if (iPrev < iMax / 3 && SoundExists("SiegeCrack")) Sound("SiegeCrack");
	} else {
		iSiegeTier = 0;
	}
	// Destruction threshold: SBLD ignores 50% of MaxSiegeHP
	var iEffMax = iMax;
	if (idAmmo == SBLD) iEffMax = iMax / 2;
	if (iSiegeDamage >= iEffMax) {
		iSiegeTier = 3;
		OnSiegeDestroyed(iByPlayer);
	}
}

/* Decrement accumulated siege damage, re-evaluate crack overlays.
   Called by a repair action on the structure:
     pTarget->~SiegeRepair(iAmt, GetController());
   No-op if the structure is already destroyed. */
public func SiegeRepair(int iAmt, int iByPlayer) {
	if (iAmt <= 0) return;
	if (fSiegeDestroyed) return;
	iSiegeDamage = Max(0, iSiegeDamage - iAmt);
	// Re-evaluate crack graphics base swaps (clear at < 1/3, Crack1 at < 2/3).
	// Slot-0 swaps restore/re-apply the base graphics group; picture overlays
	// never render in-world, so the repair must swap the base sheet back.
	if (iSiegeDamage < MaxSiegeHP() / 3) {
		// Base back to the DEFAULT group: SetGraphics(0, ..., 0, _) with
		// overlay slot 0 routes to C4Object::SetGraphics, and a null
		// graphics name resolves to the default sheet (no-op if already).
		SetGraphics(0, this(), GetID(), 0, 0);
		iSiegeTier = 0;
	} else if (iSiegeDamage < MaxSiegeHP() * 2 / 3) {
		SetGraphics("Crack1", this(), GetID(), 0, 0);
		iSiegeTier = 1;
	}
	// SolidMask is never cleared until OnSiegeDestroyed, so no re-arming here.
}

/* Called when the structure reaches its siege-damage threshold. Clears the
   SolidMask so clonks can walk through, swaps to Ruin graphics, casts debris,
   and removes the object after 1 frame so any Hit() chain completes. */
public func OnSiegeDestroyed(int iByPlayer) {
	if (fSiegeDestroyed) return;
	fSiegeDestroyed = true;
	if (SoundExists("SiegeBreak")) Sound("SiegeBreak");
	CastObjects(ROCK, 8, 20);
	SetGraphics("Ruin", this(), GetID(), 0, 5);
	SetSolidMask(0, 0, 0, 0);
	// 35-frame linger: the ruin art + break sound become perceivable
	// (SolidMask is already clear, so passage is immediate).
	Schedule("RemoveObject()", 35, 0, this());
}
