/*-- Sturmfront.c4s -- rain > storm > flood river-valley homestead. -----
  Cycle 175, roadmap sturmfront-prototype (prototype-slot fire).
  Spec: .opencode/specs/2026-09-26-sturmfront-prototype.md.
  Probe evidence: .opencode/scratch/175/probe/EVIDENCE.md -- every water
  primitive below is banked: bottom-up InsertMaterial stacks (run6),
  painted water flows through connected carves in <35 ticks (run5),
  floor punctures leak the world dry (run5 -- never touch rows 58-59),
  CastPXS water never integrates (run4 -- hence no CastPXS anywhere).

  Fronts (FrameCounter ticks, 35 fps; ~16 min full run):
     70        forecast
     4200-6300    front 1 rain    -- scripted InsertMaterial drizzle
     6300-21000   calm 1          -- sow / finish mill / trench / sill
     21000-23100  front 2 storm   -- LaunchWeatherEvent(STRM,60,2100)
     23100-31500  calm 2          -- bank crunch (knife-edge cycle 3)
     31500-31800  front 3 rise    -- scripted canyon row painting
     31800-32100  recession       -- ExtractLiquid(x,y) surface sweeps
     31800/32600  claim sweeps    -- submerged field assets are swept
     33600        EvaluateOutcome -- WIN/LOSE by granary+homestead+crew

  Model-assisted driver actions (spec "Crew driver" -- commands are
  unreliable headless, R7; every assist is listed here honestly):
    - trench: staged DigFreeRect at the digger's site (measured dig rate
      = one increment per 35-tick step);
    - sill: staged InsertMaterial(Material("Earth")) pours -- the LOAM
      item builds Earth bridges (Loam.c4d BridgeMaterial()); no "Loam"
      material exists, so Earth IS the loam wall;
    - sowing/harvest: the clonk walks there (real MoveTo), then the
      driver performs Plant()/Harvest() natively at the clonk's feet.
  Everything else is real engine commands (MoveTo/Acquire/Put/Build/
  Call -- Windmill.ProductionOrder and Workshop StartProduction shapes;
  mill grinding reuses Windmill.ProductionOrder verbatim).

  Instrumentation: SFMT:<key>=<value> log lines, mirrored by
  SturmfrontSmoke.c4s (FirstLight/FirstLightSmoke mirror discipline).
  SF_AUTOPLAY drives the crew (promotion strips it). Variant switches
  SF_SKIP_* / SF_RIDGE_ONLY are flipped one-at-a-time on scratch copies
  by .opencode/scratch/175/sturmfront_variants.py (never in this file). --*/

#strict 2

// ---------------- pacing (FrameCounter ticks; absolute -- D7) ----------
static const SF_T_FORECAST     = 70;
static const SF_T_RAIN_START   = 4200;
static const SF_T_RAIN_END     = 6300;
static const SF_T_STORM_START  = 21000;
static const SF_STORM_INTENSITY = 60;   // R3: drop to 40 / shorten to 1400 if
static const SF_STORM_LENGTH   = 2100;  //        the mill burns >1 of 3 seeds
static const SF_T_FLOOD_START  = 31500;
static const SF_FLOOD_RISE     = 300;
static const SF_FLOOD_RECEDE   = 300;
static const SF_T_CLAIM1       = 31800; // flood peak
static const SF_T_CLAIM2       = 32600; // post-recession
static const SF_T_EVAL         = 33600;

// ---------------- economy ------------------------------------------------
static const SF_GRANARY_QUOTA  = 12;    // calibration 10-16 (playtest task)
static const SF_KIT_SEEDS      = 20;    // 3 field cycles need 18 (D3)
static const SF_KIT_WOOD       = 16;
static const SF_KIT_METAL      = 2;

