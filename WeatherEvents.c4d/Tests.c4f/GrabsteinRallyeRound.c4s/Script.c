/*-- GrabsteinRallyeRound: scripted 4-racer bot race on the GrabsteinRallye
  course (cycle 200, roadmap grabstein-rallye-prototype, Phase A gate).

  Permanent gate for "a scripted 4-racer race runs green and the
  dug-pixel telemetry emits" - the ArenaBotRound pattern (cycle 172):
  four TestPlayer fixtures join, the GRBotDriver effect drives all four
  cursors down the course with real MoveTo commands plus a
  stuck-watchdog DigFreeRect assist, the leader crossing the finish
  column hands the round to RACE+RVLR (ELIMINATION + GOAL controller ->
  GameOver - the round ends ENGINE-OWNED, the smoke never calls
  GameOver), and the GRHeatmap diff must emit GRHM:lap=1 total>0.

  The scenario name carries NO "Smoke" suffix: the Tests.c4f glob must
  not double-register it (arena_bot_round_smoke precedent). The custom
  CTest entry (tests/CMakeLists.txt) passes the four player fixtures
  and the seed pin 200.

  Late canary (phase 3): the engine-owned end needs retire (~60 ticks)
  + GOAL controller CheckTime poll (Timer=250) + Wait4End (Delay 30);
  everything fits inside 13 steps (455 ticks). Reaching canary step 13
  means the round did NOT end by itself - the cycle-172 B1 class. --*/

#strict 2

// ---- RACE goal callbacks (Goldrace values, verbatim) ----
func GetRACEDirection() { return 1; }  // 1: left -> right
func GetRACEStartOffset() { return 20; }
func GetRACEEndOffset() { return 30; } // win column: GetX(cursor) > LandscapeWidth()-30

// ---- course geometry (world px; 100x60 map, zoom 10 -> 1000x600) ----
static const GR_GateX = 40;     // spawn column on the start shelf
static const GR_GateY = 240;    // standing line (start-shelf surface y=250)
static const GR_FinishX = 970;  // mirrors LandscapeWidth()-GetRACEEndOffset()

// fixed race seam: Sand band through the base of the mid-shelf pillar
// (map cols 52-53, world x 520-540). The pillar above is unjumpable, so
// the seam band IS the shortcut every racer digs their own line through.
static const GR_SeamX1 = 505;
static const GR_SeamY1 = 280;
static const GR_SeamX2 = 545;
static const GR_SeamY2 = 320;

// ---- telemetry grid: 20px cells, 50x30 over the 1000x600 world ----
static const GR_Cell  = 20;
static const GR_GridW = 50;
static const GR_GridH = 30;

// ---- bot course waypoints (world px) --------------------------------------
global func GRWaypointX(int i)
{
	if (i <= 0) return 60;
	if (i == 1) return 260;
	if (i == 2) return 480;
	if (i == 3) return 600;
	if (i == 4) return 800;
	return 972;
}
global func GRWaypointY(int i)
{
	if (i <= 0) return 242;
	if (i == 1) return 312;
	if (i == 2) return 312;
	if (i == 3) return 312;
	if (i == 4) return 372;
	return 372;
}
static const GR_WPCount = 6;

static g_GRbase;          // per-cell solid-sample bitmask at lap start
static g_GRmat;           // per-cell baseline material (cell-centre sample)
static g_GRdug;           // dug cells at the last lap-end diff
static g_GRlapDone;       // 1 once the leader crossed and the diff ran
static g_GRseamsPainted;  // 1 once the seam painter ran (first effect tick)
static g_GRTimes;         // per-plr finish tick; 0 = not finished yet
static g_GRwpIdx;         // per-plr current waypoint index

// ---- Phase-B per-racer line bias (T1): each bot rolls a deterministic
// triple in GRStepSetup so the pack stops being lockstep clones.
//   [0] entryY   -10/0/+10 px vertical shift of the seam-swipe lane
//   [1] aggr      7/10/14 -> dig-assist + watchdog corridor scale /10
//   [2] gamble    1 = commit one detour waypoint at the nearest painted
//                  vein centre on the seam approach
static g_GRbias;          // per-plr [entryY, aggr, gamble]
static g_GRveinX;         // painted vein centres (x), 6 seeds
static g_GRveinY;         // painted vein centres (y)
static g_GRdetour;        // per-plr gamble state: 0 idle, 1 committed
static g_GRdetourX;       // gamble detour waypoint (x), 0 = none yet
static g_GRdetourY;       // gamble detour waypoint (y)
static g_GRcarveLog;      // per-plr GRCARVE line counter (log cap)

