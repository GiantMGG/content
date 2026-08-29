/*-- Sandstorm (SNDT) -- desert weather event. --*/
/* Follows the Storm.c4d event contract: Start/Execute/Stop, idempotent  */
/* Stop, baseline capture in Construction.                               */

#strict

local baseline_wind;

public func Construction()
{
	baseline_wind = GetWind(0, 0, true);
	return 1;
}

public func Start()
{
	Log("The sky turns ochre -- a sandstorm is coming.");
	SetSkyFade(196, 164, 110, 216, 190, 134);
}

public func Execute()
{
	var intensity = GetWeatherEventIntensity();
	// Ramp the wind toward gale strength.
	SetWind(BoundBy(baseline_wind + 40 + intensity / 2, -100, 100));
	// Stream FlySand PXS across the landscape (visual only).
	// CastPXS is object-relative and the event object sits at
	// (LandscapeWidth()/2, 0), so subtract the event object's own
	// position to make the random x cover the whole map.
	CastPXS("FlySand", 8 + intensity / 8, 25,
		Random(LandscapeWidth()) - GetX(), Random(20) - GetY());
}

public func Stop()
{
	// Idempotent (Storm contract). Sky restoration is handled by
	// SetSeasonGamma on the next season tick.
	SetWind(baseline_wind);
}
