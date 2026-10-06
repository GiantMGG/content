/*-- Frontier: Green Vale -- dealt goal cards + announced fronts (cycle 199,
  roadmap frontier-mvp). Outset.c4s is the Free Game pin; its [Landscape]
  sliders carry the Green Vale preset, and this script deals the two goal
  cards (GSTV Starving Season + GHWV High Water Harvest), spawns the
  scenario-local GRNY granary, runs the FRHunger driver through the DRGT
  window, launches DRGT and FLDD via the stock LaunchWeatherEvent platform
  (EventSmoke.c4s precedent), delivers the flood bite as the FRSweep claim
  over the floodplain band, and calls GameOver once both cards resolve.
  Wolves stay ambient (WLSP rule). --*/

#strict 2

// ---------------- schedule (35-tick lattice; all beats % 35 == 0) --------
static const FR_T_FORECAST = 70;     // forecast announce
static const FR_T_DRGT     = 4200;   // drought onset (2:00)
static const FR_DRGT_LEN   = 4200;   // drought window (2:00)
static const FR_T_FLDD_ANN = 8750;   // flood announce (4:00 lead)
static const FR_T_FLDD     = 17150;  // flood onset
static const FR_FLDD_LEN   = 3500;   // flood window
static const FR_T_SWEEP    = 18900;  // claim sweep at flood peak
static const FR_T_EVAL     = 20650;  // both cards resolved

// ---------------- hunger tuning ------------------------------------------
static const FR_HUNGER_DRAIN = 2;    // energy points per 35-tick drain
static const FR_HUNGER_FLOOR = 15;   // drain stops above lethality

// ---------------- floodplain band ----------------------------------------
static const FR_BAND_MARGIN = 60;    // px above the waterline = floodplain

// ---------------- card state (0 pending, 1 won, -1 lost) -----------------
static g_fr_c3;
static g_fr_c4;
static g_fr_grny;

global func FRLatticeFatal(string name)
{
	FatalError(Format("Frontier lattice: %s is not a multiple of 35", name));
	return false;
}

global func FRSurfaceY(int x)
{
	var y = 10;
	while (y < LandscapeHeight() - 5 && !GBackSolid(x, y)) y++;
	return y;
}

// Topmost standing-liquid y across sampled columns (the waterline).
global func FRWaterY()
{
	var wdt = LandscapeWidth();
	var x, y, best = LandscapeHeight();
	for (x = wdt / 10; x < wdt; x += wdt / 10)
	{
		y = 0;
		while (y < LandscapeHeight() && !GBackLiquid(x, y)) y++;
		if (y < best) best = y;
	}
	return best;
}

global func FRInBand(int x, int y)
{
	return y >= FRWaterY() - FR_BAND_MARGIN;
}

global func FRAnnounce(string msg)
{
	var i;
	for (i = 0; i < GetPlayerCount(); i++)
		CustomMessage(msg, 0, GetPlayerByIndex(i), 0, 0, 0xffffff, 0, 0, MSG_Bottom, 300);
	Log(msg);
	return true;
}

protected func Initialize()
{
	// Lattice self-check (Sturmfront.c4s:227-243 pattern -- the cycle-175
	// silent-endgame lesson: every one-shot beat must sit on the lattice).
	if (FR_T_FORECAST % 35 != 0) return FRLatticeFatal("FR_T_FORECAST");
	if (FR_T_DRGT % 35 != 0) return FRLatticeFatal("FR_T_DRGT");
	if ((FR_T_DRGT + FR_DRGT_LEN) % 35 != 0) return FRLatticeFatal("FR_T_DRGT+FR_DRGT_LEN");
	if (FR_T_FLDD_ANN % 35 != 0) return FRLatticeFatal("FR_T_FLDD_ANN");
	if (FR_T_FLDD % 35 != 0) return FRLatticeFatal("FR_T_FLDD");
	if ((FR_T_FLDD + FR_FLDD_LEN) % 35 != 0) return FRLatticeFatal("FR_T_FLDD+FR_FLDD_LEN");
	if (FR_T_SWEEP % 35 != 0) return FRLatticeFatal("FR_T_SWEEP");
	if (FR_T_EVAL % 35 != 0) return FRLatticeFatal("FR_T_EVAL");

	g_fr_c3 = 0;
	g_fr_c4 = 0;

	// The deal: spawn the two goal cards and print both objectives
	// (display path: Activate -> MessageWindow(GetDesc()), Goal.c4d:156-160).
	var pC3 = CreateObject(GSTV, 0, 0, NO_OWNER);
	var pC4 = CreateObject(GHWV, 0, 0, NO_OWNER);
	if (!pC3 || !pC4) FatalError("Frontier: goal card spawn failed");
	FRAnnounce("CURRENT GOAL -- A drought is coming -- plant before it, and keep every clonk fed. Wheat needs 3:20 to ripen: sow NOW. -- Drought at 2:00 -- counter: 8 edibles in the granary at drought end");
	FRAnnounce("CURRENT GOAL -- The river tops the floodplain in 4:00 -- anything on the ground is forfeit. Bank your sheaves. -- counter: 10 banked (granary + carried) at flood end");

	AddEffect("FrontDirector", 0, 1, 35, 0, 0);
	AddEffect("FRHunger", 0, 1, 35, 0, 0);
	return true;
}

