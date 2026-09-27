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
		// Cast a small amount of Water along the bottom edge. CastPXS
		// takes a material NAME string, not a Material() int (the int
		// form logged one [error] per call -- cycle-175 fix). NOTE: this
		// kills the error spam; it does NOT deliver water. CastPXS water
		// never integrates into the landscape at this engine tip (probe
		// evidence .opencode/scratch/175/probe/EVIDENCE.md, runs 3-4;
		// roadmap chore castpxs-water-no-integration tracks the engine
		// side). Sturmfront-style scripted floods deliver via
		// InsertMaterial instead.
		for (var x = 0; x < wdt; x += 20)
			CastPXS("Water", 30, 20, x, hgt - 5);
	}
	else
	{
		// Recede: extract liquid from the bottom band (ExtractLiquid
		// takes exactly (x, y) -- the former third argument was ignored).
		for (var x = 0; x < wdt; x += 20)
		{
			if (GetMaterial(x, hgt - 5) == waterMat)
				ExtractLiquid(x, hgt - 5);
		}
	}
}

public func Stop()
{
	// Nothing to restore — the recession phase handled cleanup.
}