// ---------------- ROI / geometry (world px; map 100x60 @ zoom 10) --------
static const SF_FIELD_W1 = 60;     // west field flank
static const SF_FIELD_W2 = 230;
static const SF_FIELD_E1 = 430;    // east field flank
static const SF_FIELD_E2 = 640;
static const SF_FIELD_Y1 = 420;
static const SF_FIELD_Y2 = 545;
static const SF_CH_X1 = 240;       // canyon slot
static const SF_CH_X2 = 430;
static const SF_CH_Y1 = 420;
static const SF_CH_Y2 = 600;
static const SF_RIDGE_X1 = 700;    // terrace shelf
static const SF_RIDGE_X2 = 890;
static const SF_RIVER_TOP = 540;   // authored standing-river surface
static const SF_RIM_Y = 440;       // flank surface == spill lip
static const SF_FLOOD_TOP = 435;   // paint up to here (spill head)
static const SF_WRKS_X = 720;      // terrace furniture
static const SF_MILL_X = 790;
static const SF_RIDGE_PLOT1 = 740; // the pre-sown family plot
static const SF_RIDGE_PLOT2 = 755;

// ---------------- rain / flood rates --------------------------------------
static const SF_RAIN_RATE = 8;     // cells/tick drizzle
static const SF_FLOOD_RATE = 67;   // cells/tick (191*105 cells / 300 ticks)

// ---------------- variant switches (playtest flips scratch copies) -------
static const SF_SKIP_TRENCH = 0;
static const SF_SKIP_MILL   = 0;
static const SF_SKIP_WOOD   = 0;
static const SF_SKIP_WALL   = 0;
static const SF_RIDGE_ONLY  = 0;
static const SF_AUTOPLAY    = 1;

static const SF_SILL_CELLS = 464;  // 16 cols x 29 rows east-rim levee

// ---------------- runtime state ------------------------------------------
static g_FieldPlots;      // [100,140,180,470,510,550] (arrays are runtime)
static g_plant_cursor;
static g_harvest_fp;      // wheat plants harvested in the field zones
static g_harvest_ridge;   // ... on the terrace
static g_flood_claimed;   // units swept by the claim rule
static g_mill_grinds;     // FLOU milled (mill-watch delta)
static g_flou_prev;
static g_mill_done;
static g_mill_frame;
static g_t_first_task;
static g_idle_steps;      // calm-window steps with an empty stack
static g_calm_steps;
static g_flood_cursor;    // rise painter cursor
static g_recede_level;    // recession surface row
static g_recede_x;
static g_rain_cursor;
static g_trench_step;     // carve increments done (0..10)
static g_sill_cells;
static g_done;            // evaluation fired

protected func Initialize()
{
	SetWind(20);
	g_FieldPlots = [100, 140, 180, 470, 510, 550];
	g_plant_cursor = 0; g_harvest_fp = 0; g_harvest_ridge = 0;
	g_flood_claimed = 0; g_mill_grinds = 0; g_flou_prev = 0;
	g_mill_done = 0; g_mill_frame = 0; g_t_first_task = 0;
	g_idle_steps = 0; g_calm_steps = 0; g_flood_cursor = 0;
	g_recede_level = 0; g_recede_x = 0; g_rain_cursor = 0;
	g_trench_step = 0; g_sill_cells = 0; g_done = 0;

	// terrace surface (authored shelf ~220; scan is authoritative)
	var gy = 100;
	while (gy < 260 && !GBackSolid(SF_WRKS_X, gy)) gy++;

	// the workbench + physical kit (FirstLight pattern: no HomeBaseMaterial)
	var pBase = CreateObject(WRKS, SF_WRKS_X, gy, NO_OWNER);
	if (pBase)
	{
		CreateContents(AGWS, pBase, SF_KIT_SEEDS);
		CreateContents(AGSK, pBase, 2);
		CreateContents(WOOD, pBase, SF_KIT_WOOD);
		CreateContents(METL, pBase, SF_KIT_METAL);
		CreateContents(LOAM, pBase, 4);
		CreateContents(CNKT, pBase, 2);
	}

	// the old windmill: a 40% site on the terrace (Stormwatch ruin pattern)
	CreateConstruction(AGWM, SF_MILL_X, gy, NO_OWNER, 40, 1);

	// the family plot: 2 pre-sown ridge seedlings (EventSmoke re-set pattern)
	var j, pW;
	for (j = 0; j < 2; j++)
	{
		pW = CreateObject(AGWH, SF_RIDGE_PLOT1 + j * 15, gy, NO_OWNER);
		if (pW) pW->SetAction("Seedling");
	}

	Log("Sturmfront: crew dropped -- forecast at first light");
	AddEffect("FrontDirector", 0, 1, 35, 0, 0);
	if (SF_AUTOPLAY) AddEffect("CrewDriver", 0, 1, 35, 0, 0);
	return true;
}

