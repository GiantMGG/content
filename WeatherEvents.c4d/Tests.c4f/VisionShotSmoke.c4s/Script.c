/*-- VisionShotSmoke.c4s — deterministic crew placement for the capture
   E2E (cycle-108 playtest-vision-tier2). Keeps one clonk alive and inside
   the center-anchored 320x240 capture region through tick >= 200 so an
   external --screenshot-at 120 capture has stable, in-region subject
   matter; then logs the PASS marker and GameOver(). Scenario-state
   assertions only (alive + in region) — artifact correctness belongs to
   playtest_vision, not this smoke. --*/

#strict 2

static g_iStep;
static g_iMinX;
static g_iMaxX;
static g_iMinY;
static g_iMaxY;

protected func Initialize()
{
	// World = MapWidth/Height * MapZoom pixels (Task-1 probe finding:
	// 100x40x8 -> 800x320). The default --shot-size capture is 320x240,
	// center-anchored (MaxPlayer=0 -> camera_source "center"),
	// clamped to the landscape: x in [240,560], y in [40,280].
	g_iMinX = LandscapeWidth() / 2 - 160;
	g_iMaxX = LandscapeWidth() / 2 + 160;
	g_iMinY = LandscapeHeight() / 2 - 120;
	g_iMaxY = LandscapeHeight() / 2 + 120;
	// Spawn column: horizontal center of the world (Task-1 correction:
	// no fixed() call — plain int coordinates).
	var spawnX = LandscapeWidth() / 2;
	// Deterministic ground surface: first solid pixel scanning down the
	// spawn column, then spawn just above it and settle by physics.
	var spawnY = 0;
	while (spawnY < LandscapeHeight() && !GBackSolid(spawnX, spawnY))
		++spawnY;
	if (spawnY >= LandscapeHeight())
		FatalError("VisionShotSmoke FAIL step 0: no terrain surface at spawn column");
	if (!CreateObject(CLNK, spawnX, spawnY - 24, NO_OWNER))
		FatalError("VisionShotSmoke FAIL step 0: could not spawn clonk");
	g_iStep = 0;
	AddEffect("RunTest", 0, 1, 35);
	return true;
}

global func FxRunTestStart(target, effect, temp) { return 1; }

global func FxRunTestTimer(target, effect, timer)
{
	++g_iStep;
	// Scenario-state assertions at every step (ticks 35..210) — the
	// crew must stay alive and inside the capture region, bracketing
	// the tick-120 capture on both sides (105 and 140).
	var pClonk = FindObject(CLNK);
	if (!pClonk)
		FatalError(Format("VisionShotSmoke FAIL step %d: clonk is dead", g_iStep));
	var crewX = pClonk->GetX();
	var crewY = pClonk->GetY();
	if (crewX < g_iMinX || crewX > g_iMaxX || crewY < g_iMinY || crewY > g_iMaxY)
		FatalError(Format("VisionShotSmoke FAIL step %d: crew at (%d,%d) outside capture region (%d..%d x %d..%d)",
		                  g_iStep, crewX, crewY, g_iMinX, g_iMaxX, g_iMinY, g_iMaxY));
	// Track the settled position each step (also serves as the run's
	// sidecar-free coordinate verification for the monitor).
	Log(Format("VisionShotSmoke CREW: x=%d y=%d region=%d..%d x %d..%d wait=%d",
	           crewX, crewY, g_iMinX, g_iMaxX, g_iMinY, g_iMaxY, timer));
	if (timer >= 200)
	{
		Log("VisionShotSmoke PASS");
		GameOver();
		return -1;
	}
	return 1;
}
