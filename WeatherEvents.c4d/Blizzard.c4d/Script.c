/*-- Blizzard (BLZD) -- temperature plunges, snow falls, crops die. --*/

#strict

// Shared contract — see Storm.c4d/Script.c header for the five-callback
// contract. Stop is idempotent and does not call LaunchWeatherEvent.

local baseline_wind, baseline_temp;

public func Construction()
{
	baseline_wind = GetWind(0, 0, true);
	baseline_temp = GetTemperature();
	return 1;
}

public func Start()
{
	GameMsg("A blizzard descends!");
	SetTemperature(-30);
	SetWind(BoundBy(baseline_wind + 80, -100, 100));
}

public func Execute()
{
	// Hold the cold; spawn snow clouds.
	SetTemperature(-30);
	if (!Random(20)) LaunchCloud(Random(LandscapeWidth()), -1,
	                             LandscapeWidth() / 12, 6, "Snow");
}

public func Stop()
{
	SetWind(baseline_wind);
	SetTemperature(baseline_temp);
}