func InitializePlayer(int iPlr)
{
	// knowledge per joining player (Homestead pattern; players join after
	// Initialize, so grants cannot live there)
	SetPlrKnowledge(iPlr, AGWS);
	SetPlrKnowledge(iPlr, AGSK);
	SetPlrKnowledge(iPlr, AGWM);
	return true;
}

// ---------------- announcements -------------------------------------------

global func SFAnnounce(string msg)
{
	// NOTE: no DECO id (that is a Tutorial.c4f def, absent from this def
	// set -- D5). 10-arg CustomMessage is the shipped Stormwatch shape.
	var i;
	for (i = 0; i < GetPlayerCount(); i++)
		CustomMessage(msg, 0, GetPlayerByIndex(i), 0, 0, 0xffffff, 0, 0, MSG_Bottom, 300);
	return true;
}

// ---------------- front director ------------------------------------------

global func FxFrontDirectorTimer(target, effect, time)
{
	var t = FrameCounter();

	if (t == SF_T_FORECAST)
	{
		SFAnnounce("Rain 2:00 - Storm 10:00 - Flood after the storm");
		Log("Sturmfront: forecast -- rain, then storm, then flood");
	}

	if (t >= SF_T_RAIN_START && t < SF_T_RAIN_END) SFRainTick();
	if (t == SF_T_RAIN_END)
	{
		Log("Sturmfront: rain ends");
		SFDumpCounters("calm1-start");
	}

	if (t == SF_T_STORM_START)
	{
		LaunchWeatherEvent(STRM, SF_STORM_INTENSITY, SF_STORM_LENGTH);
		SFAnnounce("Hold fast -- the storm is upon us!");
		Log("Sturmfront: storm begins");
	}
	if (t == SF_T_STORM_START + SF_STORM_LENGTH)
	{
		StopWeatherEvent();
		SFAnnounce("The storm passes. Bank what you can!");
		Log("Sturmfront: storm ends");
		SFDumpCounters("calm2-start");
	}

	if (t >= SF_T_FLOOD_START && t < SF_T_FLOOD_START + SF_FLOOD_RISE)
		SFFloodRiseTick();
	if (t == SF_T_FLOOD_START + SF_FLOOD_RISE)
	{
		g_recede_level = SF_FLOOD_TOP;   // recession sweeps from the spill head
		g_recede_x = 0;
		Log("Sturmfront: flood peaks");
		SFDumpCounters("flood-peak");
	}
	if (t > SF_T_FLOOD_START + SF_FLOOD_RISE && t < SF_T_FLOOD_START + SF_FLOOD_RISE + SF_FLOOD_RECEDE)
		SFFloodRecedeTick();
	if (t == SF_T_FLOOD_START + SF_FLOOD_RISE + SF_FLOOD_RECEDE)
	{
		Log("Sturmfront: flood recedes -- drain and salvage");
		SFDumpCounters("drain-start");
	}

	if (t == SF_T_CLAIM1 || t == SF_T_CLAIM2) SFClaimSweep();
	if (t == SF_T_EVAL && !g_done) EvaluateOutcome();

	return 1;
}

// front 1: deterministic drizzle (no RNG: identical across seeds; the
// seeds sample the storm's lightning, which is the stochastic stressor)
global func SFRainTick()
{
	var w = Material("Water");
	var i, x;
	for (i = 0; i < SF_RAIN_RATE; i++)
	{
		x = 60 + (g_rain_cursor * 7) % 580;   // sweeps the whole field span
		++g_rain_cursor;
		if (x > SF_FIELD_W2 && x < SF_FIELD_E1) continue;   // canyon gap
		if (!GBackLiquid(x, SF_RIM_Y - 2) && !GBackSolid(x, SF_RIM_Y - 2))
			InsertMaterial(w, x, SF_RIM_Y - 2);
	}
	return true;
}

// front 3 rise: bottom-up row painting inside the canyon slot. Painting
// on top of standing water stacks (probe-run6); above the rim (440) the
// column has no side walls, so the water spills onto the flanks natively.
global func SFFloodRiseTick()
{
	var w = Material("Water");
	var i, x, y, row_w = SF_CH_X2 - SF_CH_X1 + 1;
	for (i = 0; i < SF_FLOOD_RATE; i++)
	{
		y = SF_RIVER_TOP - 1 - g_flood_cursor / row_w;
		if (y < SF_FLOOD_TOP) return true;   // rise complete
		x = SF_CH_X1 + g_flood_cursor % row_w;
		++g_flood_cursor;
		InsertMaterial(w, x, y);
	}
	return true;
}

