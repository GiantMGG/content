/*-- Escape Lever (ERLV) — ControlThrow toggles phase and      */
/* calls target->~Access() (Hazard Switch.c4d contract).       */
/* Rooms poll the lever phase from their director effects.     */

#strict 2

local target;
local locked;

public func Lock(bool unlock) { locked = !unlock; }
public func IsLocked()        { return locked; }
public func Set(object t)     { target = t; }

protected func ControlThrow(object byObj)
{
	if (IsLocked())
		return Sound("CommandFailure1", 0, 0, 80, GetController(byObj));
	SetPhase((GetPhase() + 1) % 2);
	if (target) target->~Access();
	Sound("Lever1");
	return true;
}

protected func Initialize()
{
	SetAction("Lever");
	return true;
}
