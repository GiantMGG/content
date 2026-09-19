/*-- Scorpion (SCRP) -- nocturnal desert predator. --*/

#strict 3

#include ANIM
#include WLFA

public func IsPossessible() { return true; }

protected func Initialize()
{
	WLFA_AddBehavior(this, "HuntPrey", [35, 300]);
	WLFA_SetLootTable(this, [MEAT, 100]);
	return Birth();
}

protected func Activity()
{
	// Driven by WLF_* behaviour effects. Possessed creatures skip AI.
	if (GetEffect("PossessionSpell", this)) return;
}

/* Sting: the base attack plus a Webbed-style venom slow on the victim
   (spec §3: WLFA_Attack + reused physical reduction). Overrides the
   WLFA framework attack; called via object call by WLF_HuntPrey. */
public func WLFA_Attack(object prey, int damage)
{
	if (!inherited(prey, damage)) return false;
	// Venom slow: Webbed-style Walk cut that wears off after ~5s.
	if (prey && !GetEffect("ScorpionVenom", prey))
		AddEffect("ScorpionVenom", prey, 1, 35, this);
	return true;
}

func FxScorpionVenomStart(object target, int effect, bool temp)
{
	if (temp) return;
	// Record the baseline Walk physical, then cut it (Webbed-style).
	EffectVar(0, target, effect) = GetPhysical("Walk", 0, target);
	SetPhysical("Walk", 30000, 2, target);
}

func FxScorpionVenomTimer(object target, int effect, int time)
{
	// Venom wears off after ~5 seconds.
	if (time >= 175) return FX_Execute_Kill;
	return FX_OK;
}

func FxScorpionVenomStop(object target, int effect, int reason, bool temp)
{
	if (temp) return;
	// Restore the baseline Walk physical.
	SetPhysical("Walk", EffectVar(0, target, effect), 2, target);
}

protected func Death()
{
	WLFA_DropLoot(this, WLFA_GetLootTable(this));
	RemoveObject();
	return true;
}

private func MaxAnimalCount() { return 6; }

/* ---- Salt Road story API (cycle 146) ---------------------------------- */
/* Raid / queen / brood / ending machinery shared by the Salt Road acts   */
/* and the story smoke. RDRR SpawnBanditWave pattern for raids            */
/* (RaidDirector.c4d/Script.c:54-64). The queen is a draw-scaled,         */
/* recolored SCRP (SetObjDrawTransform per-mille: 1000 = 100%); the      */
/* scaling is visual-only, her shape stays 20x12.                         */

// Spawn up to iCount SCRP at (iX, iY), Attack-commanded onto the nearest
// clonk or camel. Returns the number actually spawned.
// Spawn-embedding hardening (review F5): the SCRP 20x12 shape embeds into
// slopes/outcrops when the terrain rises into the spawn column, which
// leaves an immobile-unreachable raider -- an Act I raid-gate stall. Every
// spawn nudge picks a free-stance column and spawns a few px higher.
global func SaltRoad_SpawnRaid(int iX, int iY, int iCount)
{
	var victim = SaltRoad_NearestVictim(iX, iY);
	var spawned = 0;
	var i;
	for (i = 0; i < iCount; i++)
	{
		var sy = iY - 6 - Random(15);
		var sx = SaltRoad_FreeStanceX(iX + Random(41) - 20, sy);
		var scrp = CreateObject(SCRP, sx, sy, NO_OWNER);
		if (!scrp) break;
		if (victim) SetCommand(scrp, "Attack", victim);
		spawned++;
	}
	return spawned;
}

