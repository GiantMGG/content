/*-- Sturmfront.c4s -- rain > storm > flood river-valley homestead. -----
  Cycle 175, roadmap sturmfront-prototype (prototype-slot fire).
  Spec: .opencode/specs/2026-09-26-sturmfront-prototype.md.
  Probe evidence: .opencode/scratch/175/probe/EVIDENCE.md -- every water
  primitive below is banked: bottom-up InsertMaterial stacks (run6),
  painted water flows through connected carves in <35 ticks (run5),
  floor punctures leak the world dry (run5 -- never touch rows 58-59),
  CastPXS water never integrates (run4 -- hence no CastPXS anywhere).

  Fronts (FrameCounter ticks, 35 fps; ~16 min full run). Every beat is a
  multiple of 35 -- the 35-tick FrontDirector effect can only ever sample
  those (cycle-175 A: the flood endgame once sat on non-lattice beats and
  silently never ran; Initialize enforces the lattice with a FatalError):
     70        forecast
     4200-6300    front 1 rain    -- scripted InsertMaterial drizzle
     6300-21000   calm 1          -- sow / finish mill / trench / berm
     21000-22400  front 2 storm   -- LaunchWeatherEvent(STRM,40,1400) (R3)
     22400-31500  calm 2          -- bank crunch (knife-edge cycle 3)
     31500-31850  front 3 rise    -- scripted canyon row painting (10 x 35)
     31850        flood peaks     -- claim sweep 1 (spill head reached)
     31850-32165  recession       -- ExtractLiquid(x,y) surface sweeps
     32480        claim sweep 2   -- 315 frames post-drain-start
     33600        EvaluateOutcome -- WIN/LOSE by granary+homestead+crew

  Model-assisted driver actions (spec "Crew driver" -- commands are
  unreliable headless, R7; every assist is listed here honestly):
    - trench: staged DigFreeRect at the digger's column, dug while the
      clonk STANDS BESIDE the shaft (stand x = dig column + 24 -- never
      over the mouth; fall-ins were a shipped death cause, B3);
    - berm: staged InsertMaterial(Material("Earth")) pours -- the LOAM
      item builds Earth bridges (Loam.c4d BridgeMaterial()); no "Loam"
      material exists, so Earth IS the loam wall. The berm seals the
      WHOLE east rim x430-649 (rows 419-439): the east flank stays dry,
      the west flank is the exposed floodplain (that is the bet);
    - transit: cross-zone Movement is assisted by SetPosition (the 220-px
      terrace face and the open canyon kill headless clonks; interactive
      players bridge with the kit's CNKT/LOAM -- kept for them, the
      driver transits instead). Counted via SFMT:transit;
    - assist gating (critic pass): EVERY teleport assist -- the zone
      transit, the storm-haven transport, and the t=35 spawn-marshal
      pull -- is SF_AUTOPLAY-gated, so a human (auto-play off) never
      sees the crew teleport; the one deliberate exception is the
      SFEvacWatch mill-completion evacuation, which stays UNGATED: it
      guards against the engine's completion-snap kill (a defect, not
      an assist) and a nudge beats a death there;
    - storm: the driver HUNKERS -- while SFStormActive() no jobs are
      assigned; at the storm's first tick every crew member transports
      to one of four scattered SAFE havens (SFSafeHaven: terrace east,
      peak crown, west crag, berm midspan -- >= 80 px from the wooden
      WRKS/mill, >= 40 px apart) and their command stacks are cleared,
      so nobody is mid-transit outdoors in lightning nor clustered next
      to a burning structure ("hold fast" = take shelter; seeds 176/177
      lost all four clonks to strikes at ~frame 21100 and to the mill/
      WRKS fire right after storm end -- fixprobe-176/177.log,
      baseline-177.log). Work is a calm-window activity, matching the
      announce and the spec's calm-window schedule; the hunkered clonks
      are never counted in the idle census (the calm expression excludes
      the storm window);
    - claim bite: the flood's economic bite is measured SMALL
      (flood_claimed <= 1 max across ALL 9 POST-fix runs -- baselines
      1/0/1, post-fix parse-table; the pre-fix var-wall draw of 2 is
      superseded) -- crops that would straddle the flood would need
      sowing in [27960, 31500], which the greedy driver's early-sowing
      cycle never does; the floodplain-vs-ridge dilemma resolves to
      harvest-before-the-flood. Recorded as a cycle-175 finding, not
      engineered around (a late-sow reserve wave was tried and removed --
      it never fired once in 3 seeds, and its hold-back only starved the
      granary while serving nothing);
    - sowing: RemoveObject(seed) + CreateObject(AGWH) +
      SetAction("Seedling") -- NOT Plant(): the WheatSeed Earth/Tunnel
      soil check is bypassed (EventSmoke re-set pattern, deterministic
      headless);
    - sheaf bank / mill feed: a carried sheaf is moved with
      RemoveObject + CreateContents into the WRKS or (completed mill,
      up to its 5-cap) into the mill hopper -- no container-Put works
      headless (haulprobe1/4-6: the Acquire section search cannot reach
      container contents and the mill has no collection/entrance);
    - mill grind: Call ProductionStart -- the in-tree
      ProductionOrder's Put/Acquire prefix stalls in this engine; the
      grind itself (consume hopper sheaf -> Grinding action -> FLOU
      after grind_time) is the real Windmill script (Windmill.c4d/
      Script.c:60-88, Starts only at con==100 -- incomplete-phase
      actions are forced to Idle);
    - harvest: native pW->Harvest(clnk) at the clonk's feet (Wheat.Harvest
      collects both sheaves into the clonk, so banking works);
    - rain / flood rise / recession / claim = scripted sweeps (above).
  Everything else is real engine commands (MoveTo/Acquire/Put/Build/Call
  -- Workshop StartProduction shapes).

  Instrumentation: SFMT:<key>=<value> log lines, mirrored by
  SturmfrontSmoke.c4s (FirstLight/FirstLightSmoke mirror discipline).
  SF_AUTOPLAY drives the crew (promotion strips it). Variant switches
  SF_SKIP_* / SF_RIDGE_ONLY are flipped one-at-a-time on scratch copies
  by .opencode/scratch/175/sturmfront_variants.py (never in this file). --*/

