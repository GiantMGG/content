/*-- AutoTasksSmoke.c4s — standing-jobs scaffold (cycle 180 auto-tasks-mvp).
   Byte-identical mirror twin in AutoTasksAccept.c4s (plan R4): the
   SHARED-CORE region below is diffed by the mirror-drift check. Task 1
   scope: skeletons + fixtures only. No job assignment, no effects yet
   (Tasks 2/4/5 add them below the SHARED-CORE markers). --*/

#strict 3

// SHARED-CORE-BEGIN -----------------------------------------------------------
// Station constants (world px; provisional per spec — the surface y is
// scan-derived, everything else anchors to it).

static const AT_SAWM_X = 150;        // sawmill station
static const AT_MINT_TREE_X = 185;   // reserved mint tree (pre-marked)
static const AT_GRNY_X = 450;        // granary station
static const AT_QRRY_X = 700;        // quarry station
static const AT_OASS_X = 950;        // oasis station
static const AT_CENSUS_RADIUS = 250; // near-mill WOOD census radius (px)
static const AT_SURF_MIN = 140;      // surface scan band (world px)
static const AT_SURF_MAX = 170;
static const AT_SURF_SPREAD = 5;
static AT_FIELD_PLOTS;               // [380,410,470,520] (arrays are runtime)

// Station + crew statics (C4Aul statics are one engine-wide namespace — the
// g_at_ prefix is the cycle's collision guard).
static g_at_sawm, g_at_grny, g_at_qrry, g_at_oass;
static g_at_clonk_reap, g_at_clonk_saw, g_at_clonk_quarry;
static g_at_gy;                      // pinned surface y (GRNY station scan)

// Sturmfront SFSurfaceY shape: first solid y from 100 up.
global func ATSurfaceY(int x)
{
	var y = 100;
	while (y < 390 && !GBackSolid(x, y)) y++;
	return y;
}

// Near-mill loose-WOOD census. Context-free by construction: FindObjects with
// a Find_ID filter carries no caller position (planet/System.c4g/FindObject.c:33)
// — unlike Find_Distance, which injects this->GetX/GetY (safe only in object
// contexts like the Sickle). The census box is the metric all shipped bars
// are calibrated against.
global func ATWoodCensus()
{
	var w, n = 0;
	for (w in FindObjects(Find_ID(WOOD)))
		if (Abs(GetX(w) - GetX(g_at_sawm)) <= AT_CENSUS_RADIUS
		 && Abs(GetY(w) - GetY(g_at_sawm)) <= AT_CENSUS_RADIUS)
			n++;
	return n;
}