// front 3 recession: per-cell ExtractLiquid(x, y) surface sweeps, walking
// the level back down to the authored river surface (the river itself
// stays). Field leftovers drain over the rim / through the trench and
// are picked up on later sweeps -- that asymmetry IS the trench's story.
global func SFFloodRecedeTick()
{
	var i, x, row_w = SF_CH_X2 - SF_CH_X1 + 1;
	for (i = 0; i < SF_FLOOD_RATE && g_recede_level < SF_RIVER_TOP; i++)
	{
		x = SF_CH_X1 + g_recede_x;
		++g_recede_x;
		if (g_recede_x >= row_w) { g_recede_x = 0; ++g_recede_level; }
		if (GBackLiquid(x, g_recede_level))
			ExtractLiquid(x, g_recede_level);
	}
	return true;
}

// ---------------- censuses + counters -------------------------------------

global func SFLiquidCount(int x1, int y1, int x2, int y2)
{
	var n = 0, x, y;
	for (y = y1; y < y2; y++)
		for (x = x1; x < x2; x += 2)
			if (GBackLiquid(x, y)) ++n;
	return n;
}

global func FieldWaterPx()
{
	return SFLiquidCount(SF_FIELD_W1, SF_FIELD_Y1, SF_FIELD_W2, SF_FIELD_Y2)
	     + SFLiquidCount(SF_FIELD_E1, SF_FIELD_Y1, SF_FIELD_E2, SF_FIELD_Y2);
}

global func ChannelWaterPx()
{
	return SFLiquidCount(SF_CH_X1, SF_CH_Y1, SF_CH_X2, SF_CH_Y2);
}

global func SFDumpCounters(string when)
{
	Log(Format("Sturmfront: counters at %s", when));
	Log(Format("SFMT:field_water_px=%d", FieldWaterPx()));
	Log(Format("SFMT:channel_water_px=%d", ChannelWaterPx()));
	Log(Format("SFMT:harvest_fp=%d", g_harvest_fp));
	Log(Format("SFMT:harvest_ridge=%d", g_harvest_ridge));
	Log(Format("SFMT:granary_units=%d", GranaryUnits()));
	Log(Format("SFMT:mill_grinds=%d", g_mill_grinds));
	Log(Format("SFMT:flood_claimed=%d", g_flood_claimed));
	// idle_pct only for windows that actually accumulated calm steps --
	// a vacuous dump (e.g. at eval, outside any calm window) must not
	// become the last SFMT:idle_pct the parser reads
	if (g_calm_steps > 0) Log(Format("SFMT:idle_pct=%d", SFIdlePct()));
	Log(Format("SFMT:t_first_task=%d", g_t_first_task));
	// idle accumulators are per calm window
	g_idle_steps = 0;
	g_calm_steps = 0;
	return true;
}

global func SFIdlePct()
{
	if (g_calm_steps <= 0) return 100;
	return g_idle_steps * 100 / g_calm_steps;
}

// ---------------- claim rule (rule 1) --------------------------------------

global func SFClaimSweep()
{
	var n = 0, rect, x1, x2, pW, pS;
	for (rect = 0; rect < 2; rect++)
	{
		if (rect == 0) { x1 = SF_FIELD_W1; x2 = SF_FIELD_W2; }
		else { x1 = SF_FIELD_E1; x2 = SF_FIELD_E2; }
		for (var pW in FindObjects(Find_ID(AGWH), Find_InRect(x1, SF_FIELD_Y1, x2 - x1, SF_FIELD_Y2 - SF_FIELD_Y1)))
			if (pW->~IsRipe() && GBackLiquid(GetX(pW), GetY(pW)))
			{
				RemoveObject(pW);
				++n;
			}
		for (var pS in FindObjects(Find_ID(AGSH), Find_InRect(x1, SF_FIELD_Y1, x2 - x1, SF_FIELD_Y2 - SF_FIELD_Y1)))
			if (!pS->Contained() && GBackLiquid(GetX(pS), GetY(pS)))
			{
				RemoveObject(pS);
				++n;
			}
	}
	g_flood_claimed += n;
	Log(Format("SFMT:flood_claimed=%d", g_flood_claimed));
	if (n == 0) Log("Sturmfront: claim sweep -- nothing submerged");
	return true;
}

