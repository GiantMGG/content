/*-- Frontier: Breaker Cove -- the C1 Beacon round (cycle 217, roadmap
  frontier-cove). BreakerCove.c4s is a free-game world: its [Landscape]
  sliders carry the Breaker Cove preset (sea over the lower third,
  border sealed via AutoScanSideOpen=0), and this script deals exactly
  one card -- C1 The Beacon. The scenario-stamped lighthouse starts DARK
  (a LocalN flip; Lighthouse.c4d defaults untouched), relight burns one
  WOOD (kindling rule, scenario-owned), FOGG fires at 2:00 via the stock
  LaunchWeatherEvent platform (700-tick window), win = a lit LHGT at
  fog end, GameOver at t=4900 (~2:20). No [WeatherEvents] ambient table:
  the dealt FOGG is the only front, exactly like Outset. --*/

#strict 2

// ---------------- schedule (35-tick lattice; every beat % 35 == 0) ------
static const BC_T_STAMP    = 35;    // stamps + dark start
static const BC_T_FORECAST = 70;    // forecast announce (the fantasy line)
static const BC_T_FOG_ANN  = 2450;  // fog countdown announce (~50 s lead)
static const BC_T_FOG      = 4200;  // fog onset (2:00)
static const BC_FOG_LEN    = 700;   // fog window (4200-4900)
static const BC_T_EVAL     = 4900;  // verdict + finish (~2:20)
static const BC_RELIGHT_RADIUS = 45; // kindling radius around the tower

// ---------------- card state (0 pending, 1 won, -1 lost) ----------------
static g_bc_c1;
static g_bc_lhgt;   // the stamped tower (C1's lighthouse)

global func BCLatticeFatal(string name)
{
	FatalError(Format("Breaker Cove lattice: %s is not a multiple of 35", name));
	return false;
}

// First solid y at column x (the surface line), Outset FRSurfaceY mirror.
global func BCSurfaceY(int x)
{
	var y = 10;
	while (y < LandscapeHeight() - 5 && !GBackSolid(x, y)) y++;
	return y;
}

// Topmost standing-liquid y across sampled columns (the waterline),
// Outset FRWaterY mirror.
global func BCWaterY()
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

global func BCAnnounce(string msg)
{
	var i;
	for (i = 0; i < GetPlayerCount(); i++)
		CustomMessage(msg, 0, GetPlayerByIndex(i), 0, 0, 0xffffff, 0, 0, MSG_Bottom, 300);
	Log(msg);
	return true;
}

protected func Initialize()
{
	// Lattice self-check (Outset FRLatticeFatal pattern -- the cycle-175
	// silent-endgame lesson: every one-shot beat must sit on the lattice).
	if (BC_T_STAMP % 35 != 0) return BCLatticeFatal("BC_T_STAMP");
	if (BC_T_FORECAST % 35 != 0) return BCLatticeFatal("BC_T_FORECAST");
	if (BC_T_FOG_ANN % 35 != 0) return BCLatticeFatal("BC_T_FOG_ANN");
	if (BC_T_FOG % 35 != 0) return BCLatticeFatal("BC_T_FOG");
	if (BC_FOG_LEN % 35 != 0) return BCLatticeFatal("BC_FOG_LEN");
	if (BC_T_EVAL % 35 != 0) return BCLatticeFatal("BC_T_EVAL");
	if (BC_T_EVAL != BC_T_FOG + BC_FOG_LEN)
		return BCLatticeFatal("BC_T_EVAL == BC_T_FOG + BC_FOG_LEN");

	g_bc_c1 = 0;

	// The deal: one goal card, one deal print (display path: Activate ->
	// MessageWindow(GetDesc()), Goal.c4d:156-160).
	var pGoal = CreateObject(GBCN, 0, 0, NO_OWNER);
	if (!pGoal) FatalError("Breaker Cove: goal card spawn failed");
	BCAnnounce("CURRENT GOAL -- Relight the lighthouse -- the fog is coming, and ships sail by the beam. -- fog at 2:00 -- counter: the beacon still lit when the fog lifts");

	AddEffect("BCDirector", 0, 1, 35, 0, 0);
	return true;
}

global func FxBCDirectorTimer(target, effect, time)
{
	var t = FrameCounter();

	// Kindling check: EVERY director tick while the tower is dark (the
	// relight can fire on any tick, not only on beats).
	BCKindleCheck();

	// Stamps near the first crew: the dark tower, the dock, the rule.
	if (t == BC_T_STAMP && !g_bc_lhgt)
		BCStampCove();

	if (t == BC_T_FORECAST)
		BCAnnounce("The fog is coming -- light the tower before the ships lose the channel.");

	if (t == BC_T_FOG_ANN)
		BCAnnounce("Fog at 1:00 -- relight the lighthouse before the channel goes dark.");

	if (t == BC_T_FOG)
	{
		LaunchWeatherEvent(FOGG, 50, BC_FOG_LEN);
		BCAnnounce("The fog rolls in off the sea -- hold the beacon until it lifts.");
		Log("FRMT:fog_begins");
	}

	if (t == BC_T_EVAL)
	{
		StopWeatherEvent();   // belt-and-braces (Outset storm idiom)
		BCEvalC1();
		BCFinish();
	}
	return 1;
}

