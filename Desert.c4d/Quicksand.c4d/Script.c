/*-- Quicksand (QKSD) -- StaticBack hazard patch. --*/
/* Walkers on the patch sink; jumping struggles free. */

#strict

public func Construction()
{
	// Stamp a small Sand patch so the hazard is visible in terrain.
	// InsertMaterial is object-relative in object context.
	InsertMaterial(Material("Sand"), -10, -3);
	InsertMaterial(Material("Sand"), 9, -3);
	return 1;
}

public func Sink()
{
	// Attach the sink effect to walkers on the patch (mass-gated:
	// only objects with a Walk physical sink; vehicles/PXS skip).
	var victim;
	for (var victim in FindObjects(Find_AtRect(-10, -3, 20, 6), Find_NoContainer()))
	{
		if (!GetPhysical("Walk", 0, victim)) continue;
		if (!GetEffect("QuicksandSink", victim))
			AddEffect("QuicksandSink", victim, 1, 4, this);
	}
}

func FxQuicksandSinkStart(object target, int effect, bool temp)
{
	if (temp) return;
	// Mass-gate: walkers only.
	if (!GetPhysical("Walk", 0, target)) return FX_Execute_Kill;
	// Record baseline Walk physical, then cut it (Webbed-style).
	EffectVar(0, target, effect) = GetPhysical("Walk", 0, target);
	SetPhysical("Walk", 20000, 2, target);
}

func FxQuicksandSinkTimer(object target, int effect, int time)
{
	// Off the patch? Effect ends. Find_AtRect is caller-relative, and the
	// effect timer's caller is the quicksand (AddEffect command target),
	// so re-centre the rect on the victim: subtract the quicksand's own
	// position from the victim's before adding the half-extents.
	if (!FindObject2(Find_ID(QKSD),
		Find_AtRect(GetX(target) - GetX() - 12, GetY(target) - GetY() - 5, 24, 10)))
		return FX_Execute_Kill;
	// Jumping struggles free: 2px up per tick.
	if (GetAction(target) == "Jump")
	{
		SetPosition(GetX(target), GetY(target) - 2, target);
		return FX_OK;
	}
	// Sinking drift: 1px down per tick.
	SetPosition(GetX(target), GetY(target) + 1, target);
	return FX_OK;
}

func FxQuicksandSinkStop(object target, int effect, int reason, bool temp)
{
	if (temp) return;
	// Restore the baseline Walk physical.
	SetPhysical("Walk", EffectVar(0, target, effect), 2, target);
}
