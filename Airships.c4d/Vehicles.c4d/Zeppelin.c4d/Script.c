/*-- Zeppelin (ZEPN) -- heavy lift, weather-fragile --
  A scaled-up Blimp.c4d. Lift is continuous and fuel-gated: the burner
  consumes fuel each tick; while fuelled, Float=100 holds; when
  Burn_Consume returns false, Float drops to 40 (sag, not crash).
  STRM at intensity >=80 clamps XDir to +/-20 (wind shear). --*/

#strict 2

#include AERO
#include FUEL

local fuel_out;

public func IsFuel() { return false; }

protected func Initialize()
{
	fuel_out = false;
	SetAction("FloatIdle");
	SetPhysical("Float", 100, PHYS_Temporary, this);
	return true;
}

public func OnBurn(int need)
{
	SetPhysical("Float", 100, PHYS_Temporary, this);
	fuel_out = false;
}

public func BurnerTimer()
{
	var mods = Aero_WeatherModifier();
	var lift_pct = mods[0];
	var wind_pct = mods[1];
	var burn_pct = mods[2];

	var need = 10 * burn_pct / 100;
	if (Burn_Consume(this, need))
	{
		SetPhysical("Float", 100 * lift_pct / 100, PHYS_Temporary, this);
		fuel_out = false;
	}
	else
	{
		SetPhysical("Float", 40, PHYS_Temporary, this);
		fuel_out = true;
	}

	var clamp = 10;
	var event = GetActiveWeatherEvent();
	if (event == C4Id("STRM") && GetWeatherEventIntensity() >= 80)
		clamp = 20;
	var drift = BoundBy(GetWind() / 4 * wind_pct / 100, -clamp, clamp);
	SetXDir(drift);

	return true;
}

private func FloatProcess()
{
	NoPilotCheck();
	return true;
}

private func FlightProcess()
{
	NoPilotCheck();
	return true;
}

private func NoPilotCheck()
{
	if (!FindObject(0, 0, 0, 0, 0, 0, "Push", this()))
	{
		if (GetAction() != "FloatIdle")
			SetAction("FloatIdle");
		SetComDir(COMD_Down);
		return true;
	}
	return false;
}

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
	if (ContentsCount() >= 8)
		return true;
	return false;
}

public func GetResearchBase() { return AIRS; }