#strict 2

// ---------------- pacing (FrameCounter ticks; absolute -- D7) ----------
static const SF_T_FORECAST     = 70;
static const SF_T_RAIN_START   = 4200;   // 120 x 35
static const SF_T_RAIN_END     = 6300;   // 180 x 35
static const SF_T_STORM_START  = 21000;  // 600 x 35
static const SF_STORM_INTENSITY = 40;   // R3 mitigation applied 2026-09-27:
static const SF_STORM_LENGTH   = 1400;  //        intensity 60->40 + length 2100->1400
                                        //        after storm fire ruined 2 of 3
                                        //        robustness seeds (mill-convergence
                                        //        blast, seed 176, frame ~21100;
                                        //        WRKS burn, seed 177) -- evidence
                                        //        fixprobe-176/177.log
static const SF_T_FLOOD_START  = 31500; // 900 x 35
static const SF_FLOOD_RISE     = 350;   // 10 x 35 director calls to peak
static const SF_FLOOD_RECEDE   = 315;   // 9 x 35 calls back to the river bed
static const SF_T_CLAIM1       = 31850; // 910 x 35 == flood peak
static const SF_T_CLAIM2       = 32480; // 928 x 35, 315 after drain-start
static const SF_T_EVAL         = 33600; // 960 x 35

// ---------------- economy ------------------------------------------------
static const SF_GRANARY_QUOTA  = 10;    // CONFIRMED 2026-09-28 from the POST-NR2 matrix
                                        // (parse-table.md, after the re-review grind-leak +
                                        // evacuation fixes landed): baselines granary
                                        // 14/12/16 -- all WIN at quota 10 (margins 2-6),
                                        // ridge-only 4, mill-active 11-14, no-mill control
                                        // 43. NR2 is closed: grinds are 1:1 (mill_grinds ==
                                        // FLOU, diag evidence) -- but ~2/3 of every harvest
                                        // lies LOOSE at the plots, never banked: 1-slot
                                        // clonks (Clonk.c4d MaxContentsCount()==1) make
                                        // Wheat.Harvest's SECOND Collect overflow (one sheaf
                                        // carried, one falls loose per plant -- slotprobe.log,
                                        // the 1-slot harvest mechanism), and there is no
                                        // loose-pickup assist (mill-active clonks walk away
                                        // before recovering them). diag-gap-175.log closes the
                                        // arithmetic exactly: 36 units = 11 FLOU + 13 loose
                                        // on the ground + 1 flood-claimed + 11 fire-lost --
                                        // the units EXIST as loose ground objects, none burn
                                        // (the Flash reviewer's stacked-Call burn theory is
                                        // refuted by these probes). Mill-active granary thus
                                        // UNDERCOUNTS the true harvest; 10 remains the best
                                        // separator regardless (ridge-only margin 6; 176's
                                        // fire-loss sits below the line).
static const SF_KIT_SEEDS      = 20;    // 3 field cycles need 18 (D3)
static const SF_KIT_WOOD       = 16;
static const SF_KIT_METAL      = 2;

// ---------------- ROI / geometry (world px; map 100x60 @ zoom 10) --------
static const SF_FIELD_W1 = 60;     // west field flank (the floodplain)
static const SF_FIELD_W2 = 230;
static const SF_FIELD_E1 = 430;    // east field flank (behind the berm)
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
static const SF_FLOOD_TOP = 420;   // paint up to here (spill head): the west
                                   // flank sheet is rows 420-439 = 20 px deep
static const SF_WRKS_X = 720;      // terrace furniture
static const SF_MILL_X = 790;
static const SF_MILL_STAGE = 740;  // mill work staging/transit x: >= 40 px away
                                   // from the mill so the completed shape's
                                   // ContactIncinerate footprint (x 776..804)
                                   // cannot reach a queued worker even on the
                                   // off-vigil frame (re-review B2: the old
                                   // x760 staging was 30 px out -- outside the
                                   // old radius-20 guard)
static const SF_RIDGE_PLOT1 = 740; // the pre-sown family plot
static const SF_RIDGE_PLOT2 = 755;

// ---------------- rain / flood rates --------------------------------------
static const SF_RAIN_RATE = 280;    // 8 cells/game-tick x 35 = 280 per call;
                                    // 60 calls over the 2100-frame rain
                                    // deliver ~16,800 cells (B2: the old
                                    // per-tick constant delivered 1/35 of
                                    // the designed volume)
static const SF_FLOOD_RATE = 2345;  // 67 cells/game-tick x 35 per call;
                                    // 10 rise calls paint 23,450 cells >=
                                    // the 22920-cell canyon fill (191 x 120)

// ---------------- variant switches (playtest flips scratch copies) -------
static const SF_SKIP_TRENCH = 0;
static const SF_SKIP_MILL   = 0;
static const SF_SKIP_WOOD   = 0;
static const SF_SKIP_WALL   = 0;
static const SF_RIDGE_ONLY  = 0;
static const SF_AUTOPLAY    = 1;