// Spawns + asserts every fixture. FatalError on any miss (top-level Initialize
// path => exit 1). The surface scan is the spec's color->material pin: a
// silently-fallback procedural map produces a randomized surface that fails
// the band/spread assert.
global func ATFixtures()
{
	// 1. Surface scan at the four stations.
	var i, gy, gm;
	var xs = [AT_SAWM_X, AT_GRNY_X, AT_QRRY_X, AT_OASS_X];
	var ymax = 0, ymin = 1000;
	for (i = 0; i < 4; i++)
	{
		gy = ATSurfaceY(xs[i]);
		Log(Format("ATFIX:surface x=%d y=%d", xs[i], gy));
		if (gy < AT_SURF_MIN || gy > AT_SURF_MAX)
			FatalError(Format("AutoTasksSmoke FAIL: surface y=%d at x=%d outside [%d,%d]",
			                  gy, xs[i], AT_SURF_MIN, AT_SURF_MAX));
		if (gy < ymin) ymin = gy;
		if (gy > ymax) ymax = gy;
		if (xs[i] == AT_GRNY_X) gm = gy;
	}
	if (ymax - ymin > AT_SURF_SPREAD)
		FatalError(Format("AutoTasksSmoke FAIL: surface spread %d > %d",
		                  ymax - ymin, AT_SURF_SPREAD));
	g_at_gy = gm; // pinned surface (x=450 GRNY station)

	// 2. Material pin: the plate under the quarry station is Earth.
	if (GetMaterial(700, g_at_gy + 5) != Material("Earth"))
		FatalError(Format("AutoTasksSmoke FAIL: material at (700,%d+5) is not Earth",
		                  g_at_gy));
	Log("ATFIX:mat700 ok");

	// 3. Sawmill — complete build (Pearls.c4s/Script.c:38 shape; SAWM graphic
	//    bottom at y+28, DefCore Offset=-36,-27 Height=55).
	gy = ATSurfaceY(AT_SAWM_X);
	g_at_sawm = CreateConstruction(SAWM, AT_SAWM_X, gy - 28, NO_OWNER, 100, 1);
	if (!g_at_sawm || GetCon(g_at_sawm) < 100)
		FatalError("AutoTasksSmoke FAIL: SAWM not built to con>=100");
	Log(Format("ATFIX:sawm con=%d", GetCon(g_at_sawm)));

	// 4. Forest: 10 TRE1 at x = 62 + 9*i (graphic bottom at y+35, DefCore
	//    Offset=-36,-36 Height=71 => base on the surface).
	for (i = 0; i < 10; i++)
	{
		var tree = CreateObject(TRE1, 62 + 9 * i, g_at_gy - 35, NO_OWNER);
		if (!tree) FatalError(Format("AutoTasksSmoke FAIL: forest tree %d", i));
	}

	// 5. Reserved mint tree + pre-claim (Sawmill :58 shape: the marker makes
	//    FindTreeToChop skip it, so the saw job never touches the mint fixture).
	var mint = CreateObject(TRE1, AT_MINT_TREE_X, g_at_gy - 35, NO_OWNER);
	if (!mint) FatalError("AutoTasksSmoke FAIL: mint tree");
	AddEffect("IntSawmillTreeMarker", mint, 1, 5000, g_at_sawm, 0, 0);

	// 6. Tree count pin (after both spawns — single unambiguous line).
	if (ObjectCount(TRE1) != 11)
		FatalError(Format("AutoTasksSmoke FAIL: TRE1 count %d != 11", ObjectCount(TRE1)));
	Log("ATFIX:trees n=11");

	// 7. Granary (donor geometry: bottom at y+0).
	g_at_grny = CreateObject(GRNY, AT_GRNY_X, g_at_gy, NO_OWNER);
	if (!g_at_grny) FatalError("AutoTasksSmoke FAIL: GRNY not spawned");
	Log("ATFIX:grny ok");

	// 8. Wheat: 4 ripe plots (Construction forces Seedling — re-set after
	//    full-con spawn, EventSmoke idiom).
	AT_FIELD_PLOTS = [380, 410, 470, 520];
	for (i = 0; i < GetLength(AT_FIELD_PLOTS); i++)
	{
		var wheat = CreateObject(AGWH, AT_FIELD_PLOTS[i], g_at_gy, NO_OWNER);
		if (!wheat) FatalError(Format("AutoTasksSmoke FAIL: wheat plot %d", i));
		wheat->SetAction("Ready");
		if (!wheat->IsRipe())
			FatalError(Format("AutoTasksSmoke FAIL: wheat %d not ripe", i));
	}
	Log("ATFIX:wheat n=4");

	// 9. Loose sickles at x 420/440 (drop to the surface).
	var sk1 = CreateObject(AGSK, 420, g_at_gy - 10, NO_OWNER);
	var sk2 = CreateObject(AGSK, 440, g_at_gy - 10, NO_OWNER);
	if (!sk1 || !sk2) FatalError("AutoTasksSmoke FAIL: sickles missing");
	Log("ATFIX:sickles n=2");

	// 10. Sandstone vein (40 wide x 20 deep = 800 px^2 ~ 400 extractions) then
	//     the quarry (bottom at y+0; probe (0,4) hits gy+4 = vein top —
	//     DesertSmoke's exact relative geometry).
	DrawMaterialQuad("Sandstone", 680, g_at_gy + 4, 720, g_at_gy + 4,
	                 720, g_at_gy + 24, 680, g_at_gy + 24, false);
	g_at_qrry = CreateObject(QRRY, AT_QRRY_X, g_at_gy, NO_OWNER);
	if (!g_at_qrry) FatalError("AutoTasksSmoke FAIL: QRRY not spawned");
	Log("ATFIX:qrry ok");

	// 11. Oasis (24x8, bottom at y+0; Construction carves the basin, Fill waters
	//     it — a pool the hauler must NOT wade: Task 2's MoveTo targets x=925).
	g_at_oass = CreateObject(OASS, AT_OASS_X, g_at_gy, NO_OWNER);
	if (!g_at_oass) FatalError("AutoTasksSmoke FAIL: OASS not spawned");
	Log("ATFIX:oass ok");

	// 12. Crew: one unowned CLNK per job (SpawnerSmoke.c4s/Script.c:22 shape).
	g_at_clonk_reap = CreateObject(CLNK, 400, g_at_gy - 8, NO_OWNER);
	g_at_clonk_saw = CreateObject(CLNK, 170, g_at_gy - 8, NO_OWNER);
	g_at_clonk_quarry = CreateObject(CLNK, 660, g_at_gy - 8, NO_OWNER);
	if (!g_at_clonk_reap || !g_at_clonk_saw || !g_at_clonk_quarry)
		FatalError("AutoTasksSmoke FAIL: crew not spawned");
	Log("ATFIX:clonks n=3");

	return true;
}

protected func Initialize()
{
	ATFixtures();
	return true;
}
// SHARED-CORE-END -------------------------------------------------------------
