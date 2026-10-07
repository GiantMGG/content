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
static const FR_T_FORECAST = 70;     // forecast announce (unchanged)
static const FR_T_DRGT     = 4200;   // drought onset (2:00, unchanged)
static const FR_DRGT_LEN   = 4200;   // drought window (unchanged)
static const FR_T_FLDD_ANN = 8750;   // flood announce (2:00 lead)
static const FR_T_FLDD     = 12950;  // flood onset
static const FR_FLDD_LEN   = 2450;   // flood window (12950-15400)
static const FR_RISE_LEN   = 700;    // rise 12950-13650 (20 director calls)
static const FR_HOLD_LEN   = 700;    // hold 13650-14350 (flood stands)
static const FR_RECEDE_LEN = 1050;   // recede 14350-15400 (30 director calls)
static const FR_T_SWEEP    = 13650;  // claim sweep at flood peak
static const FR_T_EVAL     = 15400;  // both cards resolved
static const FR_RISE_PX    = 40;     // rise ceiling above the onset waterline
// Dynamic-extent rates (ADDENDUM C): the flooded body is ~940 columns at
// seed 199 (lake 238 + east floodplain spread), ~37,600 cells to the
// 40-px ceiling over 20 rise calls -> 1880 cells/call. The recede drains
// every painted cell above the onset profile, so its budget must cover the
// same volume over 30 calls: ceil(1880 * 20 / 30) = 1254 cells/call.
static const FR_FLOOD_RATE  = 1880;  // rise cells per director call (dynamic body)
static const FR_RECEDE_RATE = 1254;  // recede cells per director call (covers rise volume)

// ---------------- hunger tuning ------------------------------------------
static const FR_HUNGER_DRAIN = 2;    // energy points per 35-tick drain
static const FR_HUNGER_FLOOR = 15;   // drain stops above lethality

// ---------------- floodplain band ----------------------------------------
static const FR_BAND_MARGIN = 60;    // px above the waterline = floodplain

// ---------------- card state (0 pending, 1 won, -1 lost) -----------------
static g_fr_c3;
static g_fr_c4;
static g_fr_grny;

// ---------------- flood runtime state (snapshotted once at onset) --------
static g_fr_base_wy;      // onset waterline (topmost standing-liquid y)
static g_fr_col0;         // per-column onset surface (recede target; LandscapeHeight = dry)
static g_fr_wet_n;        // onset wet-span width (geom telemetry)
static g_fr_rise_cursor;  // rise round-robin column cursor
static g_fr_recede_x;     // recede round-robin column cursor
static g_fr_warm_base;    // ambient temperature captured at the warm front

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
	if (FR_RISE_LEN % 35 != 0) return FRLatticeFatal("FR_RISE_LEN");
	if (FR_HOLD_LEN % 35 != 0) return FRLatticeFatal("FR_HOLD_LEN");
	if (FR_RECEDE_LEN % 35 != 0) return FRLatticeFatal("FR_RECEDE_LEN");
	if (FR_RISE_LEN + FR_HOLD_LEN + FR_RECEDE_LEN != FR_FLDD_LEN)
		return FRLatticeFatal("FR_RISE_LEN+FR_HOLD_LEN+FR_RECEDE_LEN == FR_FLDD_LEN");
	if (FR_T_EVAL != FR_T_FLDD + FR_FLDD_LEN)
		return FRLatticeFatal("FR_T_EVAL == FR_T_FLDD + FR_FLDD_LEN");

	g_fr_c3 = 0;
	g_fr_c4 = 0;

	// The deal: spawn the two goal cards and print both objectives
	// (display path: Activate -> MessageWindow(GetDesc()), Goal.c4d:156-160).
	var pC3 = CreateObject(GSTV, 0, 0, NO_OWNER);
	var pC4 = CreateObject(GHWV, 0, 0, NO_OWNER);
	if (!pC3 || !pC4) FatalError("Frontier: goal card spawn failed");
	FRAnnounce("CURRENT GOAL -- A drought is coming -- plant before it, and keep every clonk fed. Wheat needs 3:20 to ripen: sow NOW. -- Drought at 2:00 -- counter: 8 edibles in the granary at drought end");
	FRAnnounce("CURRENT GOAL -- The river tops the floodplain -- anything on the ground is forfeit. Bank your sheaves. -- counter: 10 banked (granary + carried) at flood end");

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
	{
		// Warm front: capture the ambient, force spring, re-force every
		// director tick through flood end (ADDENDUM A). Water.c4m freezes
		// below -10; the seasonal curve crosses it shortly after drought
		// end, so without this the lake is ice at onset and painted water
		// cannot hold on the plain (T2 long-boot: wy_peak=222 vs base 218).
		g_fr_warm_base = GetTemperature();
		SetTemperature(70);
		Log(Format("FRMT:warm_front=on baseline=%d", g_fr_warm_base));
		FRAnnounce("A warm front breaks the winter -- high water in 2:00. Anything on the floodplain's low ground is forfeit -- bank your sheaves.");
	}
	if (t >= FR_T_FLDD_ANN && t < FR_T_EVAL) SetTemperature(70);
	if (t == FR_T_FLDD)
	{
		LaunchWeatherEvent(FLDD, 50, FR_FLDD_LEN);
		FRAnnounce("The river tops the floodplain!");
		Log("Frontier: flood begins");
		FRFloodSnapshot();
	}
	if (t >= FR_T_FLDD && t < FR_T_FLDD + FR_RISE_LEN) FRRiseTick();
	if (t == FR_T_SWEEP)
	{
		Log(Format("FRMT:wy_peak=%d", FRWaterY()));
		var n = FRSweep();
		Log(Format("Frontier: claim sweep -- %d claimed", n));
	}
	if (t >= FR_T_FLDD + FR_RISE_LEN + FR_HOLD_LEN && t < FR_T_FLDD + FR_FLDD_LEN)
		FRRecedeTick();
	if (t == FR_T_EVAL)
	{
		SetTemperature(g_fr_warm_base);
		Log("FRMT:warm_front=off");
		StopWeatherEvent();
		Log("Frontier: flood ends");
		Log(Format("FRMT:wy_end=%d", FRWaterY()));
		FREvalC4();
		FRFinish();
	}
	return 1;
}