// ---------------- win / lose -------------------------------------------------

// pure decision core (unit-tested by the smoke mirror, branch for branch)
global func SFEvaluate(int granary, int quota, bool intact, int crew)
{
	if (granary < quota) return "granary";
	if (!intact) return "structures";
	if (crew < 1) return "crew";
	return "win";
}

global func GranaryUnits()
{
	var n = ObjectCount(FLOU);   // milled flour anywhere
	var pBase = FindObject(WRKS);
	if (pBase) n += ContentsCount(AGSH, pBase);
	var pMill = FindObject(AGWM);
	if (pMill) n += ContentsCount(AGSH, pMill);
	return n;
}

global func HomesteadIntact()
{
	var pBase = FindObject(WRKS);
	if (!pBase) return false;
	if (GetCon(pBase) < 100) return false;
	if (OnFire(pBase)) return false;
	return true;
}

global func EvaluateOutcome()
{
	g_done = 1;
	var granary = GranaryUnits();
	var crew = 0, i;
	for (i = 0; i < GetPlayerCount(); i++)
		crew += GetCrewCount(GetPlayerByIndex(i));
	Log(Format("SFMT:outcome_granary=%d", granary));
	Log(Format("SFMT:outcome_intact=%d", HomesteadIntact()));
	Log(Format("SFMT:outcome_crew=%d", crew));
	var verdict = SFEvaluate(granary, SF_GRANARY_QUOTA, HomesteadIntact(), crew);
	if (verdict == "win") Log("SFMT:outcome=WIN");   // eq/ne are not operators under #strict 2 (C4AulParse.cpp GetOperator); the smoke mirror must use ==/!= too
	else Log(Format("SFMT:outcome=LOSE|reason=%s", verdict));
	SFDumpCounters("eval");
	GameOver();
	return true;
}

// ---------------- crew driver -------------------------------------------------

global func SFCrewList()
{
	var list = [], i, c, p;
	for (i = 0; i < GetPlayerCount(); i++)
		for (c = 0; c < GetCrewCount(GetPlayerByIndex(i)); c++)
		{
			p = GetCrew(GetPlayerByIndex(i), c);
			if (p) list[GetLength(list)] = p;
		}
	return list;
}

global func FxCrewDriverTimer(target, effect, time)
{
	var t = FrameCounter();

	// mill-watch (FirstLight pattern): FLOU anywhere = milled
	var f = ObjectCount(FLOU);
	if (f > g_flou_prev) g_mill_grinds += f - g_flou_prev;
	g_flou_prev = f;

	// mill completion frame (the wood-role causality observable)
	var pSite = FindObject(AGWM);
	if (!g_mill_done && pSite && GetCon(pSite) >= 100)
	{
		g_mill_done = 1;
		g_mill_frame = t;
		Log(Format("SFMT:mill_complete=%d", t));
	}

	// idle instrumentation: calm windows only (proxy c)
	var calm = (t > SF_T_RAIN_END && t < SF_T_STORM_START)
	        || (t > SF_T_STORM_START + SF_STORM_LENGTH && t < SF_T_FLOOD_START);
	var crew = SFCrewList();
	if (calm && GetLength(crew) > 0)
	{
		++g_calm_steps;
		var c, idleAny = false;
		for (c = 0; c < GetLength(crew); c++)
			if (!GetCommand(crew[c])) { idleAny = true; break; }
		if (idleAny) ++g_idle_steps;
	}

	// keep every crew clonk's command stack non-empty (greedy)
	var i;
	for (i = 0; i < GetLength(crew); i++)
	{
		if (GetCommand(crew[i])) continue;
		SFAssignNext(crew[i], i % 4);
	}
	return 1;
}