// RunTest phases: 0 setup, 1 race watch, 2 diff wait, 3 late canary
static g_Phase;
static g_Step;
static g_Canary;

protected func Initialize()
{
	g_GRbase = CreateArray(GR_GridW * GR_GridH);
	g_GRmat = CreateArray(GR_GridW * GR_GridH);
	g_GRTimes = CreateArray(4);
	g_GRwpIdx = CreateArray(4);
	g_GRbias = CreateArray(4);
	g_GRveinX = CreateArray(6);
	g_GRveinY = CreateArray(6);
	g_GRdetour = CreateArray(4);
	g_GRdetourX = CreateArray(4);
	g_GRdetourY = CreateArray(4);
	g_GRcarveLog = CreateArray(4);
	AddEffect("GRRace", 0, 1, 35);
	AddEffect("GRBotDriver", 0, 1, 35);
	AddEffect("RunTest", 0, 1, 35);
	return true;
}

protected func InitializePlayer(int iPlr)
{
	var pCrew = GetCrew(iPlr, 0);
	if (!pCrew) return;
	// every racer starts at the mountain-top gate, staggered along x
	SetPosition(GR_GateX + iPlr * 12, GR_GateY, pCrew);
	SelectCrew(iPlr, pCrew, 1);
	SetCursor(iPlr, pCrew);
	CreateContents(TFLN, pCrew); // one blast charge per racer (A7 toolbox)
	return true;
}

// ---- seam painter ---------------------------------------------------------
global func GRPaintSeams()
{
	// fixed gate seam through the mid-shelf pillar base
	DrawMaterialQuad("Sand", GR_SeamX1, GR_SeamY1, GR_SeamX2, GR_SeamY1, GR_SeamX2, GR_SeamY2, GR_SeamX1, GR_SeamY2);
	// six seeded vein bands: positions roll off the engine Random()
	// stream (deterministic under the pinned seed). Buried Sand veins
	// in the shelf masses are the Phase-B line-choice fodder.
	var i;
	for (i = 0; i < 6; ++i)
	{
		var vx = 150 + Random(750);
		var vy = 260 + Random(180);
		DrawMaterialQuad("Sand", vx, vy, vx + 30, vy, vx + 55, vy + 60, vx + 25, vy + 60);
		// record the painted vein centre: the gamble-flag bots route one
		// detour waypoint through the nearest one (T1 line bias)
		g_GRveinX[i] = vx + 27;  // quad corners avg ~ (vx+27, vy+30)
		g_GRveinY[i] = vy + 30;
	}
	return true;
}

// ---- heatmap telemetry ----------------------------------------------------
global func GRSolidMask(int cx, int cy)
{
	var mask = 0;
	if (GBackSolid(cx * GR_Cell + 10, cy * GR_Cell + 10)) mask += 1;
	if (GBackSolid(cx * GR_Cell + 4,  cy * GR_Cell + 16)) mask += 2;
	if (GBackSolid(cx * GR_Cell + 16, cy * GR_Cell + 4))  mask += 4;
	return mask;
}

global func GRSnapshot()
{
	var cx, cy, idx;
	for (cy = 0; cy < GR_GridH; ++cy)
		for (cx = 0; cx < GR_GridW; ++cx)
		{
			idx = cy * GR_GridW + cx;
			g_GRbase[idx] = GRSolidMask(cx, cy);
			g_GRmat[idx] = GetMaterial(cx * GR_Cell + 10, cy * GR_Cell + 10);
		}
	return true;
}

global func GREmitHeatmap()
{
	var cx, cy, idx, base, now, dug;
	dug = 0;
	for (cy = 0; cy < GR_GridH; ++cy)
		for (cx = 0; cx < GR_GridW; ++cx)
		{
			idx = cy * GR_GridW + cx;
			base = g_GRbase[idx];
			if (!base) continue;
			now = GRSolidMask(cx, cy);
			if (now < base)
			{
				++dug;
				Log(Format("GRHM:lap=1 cell=%d,%d mat=%d dug=%d", cx, cy, g_GRmat[idx], base - now));
			}
		}
	g_GRdug = dug;
	Log(Format("GRHM:lap=1 total=%d cells=%d", g_GRdug, g_GRdug));
	return true;
}

