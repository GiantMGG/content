/*-- Heatwave (HTWV) -- sharp temperature spike, accelerates fire spread. --*/

#strict

// Shared contract — see Storm.c4d/Script.c header. Stop is idempotent.
//
// Performance note (spec §Edge #8): Execute scans for FLAM objects but
// caps the scan to at most 10 FLAMs per tick and only runs the
// Incinerate-neighbors logic every 5th call (throttled by tick_counter).

local baseline_temp;
local tick_counter;

public func Construction()
{
	baseline_temp = GetTemperature();
	tick_counter  = 0;
	return 1;
}

public func Start()
{
	Log("A blistering heatwave grips the land.");
	SetTemperature(85);
}

public func Execute()
{
	SetTemperature(85);

	// Throttle: fire-spread acceleration runs every 5th Execute call.
	if (++tick_counter < 5) return;
	tick_counter = 0;

	// Find at most 10 FLAM objects and Incinerate their neighbors.
	var processed = 0;
	var flame = FindObject(FLAM, 0, 0, -1, -1);
	while (flame && processed < 10)
	{
		// Incinerate a random neighbor pixel of the flame.
		var nx = flame->GetX() + RandomX(-3, 3);
		var ny = flame->GetY() + RandomX(-3, 3);
		if (GBackSemiSolid(nx, ny))
			Incinerate(nx, ny);
		++processed;
		flame = FindObject(FLAM, 0, 0, -1, -1, 0, 0, 0, flame);
	}
}

public func Stop()
{
	SetTemperature(baseline_temp);
}