// role ladder: 0 trench, 1 mill, 2 wood+metal, 3 sill; farming is the
// shared fallback (and the only job when a role's switch is skipped)
global func SFAssignNext(object clnk, int role)
{
	if (g_t_first_task == 0)
	{
		g_t_first_task = FrameCounter();
		Log(Format("SFMT:t_first_task=%d", g_t_first_task));
	}
	var pBase = FindObject(WRKS);
	var pSite = FindObject(AGWM);

	if (role == 0 && !SF_SKIP_TRENCH && g_trench_step < 10) return SFTrenchWork(clnk);
	if (role == 1 && !SF_SKIP_MILL && pSite) return SFMillWork(clnk);
	if (role == 2 && !SF_SKIP_WOOD && pSite && GetCon(pSite) < 100) return SFWoodWork(clnk, pBase);
	if (role == 3 && !SF_SKIP_WALL && g_sill_cells < SF_SILL_CELLS) return SFWallWork(clnk);
	return SFFarmWork(clnk, pBase);
}

global func SFClkNear(object clnk, int x, int y, int r)
{
	var dx = Abs(GetX(clnk) - x);
	var dy = Abs(GetY(clnk) - y);
	return dx <= r && dy <= r;
}

global func SFSurfaceY(int x)
{
	var y = 100;
	while (y < 590 && !GBackSolid(x, y)) y++;
	return y;
}

global func SFOnRidge(int x)
{
	return x >= SF_RIDGE_X1 && x <= SF_RIDGE_X2;
}

// ---- role 0: the drainage trench (assist: staged DigFreeRect) ----
// L-shape: a vertical shaft at x464-476 from the east-flank surface down
// to y515, then a horizontal adit at y492-504 from the canyon (x428)
// through the flank to the shaft -- fully connected (probe-run5 recipe).
global func SFTrenchWork(object clnk)
{
	var sx = 470, sy = 445;
	if (g_trench_step < 6)
	{
		if (!SFClkNear(clnk, sx, sy, 60))
		{
			AddCommand(clnk, "MoveTo", 0, sx, sy - 10);
			return true;
		}
		DigFreeRect(sx - 6, sy + g_trench_step * 10, 12, 12);
	}
	else
	{
		// the adit is dug from the shaft (the digger cannot stand inside
		// the adit -- the assist works from the shaft mouth, documented)
		if (!SFClkNear(clnk, sx, sy, 80))
		{
			AddCommand(clnk, "MoveTo", 0, sx, sy - 10);
			return true;
		}
		DigFreeRect(428 + (g_trench_step - 6) * 8, 492, 12, 12);
	}
	++g_trench_step;
	Log(Format("SFMT:trench_step=%d", g_trench_step));
	if (g_trench_step == 10) Log("Sturmfront: the trench is through -- the field can drain");
	return true;
}

// ---- role 1: finish the old mill, then grind (real commands) ----
global func SFMillWork(object clnk)
{
	var pSite = FindObject(AGWM);
	if (!pSite) return false;
	if (GetCon(pSite) < 100)
	{
		// help build while the wood role hauls components
		AddCommand(clnk, "Build", pSite, 0, 0, 0, 0, 0, 0, 3);
		return true;
	}
	// full mill: grind via the in-tree command chain, verbatim reuse.
	// With nothing left to grind anywhere, stand down (farm instead) so
	// a spent economy does not pollute idle_pct with Acquire churn.
	if (ObjectCount(AGSH) <= 0) return false;
	pSite->ProductionOrder(clnk);
	return true;
}

// ---- role 2: haul WOOD/METL from the workbench into the site ----
global func SFWoodWork(object clnk, object pBase)
{
	var pSite = FindObject(AGWM);
	if (!pSite || GetCon(pSite) >= 100) return false;
	if (pBase && ContentsCount(WOOD, pBase) > 0 && ContentsCount(WOOD, pSite) < 7)
	{
		AddCommand(clnk, "Acquire", 0, 0, 0, pBase, 0, WOOD, 1, 3);
		AddCommand(clnk, "Put", pSite, 0, 0, 0, 0, WOOD);
		return true;
	}
	if (pBase && ContentsCount(METL, pBase) > 0 && ContentsCount(METL, pSite) < 1)
	{
		AddCommand(clnk, "Acquire", 0, 0, 0, pBase, 0, METL, 1, 3);
		AddCommand(clnk, "Put", pSite, 0, 0, 0, 0, METL);
		return true;
	}
	AddCommand(clnk, "Build", pSite, 0, 0, 0, 0, 0, 0, 3);
	return true;
}