// Pick the nearest horizontal position to (x, y) whose 20px-wide SCRP
// stance clears solid terrain (review F5): a spawn whose GroundY was
// sampled at the group leader's column can sit inside a rising slope or
// outcrop at its own column. Try 0, then 10px/20px neighbours on both
// sides; return the first column whose head-to-foot box is open air.
// Falls back to the requested column when every candidate cuts into
// terrain (e.g. cliff faces).
global func SaltRoad_FreeStanceX(int x, int y)
{
	var offsets = [0, -10, 10, -20, 20];
	var i, cx;
	for (i = 0; i < GetLength(offsets); i++)
	{
		cx = x + offsets[i];
		if (!GBackSolid(cx - 8, y - 8) && !GBackSolid(cx, y - 8) && !GBackSolid(cx + 8, y - 8)
		 && !GBackSolid(cx - 8, y - 4) && !GBackSolid(cx, y - 4) && !GBackSolid(cx + 8, y - 4)
		 && !GBackSolid(cx - 8, y) && !GBackSolid(cx, y) && !GBackSolid(cx + 8, y))
			return cx;
	}
	return x;
}

// The queen of the toll: 2.5x draw-scaled, amber-tinted, boss-physicals.
// Tags herself with the "SaltRoadQueen" effect so acts and the smoke can
// find her robustly (no dangling object pointer in a static).
global func SaltRoad_SpawnQueen(int iX, int iY)
{
	var queen = CreateObject(SCRP, iX, iY, NO_OWNER);
	if (!queen) return nil;
	queen->SetObjDrawTransform(2500, 0, 0, 0, 2500, 0);
	// Amber tint via draw modulation: the Color property is color-by-owner
	// gated and wiped on every graphics update for SCRP (C4Object::
	// UpdateGraphics), so SetColorDw would silently no-op. SetClrModulation
	// tints at draw time regardless of the def's ColorByOwner flag and is
	// readable back (GetClrModulation) for the story smoke.
	queen->SetClrModulation(RGB(150, 100, 40));
	queen->SetPhysical("Energy", 120000, 2);
	queen->SetPhysical("Fight", 60000, 2);
	queen->DoEnergy(90000);
	queen->SetName("Scorpion Queen");
	AddEffect("SaltRoadQueen", queen, 1, 0, queen);
	return queen;
}

// Brood tick (call on a cadence while the queen lives): one minion at
// the queen's flank, self-capped at 6 live SCRP -- the framework's
// MaxAnimalCount only gates reproduction, not CreateObject, so scripted
// spawns must self-cap.
global func SaltRoad_QueenBrood(object queen)
{
	if (!queen) return 0;
	if (ObjectCount(SCRP) >= 6) return 0;
	// Free-stance nudge (review F5), same embed risk as the raids above.
	var sy = GetY(queen) - 10 - Random(6);
	var sx = SaltRoad_FreeStanceX(GetX(queen) - 20 + Random(41), sy);
	var minion = CreateObject(SCRP, sx, sy, NO_OWNER);
	if (!minion) return 0;
	var victim = SaltRoad_NearestVictim(GetX(queen), GetY(queen));
	if (victim) SetCommand(minion, "Attack", victim);
	return 1;
}

// The single deterministic epilogue: both Act 3 win paths (fight / pay)
// converge here.
global func SaltRoad_Ending()
{
	Log("The Salt Road is open: the queen's toll is settled.");
	GameOver();
	return true;
}

// Nearest uncontained live clonk or camel to (iX, iY) -- raid targeting.
// (global, not private: bare-name calls from the global raid/brood funcs
// resolve through the engine's global scope in this engine -- a local
// helper is not visible from a global func's scope.)
global func SaltRoad_NearestVictim(int iX, int iY)
{
	var best = nil;
	var best_dist = 999999;
	var obj;
	for (obj in FindObjects(Find_ID(CLNK), Find_NoContainer()))
	{
		// Liveness filter (review F6): dead clonks linger as corpses, and
		// Attack-commanding a far-away body wastes a raid wave.
		if (!GetAlive(obj)) continue;
		var dist = Abs(GetX(obj) - iX) + Abs(GetY(obj) - iY);
		if (dist < best_dist) { best_dist = dist; best = obj; }
	}
	for (obj in FindObjects(Find_ID(CAML), Find_NoContainer()))
	{
		if (!GetAlive(obj)) continue;
		var dist = Abs(GetX(obj) - iX) + Abs(GetY(obj) - iY);
		if (dist < best_dist) { best_dist = dist; best = obj; }
	}
	return best;
}
