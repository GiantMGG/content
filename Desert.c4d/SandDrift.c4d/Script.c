/*-- SandDrift (SDRF) -- wind-driven surface-sand migration director. --*/
/*                                                                        */
/* One drift tick moves `budget` sand pixels from an open-sky sand        */
/* surface 6 px downwind and stacks them on the downwind surface          */
/* (erosion windward, accretion leeward). While SNDT rages the budget     */
/* ramps with storm intensity. Walkers caught in a deposit footprint      */
/* get the SandBuried effect (QKSD QuicksandSink pattern).                */
/*                                                                        */
/* Determinism: no Random anywhere in the drift path - the scan is a      */
/* deterministic cursor sweep, so every peer reproduces it exactly.       */
/*                                                                        */
/* #strict 3 (not plain #strict): Drift()'s 0 returns must be the         */
/* integer 0, not nil (the "0 == nil below #strict 3" Aul rule).          */

#strict 3

local scan_x;  // rotating extraction cursor (serialized object local)

protected func Construction()
{
	// #strict 3: uninitialized locals are nil; the scan math needs
	// an int cursor. (Also the savegame-safe reset point.)
	scan_x = 0;
	return true;
}

/* Topmost open-sky Sand pixel of column x, else -1. */
private func SandSurfaceY(int x)
{
	// Landscape funcs in object context add the object's own position
	// (QKSD comment): subtract GetX()/GetY() to probe absolute (x, y).
	var y;
	for (y = 0; y < LandscapeHeight(); y++)
	{
		var mat = GetMaterial(x - GetX(), y - GetY());
		if (mat == -1) continue;                  // sky
		if (mat == Material("Sand")) return y;    // open-sky Sand surface
		return -1;                                // overhung: never drifts
	}
	return -1;
}

/* Topmost open-sky solid-ground pixel of column x, else -1. */
private func GroundSurfaceY(int x)
{
	var y;
	for (y = 0; y < LandscapeHeight(); y++)
	{
		var mat = GetMaterial(x - GetX(), y - GetY());
		if (mat == -1) continue;                             // sky
		if (GBackSolid(x - GetX(), y - GetY())) return y;    // solid top: deposit base
		return -1;                                           // liquid/tunnel top: never deposits
	}
	return -1;
}

/* TimerCall: one drift tick. Returns pixels moved. */
public func Drift()
{
	// Global wind (SNDT Construction precedent).
	var wind = GetWind(0, 0, true);
	if (Abs(wind) < 30) return 0;                 // no-wind idle
	if (GetPXSCount() > 9000) return 0;           // headroom guard
	var budget = 2;
	if (GetActiveWeatherEvent() == SNDT)
		budget = 2 + GetWeatherEventIntensity() / 10;
	var dir = 1;
	if (wind < 0) dir = -1;
	var moved = 0;
	var probed = 0;
	while (moved < budget && probed < LandscapeWidth())
	{
		scan_x = (scan_x + 1) % LandscapeWidth();
		probed++;
		var ys = SandSurfaceY(scan_x);
		if (ys < 0) continue;
		var xd = BoundBy(scan_x + dir * 6, 0, LandscapeWidth() - 1);
		var yd = GroundSurfaceY(xd);
		if (yd < 0) continue;
		// Extract one surface pixel windward, insert it one above the
		// downwind surface (deposition stacks, cf. U2).
		if (ExtractMaterialAmount(scan_x - GetX(), ys - GetY(), Material("Sand"), 1) != 1)
			continue;
		if (!InsertMaterial(Material("Sand"), xd - GetX(), yd - 1 - GetY()))
		{
			// Deposit failed: conserve mass by putting the pixel back.
			InsertMaterial(Material("Sand"), scan_x - GetX(), ys - GetY());
			continue;
		}
		moved++;
		CheckBurial(xd, yd);
	}
	return moved;
}

/* Walkers in the deposit footprint get the SandBuried effect. */
private func CheckBurial(int xd, int yd)
{
	var victim;
	for (var victim in FindObjects(Find_AtRect(xd - 2 - GetX(), yd - 14 - GetY(), 4, 17), Find_NoContainer()))
	{
		if (!GetPhysical("Walk", 0, victim)) continue;   // mass-gate: walkers only
		if (GetEffect("SandBuried", victim)) continue;   // no double effect
		AddEffect("SandBuried", victim, 1, 4, this);
	}
}

/*-- SandBuried effect: QKSD's QuicksandSink shape, renamed. --*/
/* Start records the baseline Walk and cuts it; the Timer ends the    */
/* effect once the victim is clear of the sand; Stop restores.        */

func FxSandBuriedStart(object target, int effect, bool temp)
{
	if (temp) return;
	// Mass-gate: walkers only.
	if (!GetPhysical("Walk", 0, target)) return FX_Execute_Kill;
	// Record baseline Walk physical, then cut it (Webbed-style).
	EffectVar(0, target, effect) = GetPhysical("Walk", 0, target);
	SetPhysical("Walk", 20000, 2, target);
}

func FxSandBuriedTimer(object target, int effect, int time)
{
	// Buried while the victim's center cell is inside static Sand; clear
	// once it jumps out or is dug free (center cell no longer Sand).
	if (GetMaterial(GetX(target) - GetX(), GetY(target) - GetY()) == Material("Sand"))
		return FX_OK;
	return FX_Execute_Kill;
}

func FxSandBuriedStop(object target, int effect, int reason, bool temp)
{
	if (temp) return;
	// Restore the baseline Walk physical.
	SetPhysical("Walk", EffectVar(0, target, effect), 2, target);
}
