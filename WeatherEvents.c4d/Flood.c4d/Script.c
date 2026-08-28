/*-- Flood (FLDD) -- sea level rises then recedes. --*/

#strict

// Shared contract — see Storm.c4d/Script.c header. Stop is idempotent.
//

// Phases: 0=rising (CastPXS Water along bottom edge), 1=receding
// (ExtractLiquid along bottom edge). Phase split is half/half of the
// event duration; we approximate using tick_counter.

local tick_counter;
local total_duration;

public func Construction()
{
	tick_counter   = 0;
	total_duration = 0;
	return 1;
}

public func Start()
{
	// EventDuration is only valid from Start onward (LaunchWeatherEvent
	// sets it after CreateObject returns), so the rising/receding split
	// is read here.
	total_duration = GetWeatherEventDuration();
	Log("Waters rise — the flood is coming!");
}

public func Execute()
{
	++tick_counter;
	var wdt = LandscapeWidth();
	var hgt = LandscapeHeight();
	var rising = (tick_counter * 2 < total_duration);
	var waterMat = Material("Water");

	if (rising)
	{
		// Cast a small amount of Water along the bottom edge.
		for (var x = 0; x < wdt; x += 20)
			CastPXS(waterMat, 30, 20, x, hgt - 5);
	}
	else
	{
		// Recede: extract liquid from the bottom band.
		for (var x = 0; x < wdt; x += 20)
		{
			if (GetMaterial(x, hgt - 5) == waterMat)
				ExtractLiquid(x, hgt - 5, 5);
		}
	}
}

public func Stop()
{
	// Nothing to restore — the recession phase handled cleanup.
}
