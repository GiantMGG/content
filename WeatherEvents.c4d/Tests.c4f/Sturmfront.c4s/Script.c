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
     21000-23100  front 2 storm   -- LaunchWeatherEvent(STRM,60,2100)
     23100-31500  calm 2          -- bank crunch (knife-edge cycle 3)
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
    - late-sow reserve wave: SF_SEED_RESERVE seeds are held back until
      SF_T_LATESOW (28980, 828 x 35 -- late enough that a wave seed is
      Seedling through BOTH claim sweeps 31850/32480 and never Ready
      before eval), then sown on the three WEST plots only -- in-ground
      over the flood peak, so the claim rule destroys them and the
      floodplain gamble becomes mechanical (without it the greedy kit
      exhausts all seeds by ~frame 16000 and the fields are fallow at
      the flood, "claim sweep -- nothing submerged" in baselines
      176/177); tracked per sow via SFMT:sow_wave; the round ALSO pinned
      that the reserve was only ever destroyed with the burning
      workbench in 176/177 (intact=0), never sown -- the trace will
      prove which; an occupied floodplain is a planted floodplain, so
      seeds_left > 0 at eval is correct;
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
static const SF_GRANARY_QUOTA  = 12;    // calibration 10-16 (playtest task)
static const SF_KIT_SEEDS      = 20;    // 3 field cycles need 18 (D3)
static const SF_KIT_WOOD       = 16;
static const SF_KIT_METAL      = 2;

// LATE-SOW RESERVE WAVE (fix 1): hold back SF_SEED_RESERVE seeds until
// SF_T_LATESOW. Without it the greedy kit exhausts all 20 seeds by
// ~frame 16000 and the fields are FALLOW at the flood ("claim sweep --
// nothing submerged" in baseline-176/177, claim=1 in 175): the spec's
// bar-b rationale (a cycle-4 sowing stands in-ground at the flood) was
// structurally unreachable.
//   Fix round 2 (ripeness race): SF_T_LATESOW moved 28000 -> 28980
// (= 828 x 35). A wave seed sown at 28980 is Seedling at 28980+3540
// = 32520 (> both claim sweeps 31850/32480), i.e. in-ground and
// un-harvestable (never Ready before eval 33600; Ready needs +7080 =
// 36060) through the entire flood window - the claim takes it, no
// "ripened dry before the water tops the rim" window remains. The
// baselines also showed the reserve was only ever *destroyed with the
// burning workbench* (seeds_left 20/14 -> 0 across the storm in
// 176/177, outcome_intact=0), never sown - sow_wave traces that.
static const SF_SEED_RESERVE  = 6;
static const SF_T_LATESOW     = 28980;

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
	// late-sow reserve hold-back: before the wave, seeds at/below the
	// reserve are a DELIBERATE DEFERRAL, not pending work (fix 1);
	// from the wave onward only a west-field dry plot counts.
	var t = FrameCounter();
	if (t < SF_T_LATESOW && ContentsCount(AGWS, pBase) <= SF_SEED_RESERVE) return false;
	if (t >= SF_T_LATESOW) return SFAnyDryPlot(true);
	return SFAnyDryPlot(false);
}