// ---- role 3: raise the east-rim levee (assist: staged Earth pours) ----
// 16 px wide wall standing on the east flank's canyon edge, x430-445,
// growing from y439 up to y411: the flood must top 410 instead of 440
// before it takes the east flank (the west flank stays exposed -- the
// floodplain bet still bites).
global func SFWallWork(object clnk)
{
	if (!SFClkNear(clnk, 438, 445, 50))
	{
		AddCommand(clnk, "MoveTo", 0, 438, 436);
		return true;
	}
	var m = Material("Earth");
	var i, x, y;
	for (i = 0; i < 40 && g_sill_cells < SF_SILL_CELLS; i++)
	{
		x = 430 + g_sill_cells % 16;
		y = 439 - g_sill_cells / 16;
		if (!GBackSolid(x, y)) InsertMaterial(m, x, y);
		++g_sill_cells;
	}
	if (g_sill_cells % 160 < 40) Log(Format("SFMT:sill_cells=%d", g_sill_cells));
	if (g_sill_cells >= SF_SILL_CELLS) Log("Sturmfront: the sill stands -- the east levy is raised");
	return true;
}

// ---- shared: sow / harvest / bank (rule 2: dry feet) ----
global func SFFarmWork(object clnk, object pBase)
{
	// 1) bank carried sheaves (real command)
	if (pBase && ContentsCount(AGSH, clnk) > 0)
	{
		AddCommand(clnk, "Put", pBase, 0, 0, 0, 0, AGSH);
		return true;
	}
	// 2) harvest ripe wheat (walk there, then the native Harvest at feet)
	var pW = SFFindRipeWheat();
	if (pW)
	{
		if (!SFClkNear(clnk, GetX(pW), GetY(pW), 25))
		{
			AddCommand(clnk, "MoveTo", 0, GetX(pW), GetY(pW));
			return true;
		}
		var wx = GetX(pW);
		pW->Harvest(clnk);
		if (SFOnRidge(wx)) ++g_harvest_ridge;
		else ++g_harvest_fp;
		Log(Format("SFMT:harvest_fp=%d", g_harvest_fp));
		Log(Format("SFMT:harvest_ridge=%d", g_harvest_ridge));
		return true;
	}
	// 3) sow a dry plot (dry-feet rule: the surface cell must not be liquid)
	var px = SFNextDryPlot();
	if (px > 0 && pBase && ContentsCount(AGWS, pBase) > 0)
	{
		var sy = SFSurfaceY(px);
		if (!SFClkNear(clnk, px, sy, 25))
		{
			AddCommand(clnk, "MoveTo", 0, px, sy - 12);
			return true;
		}
		var pSeed = pBase->FindContents(AGWS);
		if (pSeed) RemoveObject(pSeed);
		var pNew = CreateObject(AGWH, px, sy, NO_OWNER);
		if (pNew) pNew->SetAction("Seedling");
		return true;
	}
	// 4) fallback: stand by the workbench (counts as idle if the stack
	//    empties -- surfaced honestly by idle_pct)
	if (pBase) AddCommand(clnk, "MoveTo", 0, GetX(pBase) - 30, GetY(pBase));
	return true;
}

global func SFFindRipeWheat()
{
	for (var pW in FindObjects(Find_ID(AGWH)))
		if (pW->~IsRipe()) return pW;
	return 0;
}

global func SFNextDryPlot()
{
	var n_field = GetLength(g_FieldPlots);
	var i, idx, px, sy;
	for (i = 0; i < n_field + 2; i++)
	{
		idx = (g_plant_cursor + i) % (n_field + 2);
		if (SF_RIDGE_ONLY) px = SF_RIDGE_PLOT1 + (idx % 2) * 15;
		else if (idx < n_field) px = g_FieldPlots[idx];
		else px = SF_RIDGE_PLOT1 + (idx - n_field) * 15;
		sy = SFSurfaceY(px);
		if (GBackLiquid(px, sy - 1)) continue;   // dry-feet rule (rule 2)
		var occupied = FindObjects(Find_ID(AGWH), Find_InRect(px - 8, sy - 22, 16, 24));
		if (GetLength(occupied) > 0) continue;
		g_plant_cursor = idx + 1;
		return px;
	}
	return 0;
}
