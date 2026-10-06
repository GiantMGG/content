/*-- GrabsteinRallye: destructible-terrain canyon race.

  Cycle 200, roadmap grabstein-rallye-prototype - Phase A scaffold.
  RACE+RVLR wiring verbatim from Goldrace; a fixed Sand seam band plus
  six random buried Sand veins painted per race; per-lap dug-pixel
  heatmap telemetry (GRHM:/GRTM: log lines) whose Phase-B analysis
  decides kept/killed. Cup mode is Phase B.

  Telemetry vocabulary (play scenario and smoke twin log the SAME
  tokens - FirstLight mirror discipline):
    GRHM:lap=<n> total=<d> cells=<d>               lap-end summary
    GRHM:lap=<n> cell=<cx>,<cy> mat=<m> dug=<k>    one line per dug cell
    GRTM:plr=<p> finish=<ticks>                    per-racer finish tick

  Material writes only ever happen in effect timers (first fire t=35):
  Initialize-time material writes are clobbered by the landscape bake
  (FirstLightClimate lesson, rules/engine-behavior-gotchas.md). --*/

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

static g_GRbase;          // per-cell solid-sample bitmask at lap start
static g_GRmat;           // per-cell baseline material (cell-centre sample)
static g_GRdug;           // dug cells at the last lap-end diff
static g_GRlapDone;       // 1 once the leader crossed and the diff ran
static g_GRseamsPainted;  // 1 once the seam painter ran (first effect tick)
static g_GRTimes;         // per-plr finish tick; 0 = not finished yet

protected func Initialize()
{
	g_GRbase = CreateArray(GR_GridW * GR_GridH);
	g_GRmat = CreateArray(GR_GridW * GR_GridH);
	g_GRTimes = CreateArray(4);
	AddEffect("GRRace", 0, 1, 35);
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