global func FxFrontDirectorTimer(target, effect, time)
{
	var t = FrameCounter();

	// Granary near the first crew (one spawn, survives the whole round).
	if (t == 35 && !g_fr_grny)
	{
		var p, i, c;
		for (i = 0; i < GetPlayerCount() && !p; i++)
			for (c = 0; c < GetCrewCount(GetPlayerByIndex(i)) && !p; c++)
				p = GetCrew(GetPlayerByIndex(i), c);
		var gx = LandscapeWidth() / 2;
		if (p) gx = GetX(p) + 60;
		if (gx < 40) gx = 40;
		if (gx > LandscapeWidth() - 40) gx = LandscapeWidth() - 40;
		var gy = FRSurfaceY(gx);
		g_fr_grny = CreateObject(GRNY, gx, gy, NO_OWNER);
		Log(Format("FRMT:grny_spawn=%d,%d", gx, gy));
	}

	if (t == FR_T_FORECAST)
		FRAnnounce("Green Vale -- drought at 2:00, high water after. Sow wheat now -- it needs 3:20 to ripen. Bank early.");

	if (t == FR_T_DRGT)
	{
		LaunchWeatherEvent(DRGT, 50, FR_DRGT_LEN);
		FRAnnounce("The drought is upon us -- keep every clonk fed. 8 edibles in the granary when it breaks.");
		Log("Frontier: drought begins");
	}
	if (t == FR_T_DRGT + FR_DRGT_LEN)
	{
		StopWeatherEvent();
		Log("Frontier: drought ends");
		FREvalC3();
	}

	if (t == FR_T_FLDD_ANN)
		FRAnnounce("High water in 4:00 -- anything on the floodplain's low ground is forfeit. Bank your sheaves.");
	if (t == FR_T_FLDD)
	{
		LaunchWeatherEvent(FLDD, 50, FR_FLDD_LEN);
		FRAnnounce("The river tops the floodplain!");
		Log("Frontier: flood begins");
	}
	if (t == FR_T_SWEEP)
	{
		var n = FRSweep();
		Log(Format("Frontier: claim sweep -- %d claimed", n));
	}
	if (t == FR_T_FLDD + FR_FLDD_LEN)
	{
		StopWeatherEvent();
		Log("Frontier: flood ends");
		FREvalC4();
		FRFinish();
	}
	return 1;
}

// Hunger driver: stock clonks never starve (C4Object.cpp:810 burn-path
// only); the driver IS the famine. Floor keeps the base rate non-lethal.
global func FxFRHungerTimer(target, effect, time)
{
	var t = FrameCounter();
	if (t < FR_T_DRGT || t >= FR_T_DRGT + FR_DRGT_LEN) return 1;
	var i, c, p;
	for (i = 0; i < GetPlayerCount(); i++)
		for (c = 0; c < GetCrewCount(GetPlayerByIndex(i)); c++)
		{
			p = GetCrew(GetPlayerByIndex(i), c);
			if (!p) continue;
			if (GetEnergy(p) <= FR_HUNGER_FLOOR) continue;
			DoEnergy(-FR_HUNGER_DRAIN, p);
		}
	return 1;
}

// C3: at drought end, granary edibles >= 8 wins.
global func FRGranaryEdibles()
{
	var g = FindObject(GRNY);
	if (!g) return 0;
	return ContentsCount(AGSH, g) + ContentsCount(AGAP, g);
}

global func FREvalC3()
{
	var n = FRGranaryEdibles();
	g_fr_c3 = 1;
	if (n < 8) g_fr_c3 = -1;
	Log(Format("FRMT:c3_resolved=%d edibles=%d", g_fr_c3, n));
	return true;
}

// C4: at flood end, banked = GRNY AGSH + crew-carried AGSH >= 10.
global func FREvalC4()
{
	var g = FindObject(GRNY);
	var banked = 0;
	if (g) banked = ContentsCount(AGSH, g);
	var i, c, p;
	for (i = 0; i < GetPlayerCount(); i++)
		for (c = 0; c < GetCrewCount(GetPlayerByIndex(i)); c++)
		{
			p = GetCrew(GetPlayerByIndex(i), c);
			if (p) banked += ContentsCount(AGSH, p);
		}
	g_fr_c4 = 1;
	if (banked < 10) g_fr_c4 = -1;
	Log(Format("FRMT:c4_resolved=%d banked=%d", g_fr_c4, banked));
	return true;
}

global func FRFinish()
{
	if (g_fr_c3 == 0 || g_fr_c4 == 0) return false;
	if (g_fr_c3 == 1 && g_fr_c4 == 1) Log("FRMT:outcome=WIN");
	else Log(Format("FRMT:outcome=LOSE|c3=%d c4=%d", g_fr_c3, g_fr_c4));
	GameOver();
	return true;
}

// Claim rule: loose AGSH in the floodplain band is forfeit; in-ground AGWH
// under liquid is swept (SFUnderLiquid 12-px stem read, Sturmfront:580-586).
global func FRUnderLiquid(object pW)
{
	if (GBackLiquid(GetX(pW), GetY(pW))) return true;
	if (GBackLiquid(GetX(pW), GetY(pW) - 6)) return true;
	if (GBackLiquid(GetX(pW), GetY(pW) - 12)) return true;
	return false;
}

global func FRSweep()
{
	var n = 0;
	var pS;
	for (pS in FindObjects(Find_ID(AGSH)))
		if (!pS->Contained() && FRInBand(GetX(pS), GetY(pS)))
		{
			RemoveObject(pS);
			++n;
		}
	var pW;
	for (pW in FindObjects(Find_ID(AGWH)))
		if (FRUnderLiquid(pW))
		{
			RemoveObject(pW);
			++n;
		}
	return n;
}
