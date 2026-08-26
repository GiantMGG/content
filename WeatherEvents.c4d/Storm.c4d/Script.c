/*-- Storm (STRM) -- showcase multi-phase weather event. --*/

#strict

// Shared contract (every event object implements these five callbacks):
//   Start()    — read GetWeatherEventIntensity/Duration; begin ramping
//   Stop()     — restore wind/temperature/gamma to pre-event baseline
//   Execute()  — TimerCall; per-tick event effects
//   OnCropGrow / OnFireSpread are NOT called by the engine — they are
//   documentation of the contract crops/fire consumers query via
//   GetActiveWeatherEvent(). Crops branch on the returned C4ID.
//
// Stop must be idempotent and must NOT call LaunchWeatherEvent.

local phase;       // 0=gathering, 1=raging, 2=clearing
local phase_timer; // ticks remaining in current phase
local baseline_wind, baseline_temp;

public func Construction()
{
	phase = 0;
	phase_timer = GetWeatherEventDuration() / 3; // rough 3-phase split
	baseline_wind = GetWind(0, 0, true);
	baseline_temp = GetTemperature();
	return 1;
}

public func Start()
{
	GameMsg("The clouds gather…");
	SetSkyFade(20, 20, 30, 60, 60, 80);  // darken sky
}

public func Execute()
{
	var intensity = GetWeatherEventIntensity();

	if (phase == 0)  // gathering: ramp wind, spawn rain clouds
	{
		SetWind(BoundBy(baseline_wind + (50 + intensity / 3), -100, 100));
		if (!Random(30)) LaunchCloud(Random(LandscapeWidth()), -1,
		                             LandscapeWidth() / 10, 5, "Water");
	}
	else if (phase == 1)  // raging: lightning barrage
	{
		if (!Random(15)) LaunchLightning(Random(LandscapeWidth()), 0,
		                                 -20, 41, +5, 15, true);
	}
	else  // clearing: decay wind, restore gamma
	{
		SetWind(BoundBy(GetWind(0,0,true) - 2, baseline_wind, 100));
	}

	if (--phase_timer <= 0)
	{
		phase++;
		if (phase == 1) { GameMsg("Hold fast — the storm is upon us!"); phase_timer = GetWeatherEventDuration() / 2; }
		else if (phase == 2) { GameMsg("The storm passes."); phase_timer = GetWeatherEventDuration() / 6; }
	}
}

public func Stop()
{
	SetWind(baseline_wind);
	SetTemperature(baseline_temp);
	// gamma restoration handled by SetSeasonGamma on next season tick
}
