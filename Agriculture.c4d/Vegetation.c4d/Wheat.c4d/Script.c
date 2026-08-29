/*-- Weizen --*/

#strict 3

// Weather-event C4IDs. These are C4ID literals (4 uppercase letters),
// recognised by the tokenizer in any strict mode; no declaration needed.

local grow_stage; // 0=Seedling, 1=Growing, 2=Ready

protected func Construction()
{
	grow_stage = 0;
	SetAction("Seedling");
	return(1);
}

public func IsWheat() { return(1); }

public func IsRipe() { return(GetAction() == "Ready"); }

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
			if (GetAction() == "Seedling") { RemoveObject(); return 1; }
			return 1;  // pause
		}
		if (evt == DRGT && GetAction() == "Seedling")  // drought: no germination
			return 1;
		// HTWV / STRM / FLDD: growth continues (heatwave may even boost)
	}

	if (GetAction() == "Seedling") { SetAction("Growing"); Sound("Dig?"); return 1; }
	if (GetAction() == "Growing")  { SetAction("Ready");   Sound("Chop?"); return 1; }
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
