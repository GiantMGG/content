/*-- HotAirBalloon (HABL) -- storm scout, altitude steering --
  A Balloon.c4d-pattern observation balloon with a burner that scales
  a heat local into the [Physical] Float value. Carries 1 clonk + 1
  cargo item. Steering by altitude: XDir is set from GetWind() at the
  balloon's current position. Fuel: WOOD (value 40). While fuelled,
  heat rises toward ceiling 100 and Float scales with heat. Without
  fuel, heat decays and the balloon descends. --*/

#strict 2

#include AERO

local heat;

public func IsFuel() { return false; }

protected func Initialize()
{
	heat = 0;
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
	var burn_pct = mods[2];

	var need = 5 * burn_pct / 100;
	if (Aero_Burn(this, need))
	{
	}
	else
	{
		heat = BoundBy(heat - 5, 0, 100);
	}

	SetPhysical("Float", heat * lift_pct / 100, PHYS_Temporary, this);

	SetXDir(GetWind() / 4);

	return true;
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
	if (ContentsCount() >= 4)
		return true;
	return false;
}

public func GetResearchBase() { return AIRS; }
