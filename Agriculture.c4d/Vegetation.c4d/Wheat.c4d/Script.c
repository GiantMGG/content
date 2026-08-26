/*-- Weizen --*/

#strict

// Weather-event C4IDs. Declared locally to avoid an #include dependency
// from Agriculture on the WeatherEvents pack (which would force every
// scenario loading Agriculture to also load WeatherEvents).
static const C4ID BLZD = C4Id("BLZD");
static const C4ID DRGT = C4Id("DRGT");
static const C4ID STRM = C4Id("STRM");
static const C4ID HTWV = C4Id("HTWV");
static const C4ID FLDD = C4Id("FLDD");

local grow_stage; // 0=Seedling, 1=Growing, 2=Ready

protected func Construction()
{
	grow_stage = 0;
	SetAction("Seedling");
	return(1);
}

public func IsWheat() { return(1); }

public func IsRipe() { return(GetAction() eq "Ready"); }

/* TimerCall — advances through the 3 phases once each. Weather events
   can pause or kill growth; see WeatherEvents.c4d pack. */
public func Grow()
{
	// Weather-event coupling: certain events pause or kill crop growth.
	var evt = GetActiveWeatherEvent();
	if (evt != nil)
	{
		if (evt == BLZD)  // blizzard: seedlings die, others pause
		{
			if (GetAction() eq "Seedling") { RemoveObject(); return 1; }
			return 1;  // pause
		}
		if (evt == DRGT && GetAction() eq "Seedling")  // drought: no germination
			return 1;
		// HTWV / STRM / FLDD: growth continues (heatwave may even boost)
	}

	if (GetAction() eq "Seedling") { SetAction("Growing"); Sound("Dig?"); return 1; }
	if (GetAction() eq "Growing")  { SetAction("Ready");   Sound("Chop?"); return 1; }
	// Already ripe: nothing to do.
	return 1;
}

/* Harvested by the Sickle (see Tools/Sickle.c4d). Returns 2 WheatSheaf. */
public func Harvest(object pClonk)
{
	if (!IsRipe()) return(0);
	var i;
	for (i = 0; i < 2; i++)
	{
		var pSheaf = CreateObject(AGSH, 0, 0, GetOwner());
		if (pClonk) pClonk->Collect(pSheaf);
		else pSheaf->Exit(RandomX(-5, 5), -5);
	}
	RemoveObject();
	return(1);
}