static const SF_BERM_CELLS = 4620;  // 220 cols (x430-649) x 21 rows
                                    // (y439 up to y419) east-rim full-span
                                    // berm: every rim px is sealed; the
                                    // west rim stays open (the bet)

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
static g_sill_cells;      // berm pour counter (SFMT legacy name kept)
static g_done;            // evaluation fired
static g_peak_done;       // once-flags: belt-and-suspenders on the >= gates
static g_drain_done;      //   a future constant edit cannot silently kill a
static g_claim1_done;     //   one-shot beat again (review B1)
static g_claim2_done;

protected func Initialize()
{
	// A.3 LATTICE SELF-CHECK: the FrontDirector samples FrameCounter()
	// only on multiples of 35, so every one-shot beat must land on the
	// lattice or it silently never fires (the cycle-175 B1 endgame bug).
	// This is a top-level FatalError -> the run dies LOUDLY at boot.
	if (SF_T_FORECAST % 35 != 0) return SFLatticeFatal("SF_T_FORECAST");
	if (SF_T_RAIN_START % 35 != 0) return SFLatticeFatal("SF_T_RAIN_START");
	if (SF_T_RAIN_END % 35 != 0) return SFLatticeFatal("SF_T_RAIN_END");
	if (SF_T_STORM_START % 35 != 0) return SFLatticeFatal("SF_T_STORM_START");
	if ((SF_T_STORM_START + SF_STORM_LENGTH) % 35 != 0) return SFLatticeFatal("SF_T_STORM_START+SF_STORM_LENGTH");
	if (SF_T_FLOOD_START % 35 != 0) return SFLatticeFatal("SF_T_FLOOD_START");
	if ((SF_T_FLOOD_START + SF_FLOOD_RISE) % 35 != 0) return SFLatticeFatal("SF_T_FLOOD_START+SF_FLOOD_RISE");
	if ((SF_T_FLOOD_START + SF_FLOOD_RISE + SF_FLOOD_RECEDE) % 35 != 0) return SFLatticeFatal("peak+SF_FLOOD_RECEDE");
	if (SF_T_CLAIM1 % 35 != 0) return SFLatticeFatal("SF_T_CLAIM1");
	if (SF_T_CLAIM2 % 35 != 0) return SFLatticeFatal("SF_T_CLAIM2");
	if (SF_T_EVAL % 35 != 0) return SFLatticeFatal("SF_T_EVAL");

	SetWind(20);
	g_FieldPlots = [100, 140, 180, 470, 510, 550];
	g_plant_cursor = 0; g_harvest_fp = 0; g_harvest_ridge = 0;
	g_flood_claimed = 0; g_mill_grinds = 0; g_flou_prev = 0;
	g_mill_done = 0; g_mill_frame = 0; g_t_first_task = 0;
	g_idle_steps = 0; g_calm_steps = 0; g_flood_cursor = 0;
	g_recede_level = 0; g_recede_x = 0; g_rain_cursor = 0;
	g_trench_step = 0; g_sill_cells = 0; g_done = 0;
	g_peak_done = 0; g_drain_done = 0; g_claim1_done = 0; g_claim2_done = 0;

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

	// per-frame mill-completion evacuation watcher (re-review NR1/B2):
	// the site exists from frame 0; the effect ticks every frame and
	// evacuates crew the INSTANT con snaps to 100 -- frame-granular, not
	// the 35-frame/lattice coin-flip (see FxSFEvacWatchTimer).
	var pSite = FindObject(AGWM);
	if (pSite) AddEffect("SFEvacWatch", pSite, 1, 1, 0, 0);

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

global func SFLatticeFatal(string name)
{
	FatalError(Format("Sturmfront timing lattice: %s is not a multiple of 35", name));
	return false;
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

	if (t == SF_T_RAIN_START)
	{
		// critic pass: front 1 arrived SILENT (the forecast at t=70 only
		// promises rain -- the onset itself had no announce)
		SFAnnounce("Rain is falling -- bank your harvest and mind the trench");
		Log("Sturmfront: rain begins");
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

	if (t == SF_T_FLOOD_START)
	{
		// critic pass: front 3 arrived SILENT (the peak/claim sweeps only
		// Log); the river's rise must be heard before the water spreads
		SFAnnounce("The river is rising -- the west bank will flood!");
		Log("Sturmfront: flood rise begins");
	}
	if (t >= SF_T_FLOOD_START && t < SF_T_FLOOD_START + SF_FLOOD_RISE)
		SFFloodRiseTick();
	if (t >= SF_T_FLOOD_START + SF_FLOOD_RISE && !g_peak_done)
	{
		g_peak_done = 1;
		g_recede_level = SF_FLOOD_TOP;   // recession sweeps from the spill head
		g_recede_x = 0;
		Log("Sturmfront: flood peaks");
		SFDumpCounters("flood-peak");
	}
	if (t > SF_T_FLOOD_START + SF_FLOOD_RISE && t < SF_T_FLOOD_START + SF_FLOOD_RISE + SF_FLOOD_RECEDE)
		SFFloodRecedeTick();
	if (t >= SF_T_FLOOD_START + SF_FLOOD_RISE + SF_FLOOD_RECEDE && !g_drain_done)
	{
		g_drain_done = 1;
		Log("Sturmfront: flood recedes -- drain and salvage");
		SFDumpCounters("drain-start");
	}

	if (t >= SF_T_CLAIM1 && !g_claim1_done)
	{
		g_claim1_done = 1;
		SFClaimSweep();
	}
	if (t >= SF_T_CLAIM2 && !g_claim2_done)
	{
		g_claim2_done = 1;
		SFClaimSweep();
	}
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
// column has no side walls, so the water spills onto the west flank
// natively (east flank stays behind the sealed berm). The west-field
// puddle settles ~10 px deep over the surface (empirically pinned at
// peak; liquid rests 1+ px above solid -- claimprobe).
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
	Log(Format("SFMT:jobs_pending=%d", SFJobsPending()));
	Log(Format("SFMT:seeds_left=%d", SFSeedsLeft()));
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

// ---------------- job availability (instrument, read-only) -----------------

// Honest "was there work?" line, distinct from idle_pct (which measures
// empty command stacks -- the model assists finish synchronously, so the
// crew legitimately reads ~98% idle while the crops ripen). Returns the
// number of ladder jobs a freshly assigned idle clonk could take right
// now: berm cells remaining, trench steps remaining, the mill site
// (incomplete, or complete with a sheaf available), and farming (ripe
// wheat, or a dry sowable plot with seeds left). MUST NOT mutate driver
// state -- unlike SFNextDryPlot it does not advance g_plant_cursor.
global func SFJobsPending()
{
	var n = 0;
	if (!SF_SKIP_WALL && g_sill_cells < SF_BERM_CELLS) ++n;
	if (!SF_SKIP_TRENCH && g_trench_step < 10) ++n;
	var pSite = FindObject(AGWM);
	if (!SF_SKIP_MILL && pSite)
	{
		if (GetCon(pSite) < 100) ++n;
		else if (ObjectCount(AGSH) > 0) ++n;
	}
	if (SFFarmPending()) ++n;
	return n;
}

global func SFSeedsLeft()
{
	var pBase = FindObject(WRKS);
	if (!pBase) return 0;
	return ContentsCount(AGWS, pBase);
}

global func SFFarmPending()
{
	var pBase = FindObject(WRKS);
	// a ripe plant anywhere is harvest work (SFFindRipeWheat is read-only)
	if (SFFindRipeWheat()) return true;
	// no sowing without seeds in the workbench
	if (!pBase || ContentsCount(AGWS, pBase) <= 0) return false;
	// any dry, unoccupied plot left to sow
	return SFAnyDryPlot();
}

// read-only dry-plot scan (mirrors SFNextDryPlot's predicate, MUST NOT
// advance g_plant_cursor).
global func SFAnyDryPlot()
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
		if (GBackLiquid(px, sy - 1)) continue;            // pooled at surface
		if (GBackLiquid(px, sy - 2)) continue;            // 3-4 px standing pool
		if (GBackLiquid(px, sy - 3)) continue;            // flood puddle
		var occupied = FindObjects(Find_ID(AGWH), Find_InRect(px - 8, sy - 22, 16, 24));
		if (GetLength(occupied) > 0) continue;
		return true;
	}
	return false;
}

// ---------------- claim rule (rule 1) --------------------------------------

// a flood destroys crops, not just ripe ones (cycle-175 A5): ANY
// in-ground AGWH under liquid is swept. The anchor cell of a plant on
// flat ground sits 1-2 px above the ground line, which standing water
// NEVER wets (liquid rests 1+ px off solid; empirical claimprobe: the
// west-field crop anchors stayed dry under the full 10-px pool while
// their stems were submerged) -- so the probe reads the plant's stem up
// to 12 px above the anchor. Mirror: SturmfrontSmoke.c4s SFClaimSweep.
global func SFUnderLiquid(object pW)
{
	if (GBackLiquid(GetX(pW), GetY(pW))) return true;
	if (GBackLiquid(GetX(pW), GetY(pW) - 6)) return true;
	if (GBackLiquid(GetX(pW), GetY(pW) - 12)) return true;
	return false;
}

global func SFClaimSweep()
{
	var n = 0, rect, x1, x2, pW, pS;
	for (rect = 0; rect < 2; rect++)
	{
		if (rect == 0) { x1 = SF_FIELD_W1; x2 = SF_FIELD_W2; }
		else { x1 = SF_FIELD_E1; x2 = SF_FIELD_E2; }
		for (var pW in FindObjects(Find_ID(AGWH), Find_InRect(x1, SF_FIELD_Y1, x2 - x1, SF_FIELD_Y2 - SF_FIELD_Y1)))
			if (SFUnderLiquid(pW))
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

	// mill completion frame (the wood-role causality observable). The
	// Build command parks its workers AT the construction base (they
	// stood at (788,209) INSIDE the footprint in the first probes); when
	// con snaps to 100 the full building shape closes around them and
	// contact-incinerates/suffocates them all ~20 frames later (Windmill
	// DefCore ContactIncinerate=4; footprint x 776..804). NB: this
	// lattice sample (35 frames) only DETECTS the frame -- the
	// EVACUATION itself is the per-frame SFEvacWatch effect armed on the
	// site at Initialize (context: the old radius-20 loop here sampled
	// con on the driver lattice against a ~20-frame kill fuse, a ~57%
	// coin-flip that wiped the var-trench crew at mill_complete=1225 with
	// zero evacuation transits while the shipped comment falsely claimed
	// "SetPosition is atomic -- beats the 20-frame danger window").
	var pSite = FindObject(AGWM);
	if (!g_mill_done && pSite && GetCon(pSite) >= 100)
	{
		g_mill_done = 1;
		g_mill_frame = t;
		Log(Format("SFMT:mill_complete=%d", t));
	}

	// idle instrumentation: calm windows only (proxy c). Per-clonk census
	// (review A1): the shipped any-clonk-empty form made ONE idle clonk
	// count the whole step idle -- a driver artifact that fails bar c on
	// honest ripening waits. Here every crew member's step is a sample:
	// idle_pct = idle crew-steps / total crew-steps. The storm window
	// [21000, 22400) is NOT calm (both halves of this expression exclude
	// it), so the hunkered crew is never counted idle -- matching the
	// spec's calm-window schedule.
	var calm = (t > SF_T_RAIN_END && t < SF_T_STORM_START)
	        || (t > SF_T_STORM_START + SF_STORM_LENGTH && t < SF_T_FLOOD_START);
	var crew = SFCrewList();
	if (calm && GetLength(crew) > 0)
	{
		g_calm_steps += GetLength(crew);
		var c;
		for (c = 0; c < GetLength(crew); c++)
			if (!GetCommand(crew[c])) ++g_idle_steps;
	}

	// STORM HUNKER: the storm is not a work window ("Hold fast -- the
	// storm is upon us!"; the spec schedules the economy in calm windows).
	// Seeds 176/177 lost all four clonks to lightning while the greedy
	// ladder kept assigning outdoor transits/harvests during the storm
	// (~frame 21100, fixprobe-176/177.log / baseline-176.log:725-744).
	// At the storm's first driver tick the crew TRANSPORTS to scattered
	// SAFE havens (fix 2: hunkering WHERE THEY STOOD still left the
	// grind/bank branches clustered at the wooden mill/WRKS -- the fire
	// from the burning structure reached them right after storm end in
	// seed 177, baseline-177.log) and every command stack is cleared so
	// nobody is mid-transit; then NO new jobs are assigned until the
	// storm ends.
	if (SFStormActive())
	{
		if (t == SF_T_STORM_START)
		{
			var h, hc = SFCrewList();
			for (h = 0; h < GetLength(hc); h++)
			{
				// ASSIST GATE (critic pass): the haven transport is
				// instrument-only -- a human shelters in place and never
				// sees the crew teleport across the map at storm onset
				if (!SF_AUTOPLAY) continue;
				var hx = SFSafeHaven(h % 4);
				SetPosition(hx, SFSurfaceY(hx) - 12, hc[h]);
				Log(Format("SFMT:transit=%d", hx));
			}
			for (h = 0; h < GetLength(hc); h++) SetCommand(hc[h], "None");
			Log("SFMT:storm_hunker=1");
		}
		return 1;
	}

	// SPAWN MARSHAL (t == 35, once): the [Player] Position=70,35 drops
	// the crew on the west shoulder ledges (x650-665, y228-300) where
	// MoveTo cannot route (a clonk fell off a ledge and died at ~frame
	// 385 in the first fixprobe; two more churned assign_timeouts for
	// the whole run). Pull EVERY clonk to the terrace before the ladder
	// starts; from there all further moves are zone-transited.
	// ASSIST GATE (critic pass): the pull is instrument-only -- a human
	// plays from the spawn ledges and never sees the crew teleport.
	if (t == 35)
	{
		// boot evidence (one-shot): the four storm havens each resolve to
		// solid ground -- log their surfaces (220/120/400/440-or-419)
		var hq;
		for (hq = 0; hq < 4; hq++)
			Log(Format("SFMT:haven_%d_surf=%d", hq, SFSurfaceY(SFSafeHaven(hq))));
		var m;
		if (SF_AUTOPLAY)
			for (m = 0; m < GetLength(crew); m++)
			{
				SetPosition(SF_WRKS_X - 10, SFSurfaceY(SF_WRKS_X - 10) - 12, crew[m]);
				Log(Format("SFMT:transit=%d", SF_WRKS_X - 10));
			}
	}

	// keep every crew clonk's command stack non-empty (priority ladder:
	// any idle clonk takes the first actionable job -- no i%4 roles, B3)
	var i;
	for (i = 0; i < GetLength(crew); i++)
	{
		if (GetCommand(crew[i])) continue;
		SFAssignNext(crew[i]);
	}
	return 1;
}

// priority ladder: 1) BERM, 2) TRENCH, 3) MILL, 4) FARM (the shared
// fallback). Each assignment arms a job-watch that recovers stuck stacks.
global func SFAssignNext(object clnk)
{
	if (g_t_first_task == 0)
	{
		g_t_first_task = FrameCounter();
		Log(Format("SFMT:t_first_task=%d", g_t_first_task));
	}
	SFAssignEffect(clnk);
	var pBase = FindObject(WRKS);
	var pSite = FindObject(AGWM);

	if (!SF_SKIP_WALL && g_sill_cells < SF_BERM_CELLS) return SFWallWork(clnk);
	if (!SF_SKIP_TRENCH && g_trench_step < 10) return SFTrenchWork(clnk);
	if (!SF_SKIP_MILL && pSite)
	{
		if (SFMillWork(clnk)) return true;
		// mill has no grind work (complete + no sheaf anywhere): fall
		// through to farming -- never a return-deadend (A1)
	}
	return SFFarmWork(clnk, pBase);
}

// job-watch: any command stack still non-empty 350 frames (10 x 35)
// after an assignment is stuck (the 200-cap jam class: C4Object::AddCommand
// refused at 200 pending) -- recover: clear the stack, re-ladder next tick.
global func SFAssignEffect(object clnk)
{
	RemoveEffect("SFAssign", clnk);
	AddEffect("SFAssign", clnk, 1, 35, 0, 0);
	return true;
}

global func FxSFAssignTimer(object target, int effect, int frame)
{
	if (!GetCommand(target))
	{
		RemoveEffect("SFAssign", target);
		return 1;
	}
	if (frame >= 350)
	{
		SetCommand(target, "None");   // engine path: C4Script.cpp:875-884 -> ClearCommands
		RemoveEffect("SFAssign", target);
		Log(Format("SFMT:assign_timeout=%d", FrameCounter()));
	}
	return 1;
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

// true while the storm window is active (21000..22400 after the R3
// mitigation): the driver must not send clonks across open ground in
// lightning -- work happens in the calm windows (spec + announce).
global func SFStormActive()
{
	var t = FrameCounter();
	return t >= SF_T_STORM_START && t < SF_T_STORM_START + SF_STORM_LENGTH;
}

// storm havens (fix 2): four scattered, structure-free stand points for
// the hunker, read from the map: 870 = terrace east end (surface 220),
// 915 = peak crown rock (surface 120), 25 = west crag top (rock, surface
// 400), 540 = berm midspan (surface 419, 440 pre-berm). Each is >= 80 px
// from the workbench (720) and the mill (790), >= 40 px from the others,
// and stands on solid ground (SFSurfaceY; the surfaces are logged once
// at frame 35 as SFMT:haven_*_surf for the boot probe). Taking shelter
// away from the tall wooden structures is the honest reading of "hold
// fast"; the old freeze-in-place hunker left the crew clustered at the
// mill/WRKS and the burning structure's fire reached them after storm
// end (seed 177, baseline-177.log).
global func SFSafeHaven(int i)
{
	if (i == 0) return 870;
	if (i == 1) return 915;
	if (i == 2) return 25;
	return 540;
}

// SFEvacWatch: per-frame mill-completion evacuation (re-review NR1/B2).
// Armed on the construction site at Initialize (AddEffect("SFEvacWatch",
// pSite, 1, 1) -- interval 1 runs EVERY frame). The 35-frame driver
// lattice samples con once per 35 ticks, but the Build snap (99->100)
// lands on an arbitrary frame and the completed shape kills occupants on
// a ~20-frame fuse (Windmill DefCore ContactIncinerate=4; footprint
// x 776..804) -- the old lattice-sampled radius-20 guard around x790
// missed the driver's own x760 staging point (30 px out) and wiped the
// var-trench crew at mill_complete=1225 with zero evacuation transits
// (baselines survived 3/3 by phase luck; the reviewers' arithmetic puts
// the survival chance at ~57%). This effect checks con on EVERY frame
// and the frame con reaches 100, transports every crew clonk within
// 60 px of the mill (covers the staging point AND the footprint) to
// their scattered haven (>= 80 px away), logs one SFMT:mill_evac per
// evacuated clonk, and removes itself.
// UNGATED BY SF_AUTOPLAY (deliberate exception, critic pass): the
// transit/haven/marshal teleports are instrument assists and hide
// behind SF_AUTOPLAY -- but THIS one guards against the engine's
// completion-snap kill, a DEFECT not an assist, and a nudge beats a
// death for a human player too. If the engine ever resolves con snaps
// death-free (or clonks stop being Build-parked inside the footprint),
// this effect is the first thing to delete.
global func FxSFEvacWatchTimer(target, effect, time)
{
	if (GetCon(target) < 100) return 1;          // not complete -- keep watching
	var e, cc = SFCrewList();
	for (e = 0; e < GetLength(cc); e++)
		if (Abs(GetX(cc[e]) - SF_MILL_X) < 60)
		{
			var hx = SFSafeHaven(e % 4);
			SetPosition(hx, SFSurfaceY(hx) - 12, cc[e]);
			Log("SFMT:mill_evac=1");
		}
	return -1;                                   // done -- remove the effect
}

// ---------------- zone transit (B3: no foot route off the islands) --------

global func SFZoneOf(int x)
{
	if (x < 240) return 1;       // west floodplain
	if (x < 430) return 2;       // canyon -- never a destination
	if (x < 700) return 3;       // east valley
	return 4;                    // ridge
}

// assisted inter-zone movement: SetPosition(x, surface-12) instead of a
// doomed MoveTo. Returns true when the clonk was transported (the caller
// must skip its MoveTo this tick -- the clonk is already at the target).
// Counted via SFMT:transit for the playtest instrument.
// ASSIST GATE (critic pass): the transport is instrument-only -- with
// auto-play off a human never sees the crew teleport (return false and
// let the caller's own MoveTo carry them, as a player's clonk would).
global func SFTransit(object clnk, int tx)
{
	if (!SF_AUTOPLAY) return false;
	if (SFZoneOf(GetX(clnk)) == SFZoneOf(tx)) return false;
	SetPosition(tx, SFSurfaceY(tx) - 12, clnk);
	Log(Format("SFMT:transit=%d", tx));
	return true;
}

// ---- priority 2 (after the berm): the drainage trench --------------------
// L-shape: vertical shaft x464-476 from the (dynamic!) east surface down
// 6 x 12-deep steps, then horizontal adit y492-504 from the canyon (x428)
// east to the shaft in 4 steps (existing steps 0-9, keep SFMT:trench_step).
// Dynamic shaft top: the berm raises the east surface from 440 to ~419; a
// fixed 445 would leave the shaft mouth plugged behind berm cells. The
// digger stands BESIDE the dig column at (sx+24, ground), never over the
// mouth (fall-in deaths, B3 root cause 2).
global func SFTrenchWork(object clnk)
{
	var sx = 470, sy = SFSurfaceY(470);
	var tx = sx + 24, ty = SFSurfaceY(tx) - 12;
	if (g_trench_step < 6)
	{
		if (SFTransit(clnk, tx)) { }
		else if (!SFClkNear(clnk, tx, ty, 60))
		{
			AddCommand(clnk, "MoveTo", 0, tx, ty);
			return true;
		}
		DigFreeRect(sx - 6, sy + g_trench_step * 10, 12, 12);
	}
	else
	{
		// the adit is dug from beside the shaft column (the digger cannot
		// stand inside the adit -- the assist works from the shaft mouth)
		if (SFTransit(clnk, tx)) { }
		else if (!SFClkNear(clnk, tx, ty, 80))
		{
			AddCommand(clnk, "MoveTo", 0, tx, ty);
			return true;
		}
		DigFreeRect(428 + (g_trench_step - 6) * 8, 492, 12, 12);
	}
	++g_trench_step;
	Log(Format("SFMT:trench_step=%d", g_trench_step));
	if (g_trench_step == 10) Log("Sturmfront: the trench is through -- the field can drain");
	return true;
}

// ---- priority 3: finish the mill, then grind (real commands) -------------
global func SFMillWork(object clnk)
{
	var pBase = FindObject(WRKS);
	var pSite = FindObject(AGWM);
	if (!pSite) return false;
	if (GetCon(pSite) < 100)
	{
		// The wood/metal haul chart is a headless NO-OP in this engine
		// (the Acquire section search cannot reach container contents:
		// haulprobe1.log carried_wood stayed 0 forever) and the Build
		// command raises construction on its own (probe: 40->100 by
		// Build alone). The brief's haul pairs are kept for SFMT/SF_SKIP
		// fidelity -- the site ALWAYS gets the Build in the same stack,
		// so the mill completes regardless (skip-wood frees the clonk
		// from the cosmetic haul attempts; the mill_* columns coincide
		// under both switches BY DESIGN).
		if (!SF_SKIP_WOOD && pBase)
		{
			if (ContentsCount(WOOD, pBase) > 0 && ContentsCount(WOOD, pSite) < 7)
			{
				if (SFTransit(clnk, SF_MILL_STAGE)) return true;
				AddCommand(clnk, "Acquire", 0, 0, 0, pBase, 0, WOOD, 1, 3);
				AddCommand(clnk, "Put", pSite, 0, 0, 0, 0, WOOD);
			}
			else if (ContentsCount(METL, pBase) > 0 && ContentsCount(METL, pSite) < 1)
			{
				if (SFTransit(clnk, SF_MILL_STAGE)) return true;
				AddCommand(clnk, "Acquire", 0, 0, 0, pBase, 0, METL, 1, 3);
				AddCommand(clnk, "Put", pSite, 0, 0, 0, 0, METL);
			}
		}
		else if (SFTransit(clnk, SF_MILL_STAGE)) return true;
		AddCommand(clnk, "Build", pSite, 0, 0, 0, 0, 0, 0, 3);
		return true;
	}
	// Full mill: grind. The in-tree Windmill.ProductionOrder Put/Acquire
	// chain cannot complete in this engine -- the windmill has no
	// OCF_Collection/GrabPutGet/Entrance, so the Put command stalls with
	// the sheaf stuck in the worker's hands (haulprobe4/5/6: carried=1
	// forever, mill_sheaves=0). The working primitives (haulprobe9-11 +
	// engine C4Object.cpp:4213/5577): sheafs are banked INTO the mill by
	// the farm's bank assist (SFFarmWork step 1 -- the same Remove-
	// Object+CreateContents style the sow assist uses), then this branch
	// Calls ProductionStart: it consumes the mill's sheaf, enters the
	// Grinding action at con==100 (incomplete-phase actions are forced
	// to Idle and would eat the sheaf silently), whose looping action
	// climbs Action.Time and releases FLOU after grind_time (~160).
	// RE-REVIEW NR2 (sheaf leak): ProductionStart UNCONDITIONALLY
	// consumes one hopper sheaf (Windmill.c4d/Script.c:68 RemoveObject
	// (FindContents(AGSH))) even when the mill is ALREADY Grinding -- and
	// a same-action SetAction preserves Action.Time (C4Object.cpp:4211:
	// "Reset action time on change" only on CHANGE), so the call does
	// NOT restart the ongoing grind and the consumed sheaf produces
	// NOTHING. Measured: 44 units in -> 12 FLOU + 31 burned + 1 claimed
	// across every mill-active run (var-mill's no-grind control: 43
	// banked). Guard: only queue the grind while the mill is Idle --
	// otherwise fall through the ladder to farm (the A1 contract).
	if (ContentsCount(AGSH, pSite) <= 0) return false;   // farm banks next
	if (GetAction(pSite) == "Grinding") return false;    // already grinding -- stand down
	if (SFTransit(clnk, SF_MILL_STAGE)) return true;
	AddCommand(clnk, "MoveTo", 0, SF_MILL_STAGE, SFSurfaceY(SF_MILL_STAGE) - 12);
	AddCommand(clnk, "Call", pSite, 0, 0, 0, 0, "ProductionStart");
	return true;
}

// ---- priority 1: raise the east-rim full-span berm (assist: Earth pours) -
// 220 cols (x430-649) x 21 rows (y439 up to 419): the WHOLE east rim is
// sealed, top row 419 sits above the lake top 420. The west rim stays
// open -> the west flank floods (that IS the design: west = exposed
// floodplain, east = protected by the wall role, ridge = always safe).
// The stand spot is mid-span on the berm top; the pour is scripted
// (InsertMaterial), so the clonk does not need to walk the rim. Wheat
// standing in berm columns gets embedded -- accepted (harvest radius 20
// px from the berm top still reaches it; the berm-before-trench order +
// the dynamic trench surface keeps this consistent).
global func SFWallWork(object clnk)
{
	var tx = 540, ty = SFSurfaceY(tx) - 12;
	if (SFTransit(clnk, tx)) { }
	else if (!SFClkNear(clnk, tx, ty, 90))
	{
		AddCommand(clnk, "MoveTo", 0, tx, ty);
		return true;
	}
	var m = Material("Earth");
	var i, x, y;
	for (i = 0; i < 40 && g_sill_cells < SF_BERM_CELLS; i++)
	{
		x = 430 + g_sill_cells % 220;
		y = 439 - g_sill_cells / 220;
		if (!GBackSolid(x, y)) InsertMaterial(m, x, y);
		++g_sill_cells;
	}
	if (g_sill_cells % 160 < 40) Log(Format("SFMT:sill_cells=%d", g_sill_cells));
	if (g_sill_cells % 160 < 40) Log(Format("SFMT:berm_cells=%d", g_sill_cells));
	if (g_sill_cells >= SF_BERM_CELLS) Log("Sturmfront: the east levy stands -- the flood takes the west bank first");
	return true;
}

// ---- shared fallback: sow / harvest / bank (rule 2: dry feet) ------------
global func SFFarmWork(object clnk, object pBase)
{
	// 1) bank carried sheaves -- SCRIPTED ASSIST: no container-Put works
	//    headless in this engine (the Put needs a collection/entrance
	//    the containers lack; haulprobe1/4-6), so the carried sheaf is
	//    moved with the same RemoveObject+CreateContents pair the sow
	//    assist uses. Completed mill first (up to its 5-cap, Windmill
	//    RejectCollect): the delivered sheaf is what the mill branch
	//    grinds. Leftover sheafs go to the workbench (both count in
	//    GranaryUnits).
	if (ContentsCount(AGSH, clnk) > 0)
	{
		var pSite = FindObject(AGWM);
		if (pSite && GetCon(pSite) >= 100 && ContentsCount(AGSH, pSite) < 5)
		{
			var pS = clnk->FindContents(AGSH);
			if (pS) RemoveObject(pS);
			CreateContents(AGSH, pSite, 1);
			return true;
		}
		if (pBase)
		{
			var pS2 = clnk->FindContents(AGSH);
			if (pS2) RemoveObject(pS2);
			CreateContents(AGSH, pBase, 1);
			return true;
		}
		return true;
	}
	// 2) harvest ripe wheat (walk there, then the native Harvest at feet)
	var pW = SFFindRipeWheat();
	if (pW)
	{
		var wx = GetX(pW), wy = GetY(pW);
		if (SFTransit(clnk, wx)) return true;
		if (!SFClkNear(clnk, wx, wy, 25))
		{
			AddCommand(clnk, "MoveTo", 0, wx, wy);
			return true;
		}
		pW->Harvest(clnk);
		if (SFOnRidge(wx)) ++g_harvest_ridge;
		else ++g_harvest_fp;
		Log(Format("SFMT:harvest_fp=%d", g_harvest_fp));
		Log(Format("SFMT:harvest_ridge=%d", g_harvest_ridge));
		return true;
	}
	// 3) sow a dry plot (dry-feet rule: the surface cell must not be
	// liquid; probe 3 px down too -- a flood puddle rests 1+ px above the
	// ground, so a surface-only probe misses a standing pool). Any dry
	// plot while seeds remain (the greedy early-sowing cycle; a late-sow
	// reserve wave was tried and removed -- 0 sow_wave firings across
	// the round-4 seeds, and its hold-back starved the granary).
	var px = SFNextDryPlot();
	if (px > 0 && pBase && ContentsCount(AGWS, pBase) > 0)
	{
		var sy = SFSurfaceY(px);
		if (SFTransit(clnk, px)) return true;
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
	//    empties -- surfaced honestly by idle_pct). Stand target is BELOW
	//    the 700-zone boundary panes: x710 (zone 4) on the terrace
	//    surface -- the old (WRKS_x - 30 = 690, GetY(pBase)) target sat
	//    on the zone-3 side of the boundary, so the transit never fired
	//    and the mid-air y (the WRKS anchor floats ~26 px off the ground)
	//    left the clonks climbing the toe face forever (assign_timeout
	//    churn, DIA-timeout x636/y316).
	//    Stand positions are SCATTERED across the terrace (one per crew
	//    member, 45 px apart): seeds 176/177 showed a storm lightning
	//    blast on the single standby pixel incinerating all four clonks
	//    AND the workbench in one tick (fixprobe-176/177: 4 simultaneous
	//    eliminations ~1 s into the storm) -- one pixel per stand means
	//    one blast cannot end the run.
	if (pBase)
	{
		var c = 0, ci, cl = SFCrewList();
		for (ci = 0; ci < GetLength(cl); ci++)
			if (cl[ci] == clnk) { c = ci; break; }
		var sx = SF_WRKS_X - 10 + c * 45;   // 710 / 755 / 800 / 845, all zone 4
		if (SFTransit(clnk, sx)) return true;
		AddCommand(clnk, "MoveTo", 0, sx, SFSurfaceY(sx) - 12);
	}
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
		if (GBackLiquid(px, sy - 1)) continue;            // pooled at surface
		if (GBackLiquid(px, sy - 2)) continue;            // 3-4 px standing pool
		if (GBackLiquid(px, sy - 3)) continue;            // flood puddle (see 3)
		var occupied = FindObjects(Find_ID(AGWH), Find_InRect(px - 8, sy - 22, 16, 24));
		if (GetLength(occupied) > 0) continue;
		g_plant_cursor = idx + 1;
		return px;
	}
	return 0;
}
