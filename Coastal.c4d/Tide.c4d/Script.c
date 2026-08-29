/*-- Tide (TIDE) -- cyclic sea-level weather event. --*/
/* Rising half casts Water PXS along the bottom edge; falling half extracts */
/* liquid. Mirrors the Flood.c4d pattern. Exposes the global queries        */
/* IsHighTide/IsLowTide/GetTideLevel consulted by Dock.CanMooreShip().      */
/* Stop is idempotent and performs no landscape restoration -- the falling  */
/* half already recedes the sea (spec edge case #1).                        */

#strict

local tick_counter;
local total_duration;
local rising;

public func Construction()
{
	tick_counter = 0;
	total_duration = 0;
	rising = true;
	return 1;
}

public func Start()
{
	total_duration = GetWeatherEventDuration();
	if (total_duration <= 0) total_duration = 100;
	rising = true;
	Log("The tide turns -- waters rise.");
}

public func Execute()
{
	++tick_counter;
	rising = (tick_counter * 2 < total_duration);
	var wdt = LandscapeWidth();
	var hgt = LandscapeHeight();
	var waterMat = Material("Water");

	if (rising)
	{
		for (var x = 0; x < wdt; x += 20)
			CastPXS(waterMat, 30, 20, x, hgt - 5);
	}
	else
	{
		for (var x = 0; x < wdt; x += 20)
		{
			if (GetMaterial(x, hgt - 5) == waterMat)
				ExtractLiquid(x, hgt - 5, 5);
		}
	}
}

public func Stop()
{
	// Idempotent. No restoration -- the falling half receded the sea.
}

/* ---- Instance accessor used by the global queries ---- */

public func IsRising() { return rising; }

/* ---- Global tide queries (consulted by Dock.CanMooreShip + scenarios) ---- */

global func IsHighTide()
{
	var pTide = FindObject(TIDE);
	if (!pTide) return false;
	return pTide->IsRising();
}

global func IsLowTide()
{
	var pTide = FindObject(TIDE);
	if (!pTide) return false;
	return !pTide->IsRising();
}

global func GetTideLevel()
{
	// Cached scan up the reference column to the topmost Water pixel.
	var iRefX = GetTideReferenceX();
	var hgt = LandscapeHeight();
	var waterMat = Material("Water");
	for (var y = hgt - 1; y >= 0; --y)
	{
		if (GetMaterial(iRefX, y) == waterMat)
			return y;
	}
	return 0;
}

global func GetTideReferenceX()
{
	var pDock = FindObject(DKST);
	if (pDock) return GetX(pDock);
	return LandscapeWidth() / 2;
}
