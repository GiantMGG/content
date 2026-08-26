/*-- Selenite Heart (unique deep-lava objective) --*/

#strict

local heart_x;  // original spawn x (for re-spawn)
local heart_y;  // original spawn y

protected func Initialize()
{
	heart_x = GetX();
	heart_y = GetY();
	// Faint bioluminescent glow overlay (the heart glows softly in the dark lava pocket)
	AttachGlowOverlay(this(), RGB(255, 230, 160), 60);
}

/* Heavy: slows any Clonk carrying it (high Mass already does this; this is a hook) */
public func IsHeavy() { return 1; }

/* Cannot enter any container except the surface homebase (forces hand-carry ascent) */
public func RejectEntrance(object container)
{
	// Allow only the homebase structure (id HUT3 in this scenario) — spec edge case
	if (container && GetID(container) == HUT3) return 0;
	return 1;
}

protected func Hit() { Sound("Crystal*"); return 1; }

/* Destroyed if it touches DuroLava — spec edge case "SeleniteHeart dropped in lava" */
protected func ContactLeft()  { CheckLavaContact(); }
protected func ContactRight() { CheckLavaContact(); }
protected func ContactTop()   { CheckLavaContact(); }
protected func ContactBottom(){ CheckLavaContact(); }

private func CheckLavaContact()
{
	if (GetMaterial(0, 0) == Material("DuroLava") ||
	    GetMaterial(0, 2) == Material("DuroLava"))
	{
		Sparkle(GetX(), GetY(), 255, 240, 180, 30);
		Message("$MsgHeartLost$");
		// Mercy mechanic: re-spawn at the original socket after 60 seconds.
		// Capture coordinates into the schedule string before RemoveObject
		// destroys this script context.
		var hx = heart_x, hy = heart_y;
		Schedule(Format("CreateObject(SLNH, %d, %d, NO_OWNER)", hx, hy), 3600);
		RemoveObject();
	}
}

/* Non-sellable until carried to surface homebase — CalcValue returns 0 */
protected func CalcValue() { return 0; }

func IsAlchemContainer() { return false; }