global func FxGRRaceTimer(target, effect, time)
{
	if (!g_GRseamsPainted)
	{
		GRPaintSeams();
		GRSnapshot();
		g_GRseamsPainted = 1;
		return 1;
	}
	// per-racer finish poll -> GRTM lines + first-finisher lap-end diff
	var i;
	for (i = 0; i < GetPlayerCount(); ++i)
	{
		var plr = GetPlayerByIndex(i);
		var pCursor = GetCursor(plr);
		if (!pCursor) continue;
		if (GetX(pCursor) > GR_FinishX && g_GRTimes[plr] == 0)
		{
			g_GRTimes[plr] = FrameCounter();
			Log(Format("GRTM:plr=%d finish=%d", plr, g_GRTimes[plr]));
		}
	}
	if (!g_GRlapDone)
	{
		for (i = 0; i < GetPlayerCount(); ++i)
		{
			if (g_GRTimes[GetPlayerByIndex(i)] > 0)
			{
				g_GRlapDone = 1;
				GREmitHeatmap();
				break;
			}
		}
	}
	return 1;
}

// ---- bot driver: real MoveTo commands + model-assisted digging -------------
global func FxGRBotDriverTimer(target, effect, time)
{
	var i;
	for (i = 0; i < GetPlayerCount(); ++i)
		GRDriveBot(GetPlayerByIndex(i));
	return 1;
}

// raw carve telemetry: one line per driver DigFreeRect, capped at the
// first 40 per plr (carves are few; the cap just bounds the log)
global func GRLogCarve(int plr, int x, int y, int w, int h)
{
	if (g_GRcarveLog[plr] >= 40) return true;
	g_GRcarveLog[plr] += 1;
	Log(Format("GRCARVE:plr=%d x=%d y=%d w=%d h=%d", plr, x, y, w, h));
	return true;
}

