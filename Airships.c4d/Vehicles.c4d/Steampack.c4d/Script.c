/*-- Steampack (STPK) -- personal lift backpack --
  A wearable C4D_Object collected into the clonk's Contents. When
  activated, a timer effect on the clonk pins YDir=-3 (ascend) and
  XDir=GetWind()/4 (wind drift). Burns COAL from its own Contents via
  Burn_Consume(this, 10) per timer tick. One steampack per clonk
  (RejectCollect). Cannot activate in deep water. --*/

#strict 2

#include AERO
#include FUEL

local active;
local carrier;

public func IsFuel() { return false; }

public func RejectCollect(id idObject, object pObj)
{
	if (idObject == STPK)
		if (pObj->FindContents(STPK))
			return true;
	return false;
}

public func Entrance(object container)
{
	carrier = container;
	if (!container->GetEffect("SteampackControl", container))
		container->AddEffect("SteampackControl", container, 1, 0, this);
	return true;
}

public func Departure(object container)
{
	Deactivate();
	carrier = nil;
	if (container->GetEffect("SteampackControl", container))
		container->RemoveEffect("SteampackControl", container);
	return true;
}

public func Activate(object clonk)
{
	if (active) return true;
	if (!clonk) return false;

	if (GBackLiquid(0, clonk->GetObjHeight() / 2))
		return false;

	active = true;
	carrier = clonk;

	if (clonk->GetAction() ne "Jump")
		clonk->SetAction("Jump");

	if (!clonk->GetEffect("IntSteampackHover", clonk))
		clonk->AddEffect("IntSteampackHover", clonk, 1, 1, this);

	return true;
}

public func Deactivate()
{
	active = false;
	if (carrier)
	{
		var eff = carrier->GetEffect("IntSteampackHover", carrier);
		if (eff) carrier->RemoveEffect("IntSteampackHover", carrier, eff);
	}
	return true;
}

public func IsActive() { return active; }

public func FxIntSteampackHoverTimer(object target, int effect, int time)
{
	if (!active || !carrier) return -1;

	if (!Aero_BurnTick(10))
	{
		Deactivate();
		return -1;
	}

	carrier->SetYDir(-3);
	carrier->SetXDir(Aero_WindDrift(50));

	return 1;
}

public func FxSteampackControlControlUp(object target)
{
	return Activate(target);
}

public func FxSteampackControlControlDown(object target)
{
	return Deactivate();
}

protected func Initialize()
{
	active = false;
	carrier = nil;
	return true;
}
