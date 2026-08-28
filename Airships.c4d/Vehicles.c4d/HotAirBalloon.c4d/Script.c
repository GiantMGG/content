/*-- HotAirBalloon (HABL) -- storm scout, altitude steering --
  A Balloon.c4d-pattern observation balloon with a burner that scales
  a heat local into the [Physical] Float value. Carries 1 clonk + 1
  cargo item. Steering by altitude: the pilot controls vertical
  velocity (ControlUp/ControlDown adjust target_y), and the balloon's
  XDir is set entirely from GetWind()/4 at its current altitude,
  scaled by the weather wind_pct. Fuel: WOOD (value 40). While
  fuelled, heat rises toward ceiling 100 and Float scales with heat.
  Without fuel, heat decays and the balloon descends. --*/

#strict 2

#include AERO
#include FUEL

local heat;
local target_y;

public func IsFuel() { return false; }

protected func Initialize()
{
	heat = 0;
	target_y = -1;
	SetAction("Float");
	SetPhysical("Float", 0, PHYS_Temporary, this);
	return true;
}

public func OnBurn(int need)
{
	heat = BoundBy(heat + 10, 0, 100);
}

public func BurnerTimer()
{
	var mods = Aero_WeatherModifier();
	var lift_pct = mods[0];
	var wind_pct = mods[1];
	var burn_pct = mods[2];

	var need = 5 * burn_pct / 100;
	if (Burn_Consume(this, need))
	{
		/* Fuelled: heat already advanced by OnBurn. */
	}
	else
	{
		/* No fuel: heat decays. */
		heat = BoundBy(heat - 5, 0, 100);
	}

	/* Float scales with heat and lift_pct. */
	SetPhysical("Float", heat * lift_pct / 100, PHYS_Temporary, this);

	/* Altitude steering: XDir from wind at current position, scaled
	   by the weather wind_pct. Wind drift is the only horizontal
	   force — the pilot steers by choosing an altitude layer. */
	SetXDir(BoundBy(GetWind() / 4 * wind_pct / 100, -10, 10));

	/* Steer toward target_y if the pilot has set one. */
	if (target_y != -1)
	{
		SetYDir(BoundBy((target_y - GetY()) / 4, -10, 10));
		if (Inside(GetY(), target_y - 5, target_y + 5))
			target_y = -1;
	}

	return true;
}

/* ---- Altitude steering controls ----
   ControlUp/ControlDown adjust target_y; the balloon's XDir is set
   entirely from GetWind()/4 at its current altitude. */

public func ControlUp(object controller)
{
	ClearCommand();
	if (SEqual(GetAction(), "WaterLand")) SetAction("WaterLaunch");
	if (!GetPlrJumpAndRunControl(controller->GetController()))
	{
		if (GetComDir() == COMD_Down)
			SetDirection(COMD_Stop);
		else
			SetDirection(COMD_Up);
	}
}

public func ControlDownSingle(object controller)
{
	ClearCommand();
	if (!GetPlrJumpAndRunControl(controller->GetController()))
	{
		if (GetComDir() == COMD_Up)
			SetDirection(COMD_Stop);
		else
			SetDirection(COMD_Down);
	}
}

public func ControlDown(object controller) { ClearCommand(); }

public func ControlUpdate(object controller, int comdir)
{
	SetDirection(comdir);
}

public func SetDirection(int comdir)
{
	if (comdir == COMD_Up || comdir == COMD_UpLeft || comdir == COMD_UpRight)
		SetComDir(COMD_Up);
	else if (comdir == COMD_Down || comdir == COMD_DownLeft || comdir == COMD_DownRight)
		SetComDir(COMD_Down);
	else
	{
		ScheduleCall(this(), "ClearDir", 1, Abs(GetYDir()));
		SetComDir(COMD_Stop);
	}
}

private func ClearDir()
{
	if (GetYDir())
	{
		if (GetYDir() > 0) SetYDir(GetYDir() - 1);
		else SetYDir(GetYDir() + 1);
	}
}

protected func ControlCommand(string szCommand, object pTarget, int iX, int iY)
{
	if (SEqual(szCommand, "UnGrab"))
	{
		ClearCommand();
		return 0;
	}
	if (SEqual(szCommand, "MoveTo"))
	{
		target_y = iY;
		return true;
	}
}

private func ClearCommand() { target_y = -1; }

protected func RejectCollect(id idObject, object pObj)
{
	if (DefinitionCall(idObject, "IsArrow"))
		if (GetXDir(pObj) || GetYDir(pObj))
		{
			if (OnFire(pObj)) Incinerate();
			else SetAction("DropOff");
			ProtectedCall(pObj, "Hit");
			return true;
		}
	if (ContentsCount() >= 4)
		return true;
	return false;
}

public func GetResearchBase() { return AIRS; }