global func GRDriveBot(int plr)
{
	var pBot = GetCursor(plr);
	if (!pBot) return;
	// per-racer line bias (rolled in GRStepSetup; neutral default covers
	// the first driver tick, which can fire before the roll)
	var bias = g_GRbias[plr];
	if (!bias) bias = [0, 10, 0];
	var iWp = g_GRwpIdx[plr];
	if (iWp >= GR_WPCount) iWp = GR_WPCount - 1;  // hold at the finish press
	var iTx = GRWaypointX(iWp);
	var iTy = GRWaypointY(iWp);
	// gamble bias: on the seam approach, commit ONE detour waypoint at the
	// nearest painted vein centre before the seam (ahead of the bot, on the
	// walking band - deep-buried centres would pit the racer unrecoverably),
	// then rejoin the flock line. Deterministic: veins are already painted
	// (seams ran at t=35).
	if (g_GRseamsPainted && bias[2] == 1 && g_GRdetour[plr] == 0
	    && GetX(pBot) > 250 && GetX(pBot) < GR_SeamX1 - 20)
	{
		var j, iNearest = -1, iD, iMinD = 1000000;
		for (j = 0; j < 6; ++j)
		{
			if (g_GRveinX[j] <= GetX(pBot)) continue;        // only ahead
			if (g_GRveinX[j] >= GR_SeamX1 - 10) continue;    // before the seam
			if (g_GRveinY[j] > 345) continue;                // on the walk band
			iD = (g_GRveinX[j] - GetX(pBot)) * (g_GRveinX[j] - GetX(pBot))
			   + (g_GRveinY[j] - GetY(pBot)) * (g_GRveinY[j] - GetY(pBot));
			if (iD < iMinD) { iMinD = iD; iNearest = j; }
		}
		if (iNearest >= 0)
		{
			g_GRdetourX[plr] = g_GRveinX[iNearest];
			g_GRdetourY[plr] = g_GRveinY[iNearest];
			g_GRdetour[plr] = 1;
		}
	}
	// an active detour overrides the current waypoint target
	if (g_GRdetour[plr] == 1)
	{
		iTx = g_GRdetourX[plr];
		iTy = g_GRdetourY[plr];
	}
	// arrived? (at the detour: resume the course and catch up past any
	// waypoint the detour overshot - never walk backwards; at a
	// waypoint: advance to the next one)
	if (g_GRdetour[plr] == 1)
	{
		if (Abs(GetX(pBot) - iTx) <= 15 && Abs(GetY(pBot) - iTy) <= 30)
		{
			g_GRdetour[plr] = 2;  // detour done - never re-rolls
			while (g_GRwpIdx[plr] < GR_WPCount - 1 && GRWaypointX(g_GRwpIdx[plr]) + 15 < GetX(pBot))
				++g_GRwpIdx[plr];
			iTx = GRWaypointX(g_GRwpIdx[plr]);
			iTy = GRWaypointY(g_GRwpIdx[plr]);
		}
	}
	else if (Abs(GetX(pBot) - iTx) <= 15 && Abs(GetY(pBot) - iTy) <= 30)
	{
		if (g_GRwpIdx[plr] < GR_WPCount - 1) ++g_GRwpIdx[plr];
		iTx = GRWaypointX(g_GRwpIdx[plr]);
		iTy = GRWaypointY(g_GRwpIdx[plr]);
	}
	// tunnel carve: inside the fixed seam's x-range, clear the whole
	// band cross-section down to the shelf floor in ONE swipe. The
	// instable seam Sand would otherwise keep refilling a small carve:
	// the collapse piles up above the shelf floor and jams the racer
	// (T5 diagnosis - all four bots wedged at x 517 / y 286 against the
	// pillar, re-settled sand mound at their feet). Idempotent.
	// The lane's y-range shifts with the racer's entry bias (T1), but
	// every lane still FULLY covers the band 280..320 (8px margins):
	// a downward shift would leave the band's top sliver uncarved and
	// re-open the refill wedge, so +entry deepens into the floor below
	// the band instead and -entry lifts the lane with a compensating
	// height gain.
	if (GetX(pBot) > GR_SeamX1 - 20 && GetX(pBot) < GR_SeamX2 + 20)
	{
		var sy, sh;
		if (bias[0] >= 0)
		{
			sy = GR_SeamY1 - 8;                        // 272: band top covered
			sh = (GR_SeamY2 - GR_SeamY1 + 8) + bias[0]; // deepen into the floor
		}
		else
		{
			sy = GR_SeamY1 - 8 + bias[0];              // lift the lane
			sh = (GR_SeamY2 - GR_SeamY1 + 8) - bias[0]; // keep the floor margin
		}
		DigFreeRect(GR_SeamX1, sy, GR_SeamX2 - GR_SeamX1, sh);
		GRLogCarve(plr, GR_SeamX1, sy, GR_SeamX2 - GR_SeamX1, sh);
	}
	// dig assist: solid diggable material ahead -> carve a walk corridor
	// (tall enough for the 20px clonk shape; model-assisted digging,
	// sturmfront R7 precedent). Corridor scale = the racer's aggr/10 (T1).
	if (GBackSolid(GetX(pBot) + 15, GetY(pBot) - 8))
	{
		var aw = 34 * bias[1] / 10;
		var ah = 34 * bias[1] / 10;
		DigFreeRect(GetX(pBot) + 4, GetY(pBot) - 16, aw, ah);
		GRLogCarve(plr, GetX(pBot) + 4, GetY(pBot) - 16, aw, ah);
	}
	// stuck watchdog: carve free at the bot's own position (unchanged
	// mechanism; scale follows aggr/10, T1)
	if (Stuck(pBot))
	{
		var ww = 24 * bias[1] / 10;
		var wh = 36 * bias[1] / 10;
		DigFreeRect(GetX(pBot), GetY(pBot) - 16, ww, wh);
		GRLogCarve(plr, GetX(pBot), GetY(pBot) - 16, ww, wh);
	}
	// (re-)issue the real MoveTo command (GiantSquid/JungleClonk form)
	SetCommand(pBot, "MoveTo", 0, iTx, iTy);
	return true;
}

