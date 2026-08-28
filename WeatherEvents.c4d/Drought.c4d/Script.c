/*-- Drought (DRGT) -- temperature soars, water evaporates, no rain. --*/

#strict

// Shared contract — see Storm.c4d/Script.c header. Stop is idempotent.

local baseline_temp;
local evap_budget;  // pixels of Water to evaporate per Execute tick

public func Construction()
{
	baseline_temp = GetTemperature();
	evap_budget   = 4;
	return 1;
}

public func Start()
{
	Log("The land withers under a drought.");
	SetTemperature(70);
}

public func Execute()
{
	SetTemperature(70);

	// Evaporate a small fraction of surface Water pixels per tick.
	// Scan a thin band across the landscape; cap work per tick to keep
	// O(n) bounded (spec §Edge cases #8 — performance cap).
	var wdt = LandscapeWidth();
	var hgt = LandscapeHeight();
	var evaporated = 0;
	for (var x = 0; x < wdt && evaporated < evap_budget; x += 5)
	{
		// Walk up from the bottom looking for Water material.
		for (var y = hgt - 1; y > 0; --y)
		{
			if (GetMaterial(x, y) == Material("Water"))
			{
				ExtractLiquid(x, y, 1);
				++evaporated;
				break;  // one pixel per column per tick
			}
		}
	}
}

public func Stop()
{
	SetTemperature(baseline_temp);
}