// ---------------- physical flood (spec 4.1/4.2) ---------------------------

// Onset snapshot: discover the running river's geometry ONCE -- DRGT /
// post-window drainage has moved the waterline by onset (T1 probe: base_wy
// 210 pre-drought -> 218 at t=12950), so Initialize is the wrong time.
// Per-column top-liquid surface for EVERY column in [0, LandscapeWidth()):
// g_fr_col0[x] is the ONSET surface of column x (the per-column RECEDE
// target) and a dry column stores the LandscapeHeight() sentinel. The wet
// span (columns whose onset surface sits within +/-5 px of the discovered
// baseline -- the T1 probe tolerance, same body) feeds the geom telemetry
// only; the walks themselves re-discover the flood dynamically (ADDENDUM C).
global func FRFloodSnapshot()
{
	g_fr_base_wy = FRWaterY();
	g_fr_rise_cursor = 0;
	g_fr_recede_x = 0;
	var wdt = LandscapeWidth();
	g_fr_col0 = CreateArray(wdt);
	var tol_lo = g_fr_base_wy - 5;
	var tol_hi = g_fr_base_wy + 5;
	var i = 0, x, y;
	for (x = 0; x < wdt; x++)
	{
		y = 0;
		while (y < LandscapeHeight() && !GBackLiquid(x, y)) y++;
		g_fr_col0[x] = y;
		if (y >= tol_lo && y <= tol_hi) ++i;
	}
	g_fr_wet_n = i;
	Log(Format("FRMT:flood_geom base_wy=%d wet_span=%d", g_fr_base_wy, g_fr_wet_n));
	return true;
}

// Rise (dynamic extent): every call re-discovers the CURRENT flood body --
// a column is in the flood zone when its standing-liquid surface y lies in
// [base_wy - FR_RISE_PX, base_wy] (dry and above-ceiling columns are
// skipped) -- and paints one cell just above each flooded column's surface,
// row by row toward the ceiling. Water seeks level and spills east on its
// own; newly-flooded columns join the walk. Round-robin column cursor; the
// step guard (a full pass that paints nothing) ends the call early.
// FR_FLOOD_RATE cells per director call, sized for the ~940-column body.
global func FRRiseTick()
{
	var w = Material("Water");
	var wdt = LandscapeWidth();
	var n = 0, step = 0, x, y;
	while (n < FR_FLOOD_RATE)
	{
		x = g_fr_rise_cursor % wdt;
		++g_fr_rise_cursor;
		++step;
		y = 0;
		while (y < LandscapeHeight() && !GBackLiquid(x, y)) y++;
		if (y > g_fr_base_wy) { if (step > wdt) break; continue; }
		if (y <= g_fr_base_wy - FR_RISE_PX) { if (step > wdt) break; continue; }
		InsertMaterial(w, x, y - 1);
		++n;
		step = 0;
	}
	return true;
}

// Recede (dynamic extent): every call re-discovers the columns whose
// CURRENT surface is ABOVE their onset-snapshot surface (they gained flood
// water) and extracts one cell at each such column's current surface,
// walking down toward the per-column onset target. A column is NEVER
// extracted at or below its onset surface, so the standing lake below the
// baseline stays and dry-at-onset floodplain columns drain all the way back
// to dry (their target is the LandscapeHeight sentinel) -- the task's
// stranded-east-pool fix. FR_RECEDE_RATE cells per call covers the full
// painted volume over the 30 recede calls.
global func FRRecedeTick()
{
	var wdt = LandscapeWidth();
	var n = 0, step = 0, x, y, t;
	while (n < FR_RECEDE_RATE)
	{
		x = g_fr_recede_x % wdt;
		++g_fr_recede_x;
		++step;
		y = 0;
		while (y < LandscapeHeight() && !GBackLiquid(x, y)) y++;
		t = g_fr_col0[x];
		if (y < t)
		{
			ExtractLiquid(x, y);
			++n;
			step = 0;
		}
		if (step > wdt) break;
	}
	return true;
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
	if (g_fr_c3 == 1)
		FRAnnounce(Format("Drought broken -- %d edibles banked (need 8) -- the granary held", n));
	else
		FRAnnounce(Format("Drought broken -- %d edibles banked (need 8) -- the clonks go hungry", n));
	var v = "won";
	if (g_fr_c3 == -1) v = "lost";
	Log(Format("FRMT:c3_verdict=%s banked=%d", v, n));
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