// read-only dry-plot scan (mirrors SFNextDryPlot's predicate, MUST NOT
// advance g_plant_cursor). fWestOnly restricts to the LATE-SOW wave's
// target list -- EXACTLY the three west field plots (g_FieldPlots[0..2]
// = 100/140/180), no east, no ridge, no other source. Under SF_RIDGE_ONLY
// the wave is naturally inert -- that variant never touches field plots,
// so no west plot is ever offered (comment: fix 1).
global func SFAnyDryPlot(bool fWestOnly)
{
	if (fWestOnly) return SFWestPlotFree();
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

// any of the three west floodplain plots dry AND unoccupied? (fix 1,
// read-only). An occupied floodplain is a PLANTED floodplain: if all
// three already carry the wave's crops the remaining reserved seeds
// stay in the WRKS -- seeds_left > 0 at eval is CORRECT behavior.
global func SFWestPlotFree()
{
	var i, px, sy;
	for (i = 0; i < 3; i++)
	{
		px = g_FieldPlots[i];
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
	// contact-incinerates/suffocates them all ~20 frames later. Evacuate
	// anyone still inside the footprint AT the completion tick
	// (SetPosition is atomic -- beats the 20-frame danger window).
	var pSite = FindObject(AGWM);
	if (!g_mill_done && pSite && GetCon(pSite) >= 100)
	{
		g_mill_done = 1;
		g_mill_frame = t;
		Log(Format("SFMT:mill_complete=%d", t));
		var e, cc = SFCrewList();
		for (e = 0; e < GetLength(cc); e++)
			if (Abs(GetX(cc[e]) - SF_MILL_X) < 20)
			{
				SetPosition(SF_WRKS_X - 10, SFSurfaceY(SF_WRKS_X - 10) - 12, cc[e]);
				Log(Format("SFMT:transit=%d", SF_WRKS_X - 10));
			}
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
	// starts; from there all further moves are zone-transited. Interactive
	// players get the same assist (harmless).
	if (t == 35)
	{
		// boot evidence (one-shot): the four storm havens each resolve to
		// solid ground -- log their surfaces (220/120/400/440-or-419)
		var hq;
		for (hq = 0; hq < 4; hq++)
			Log(Format("SFMT:haven_%d_surf=%d", hq, SFSurfaceY(SFSafeHaven(hq))));
		var m;
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

// the west floodplain zone (x < 240): the un-bermed field the flood
// takes -- the late-sow reserve wave sows ONLY here (fix 1), so the
// flood can claim it while the east/ridge crops stay safe.
global func SFOnWestField(int x)
{
	return x < 240;
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
global func SFTransit(object clnk, int tx)
{
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
				if (SFTransit(clnk, SF_MILL_X - 30)) return true;
				AddCommand(clnk, "Acquire", 0, 0, 0, pBase, 0, WOOD, 1, 3);
				AddCommand(clnk, "Put", pSite, 0, 0, 0, 0, WOOD);
			}
			else if (ContentsCount(METL, pBase) > 0 && ContentsCount(METL, pSite) < 1)
			{
				if (SFTransit(clnk, SF_MILL_X - 30)) return true;
				AddCommand(clnk, "Acquire", 0, 0, 0, pBase, 0, METL, 1, 3);
				AddCommand(clnk, "Put", pSite, 0, 0, 0, 0, METL);
			}
		}
		else if (SFTransit(clnk, SF_MILL_X - 30)) return true;
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
	if (ContentsCount(AGSH, pSite) <= 0) return false;   // farm banks next
	if (SFTransit(clnk, SF_MILL_X - 30)) return true;
	AddCommand(clnk, "MoveTo", 0, SF_MILL_X - 30, SFSurfaceY(SF_MILL_X - 30) - 12);
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
	// ground, so a surface-only probe misses a standing pool).
	// LATE-SOW RESERVE WAVE (fix 1): before SF_T_LATESOW the driver holds
	// back SF_SEED_RESERVE seeds (no sowing while seeds are at/below the
	// reserve -- harvesting and banking continue untouched; the greedy
	// kit otherwise exhausts all seeds by ~frame 16000 and the fields are
	// FALLOW at the flood); from the wave onward the REMAINING seeds are
	// sown onto the three WEST plots ONLY (SFNextWestPlot -- the exposed
	// floodplain): in-ground and un-harvestable through both claim
	// sweeps (sown at >= 28980 -> Seedling/Growing at >= 32520 > 32480),
	// so the flood claims them and the bar-b gamble is mechanical.
	var px = 0;
	var tNow = FrameCounter();
	if (pBase && ContentsCount(AGWS, pBase) > 0)
	{
		if (tNow >= SF_T_LATESOW) px = SFNextWestPlot();
		else if (ContentsCount(AGWS, pBase) > SF_SEED_RESERVE) px = SFNextDryPlot();
	}
	if (px > 0 && pBase)
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
		if (tNow >= SF_T_LATESOW) Log(Format("SFMT:sow_wave=%d", px));
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

global func SFNextWestPlot()
{
	// the LATE-SOW wave's target list is EXACTLY the three west field
	// plots (g_FieldPlots[0..2] = 100/140/180) -- no east, no ridge, no
	// other source; reads the dry-feet + occupancy predicate of the
	// normal rotation but does NOT advance g_plant_cursor (the wave is a
	// fixed list). If all three are occupied (grown wave plants), zero is
	// returned and the remaining reserved seeds stay in the WRKS --
	// seeds_left > 0 at eval is CORRECT (an occupied floodplain is a
	// planted floodplain).
	var i, px, sy;
	for (i = 0; i < 3; i++)
	{
		px = g_FieldPlots[i];
		sy = SFSurfaceY(px);
		if (GBackLiquid(px, sy - 1)) continue;            // pooled at surface
		if (GBackLiquid(px, sy - 2)) continue;            // 3-4 px standing pool
		if (GBackLiquid(px, sy - 3)) continue;            // flood puddle
		var occupied = FindObjects(Find_ID(AGWH), Find_InRect(px - 8, sy - 22, 16, 24));
		if (GetLength(occupied) > 0) continue;
		return px;
	}
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