// Kindling (the relight rule): while the stamped tower is dark, any CLNK
// carrying WOOD within BC_RELIGHT_RADIUS of the tower burns one WOOD and
// lights the beacon. Scenario-owned; Lighthouse.c4d untouched.
global func BCKindleCheck()
{
	var pTower = g_bc_lhgt;
	if (!pTower || LocalN("bLit", pTower)) return false;
	var pClonk, pWood;
	for (pClonk in FindObjects(Find_ID(CLNK)))
	{
		if (Distance(GetX(pClonk), GetY(pClonk), GetX(pTower), GetY(pTower)) > BC_RELIGHT_RADIUS)
			continue;
		pWood = FindContents(WOOD, pClonk);
		if (!pWood) continue;
		RemoveObject(pWood);
		LocalN("bLit", pTower) = 1;
		BCAnnounce("The kindling catches -- the beacon burns.");
		Log("FRMT:c1_relit=1");
		return true;
	}
	return false;
}

// The stamps (Outset granary pattern: crew-relative, surface-snapped, no-
// crew fallback). The tower flips dark right after stamping; the dock
// lands on the shoreline nearest the crew; HMST is the dormant port-duty
// rule (no trade this cycle).
global func BCStampCove()
{
	var p, i, c;
	for (i = 0; i < GetPlayerCount() && !p; i++)
		for (c = 0; c < GetCrewCount(GetPlayerByIndex(i)) && !p; c++)
			p = GetCrew(GetPlayerByIndex(i), c);
	var cx = LandscapeWidth() / 2;
	if (p) cx = GetX(p);
	if (cx < 40) cx = 40;
	if (cx > LandscapeWidth() - 40) cx = LandscapeWidth() - 40;

	// The tower: LHGT (40 px art), base on the surface, starts dark. If
	// the +60 column is sea, fall back to the crew's column (dry land).
	var tx = cx + 60;
	if (tx > LandscapeWidth() - 40) tx = LandscapeWidth() - 40;
	if (BCSurfaceY(tx) > BCWaterY()) tx = cx;
	var ty = BCSurfaceY(tx) - 20;
	g_bc_lhgt = CreateObject(LHGT, tx, ty, NO_OWNER);
	if (!g_bc_lhgt) FatalError("Breaker Cove: lighthouse stamp failed");
	LocalN("bLit", g_bc_lhgt) = 0;  // dark start: the card's tower
	Log("FRMT:c1_darkstart=1");

	// The dock (DKST, 20 px art) on the shoreline nearest the crew.
	var dx = BCShoreDockX(cx);
	if (dx < 0) dx = cx - 60;
	if (dx < 40) dx = 40;
	var dy = BCSurfaceY(dx) - 10;
	CreateObject(DKST, dx, dy, NO_OWNER);

	// The port-duty rule (dormant this cycle: no trade, tallies nothing).
	CreateObject(HMST, 0, 0, NO_OWNER);
	Log(Format("FRMT:cove_stamps tower=%d,%d dock=%d,%d", tx, ty, dx, dy));
	return true;
}

// The shoreline column nearest x0: scan outward (east first, then west)
// for the first column whose ground surface drops below the waterline;
// return the last dry column before the water (-1 if none in range).
global func BCShoreDockX(int x0)
{
	var wy = BCWaterY();
	var wdt = LandscapeWidth();
	var d, sgn, x;
	for (d = 1; d < wdt; d++)
	{
		for (sgn = 1; sgn >= -1; sgn -= 2)
		{
			x = x0 + sgn * d;
			if (x < 20 || x >= wdt - 20) continue;
			if (BCSurfaceY(x) > wy) return x - sgn;
		}
	}
	return -1;
}

// C1 verdict (design 2 predicate): a lit LHGT exists at fog end.
global func BCEvalC1()
{
	var lit = 0;
	var pB;
	for (pB in FindObjects(Find_ID(LHGT), Find_Func("IsLit")))
		lit = 1;
	g_bc_c1 = -1;
	if (lit) g_bc_c1 = 1;
	var v = "lost";
	if (lit) v = "won";
	Log(Format("FRMT:c1_resolved=%s", v));
	if (g_bc_c1 == 1)
		BCAnnounce("The fog lifts -- the beacon held the channel.");
	else
		BCAnnounce("The fog lifts -- the channel went dark.");
	return true;
}

// Rollup: one card -> one outcome line, then GameOver (FRFinish mirror).
global func BCFinish()
{
	if (g_bc_c1 == 1) Log("FRMT:outcome=WIN");
	else Log("FRMT:outcome=LOSE");
	GameOver();
	return true;
}