// ---- staged asserts (ArenaBotRound pattern) --------------------------------
global func FxRunTestTimer(target, effect, time)
{
	++g_Step;
	if (g_Phase == 0) return GRStepSetup();
	if (g_Phase == 1) return GRStepRace();
	if (g_Phase == 2) return GRStepDiff();
	return GRStepCanary();
}

global func GRStepSetup()
{
	var i;
	if (GetPlayerCount() != 4)
		FatalError(Format("GrabsteinRallyeRound FAIL: expected 4 players, got %d", GetPlayerCount()));
	for (i = 0; i < 4; ++i)
	{
		var plr = GetPlayerByIndex(i);
		if (!GetCrew(plr, 0))
			FatalError(Format("GrabsteinRallyeRound FAIL: player %d has no crew", plr));
		if (!GetCursor(plr))
			FatalError(Format("GrabsteinRallyeRound FAIL: player %d has no cursor", plr));
	}
	// Phase-B T1: each racer rolls a deterministic line bias once, off the
	// pinned-seed Random stream. entryY shifts the seam-swipe lane,
	// aggr scales the peripheral dig corridors (/10), gamble 1 commits a
	// detour waypoint through the nearest painted vein centre.
	for (i = 0; i < 4; ++i)
	{
		var plr = GetPlayerByIndex(i);
		var entryY = (Random(3) - 1) * 10;  // -10, 0, +10
		var aggr = [7, 10, 14][Random(3)];  // 7/10/14 = shy/normal/heavy digging
		var gamble = Random(2);             // 0 = flock line, 1 = vein detour
		g_GRbias[plr] = [entryY, aggr, gamble];
		Log(Format("GRBIAS:plr=%d aggr=%d entry=%d gamble=%d", plr, aggr, entryY, gamble));
	}
	Log("GrabsteinRallyeRound setup: 4 racers at the gate");
	g_Phase = 1;
	return 1;
}

global func GRStepRace()
{
	var i;
	// the seam painter ran on the GRRace effect's first fire (t=35);
	// by our second fire (t=70) the fixed seam band MUST be in place.
	// Four probe points across the band: a single random vein (30px
	// wide) cannot blanket all four, so this asserts the FIXED band.
	if (g_Step == 2)
	{
		if (GetMaterial(510, 285) != Material("Sand") ||
		    GetMaterial(530, 285) != Material("Sand") ||
		    GetMaterial(510, 310) != Material("Sand") ||
		    GetMaterial(530, 310) != Material("Sand"))
			FatalError("GrabsteinRallyeRound FAIL: fixed seam band missing (probes 510/530 x 285/310)");
	}
	// deadline: a finisher within 40 steps (1400 ticks)
	if (g_Step > 40)
		FatalError(Format("GrabsteinRallyeRound FAIL: race not finished by step %d", g_Step));
	for (i = 0; i < GetPlayerCount(); ++i)
	{
		var pCursor = GetCursor(GetPlayerByIndex(i));
		if (pCursor && GetX(pCursor) > GR_FinishX)
		{
			g_Phase = 2;
			Log(Format("GrabsteinRallyeRound: leader crossed at step %d", g_Step));
			return 1;
		}
	}
	return 1;
}

global func GRStepDiff()
{
	// the GRRace effect emits the heatmap diff on its own poll cadence;
	// the diff MUST show at least one dug cell (telemetry plumbing)
	if (g_GRlapDone && g_GRdug > 0)
	{
		Log("GrabsteinRallyeRound PASS");
		g_Phase = 3;
		g_Canary = 0;
		return 1;
	}
	if (g_GRlapDone && g_GRdug <= 0)
		FatalError("GrabsteinRallyeRound FAIL: GRHM diff emitted zero dug cells");
	if (g_Step > 46) // diff never ran within ~6 steps of the crossing
		FatalError(Format("GrabsteinRallyeRound FAIL: telemetry never emitted (step %d)", g_Step));
	return 1;
}

global func GRStepCanary()
{
	++g_Canary;
	// the engine-owned end (retire ~60 + GOAL CheckTime <=250 + Wait4End
	// 30) fits inside 13 steps; the engine quits on that GameOver, so
	// reaching step 13 means the round did NOT end by itself
	if (g_Canary > 13)
		FatalError("GrabsteinRallyeRound FAIL: round did not end by itself (RACE+RVLR)");
	return 1;
}
