/*-- FirstLightClimate.c4s — cycle-168 climate-behavior pin (spec               --*/
/* firstlight-visible). Plays FirstLight's exact mission [Weather] (Climate=0,0, --*/
/* StartSeason=50,0, YearSpeed=20) with a carved homestead pond + fish trap.     --*/
/*                                                                               --*/
/* Pre-fix the seasonal winter freeze (Climate=50,0) iced the pond from ~frame   --*/
/* 10640 and dried the trap before the second audit, so the ledger win (frame    --*/
/* 12600) was unreachable — measured root cause, see                             --*/
/* .opencode/scratch/168/balance/timeline.md. This driver runs PAST that window  --*/
/* and asserts the pond stays liquid through it: engine climate stays +50 (the   --*/
/* seasonal temperature floor is +20 vs the -10 Water->Ice threshold) and the    --*/
/* trap is InLiquid() with the pond water column intact at frames 11200 / 12600  --*/
/* / 14700 (through the win window + one extra audit).                            --*/
/*                                                                               --*/
/* Scenario name deliberately has NO "Smoke" suffix (LavaWallLong precedent):    --*/
/* the glob's hardcoded --smoke-run 350 would permanently RED a scenario that    --*/
/* needs 16000 frames. Registered explicitly as firstlight_climate_smoke.        --*/

#strict 2

static g_pond_gy;

protected func Initialize()
{
	SetWind(20);
	AddEffect("RunTest", 0, 1, 35, 0, 0);
	return true;
}

global func FxRunTestStart(target, effect, temp) { return 1; }

/* CarveHomesteadPond — verbatim mirror of the balance driver / FirstLight.c4s   */
/* homestead pond (dig a 80x40 pocket, pour Water, drop the trap in the column). */
global func CarveHomesteadPond(int iPx)
{
	var gy = 100;
	while (gy < 550 && !GBackSolid(iPx, gy)) gy++;
	g_pond_gy = gy;
	DigFreeRect(iPx - 40, gy - 4, 80, 40);
	var px, py, water_mat = Material("Water");
	for (py = gy + 34; py > gy - 3; py--)
		for (px = iPx - 39; px < iPx + 40; px++)
			InsertMaterial(water_mat, px, py);
	// trap at the balance-measured settled position (x=360 y=gy+26, wet from
	// ~frame 175; the fishery observable this pin protects)
	CreateObject(AGFT, iPx, gy + 26, NO_OWNER);
	return gy;
}

global func PondWaterCount()
{
	var water = 0, px, py, gy = g_pond_gy;
	for (py = gy - 4; py < gy + 38; py++)
		for (px = 360 - 39; px < 360 + 40; px++)
			if (GetMaterial(px, py) == Material("Water")) water++;
	return water;
}

global func ClimateProbe(int iFrame)
{
	var pTrap = FindObject(AGFT);
	if (!pTrap)
		FatalError(Format("FirstLightClimate FAIL: no AGFT trap at frame %d", iFrame));
	if (!pTrap->InLiquid())
		FatalError(Format("FirstLightClimate FAIL: trap dry at frame %d — pond froze", iFrame));
	var water = PondWaterCount();
	// fixed world census: 2887 water cells, constant through the win window;
	// the pre-fix freeze dropped it to 2054 by frame 12425 (Water -> Ice)
	if (water < 2500)
		FatalError(Format("FirstLightClimate FAIL: pond water %d < 2500 at frame %d — Water -> Ice", water, iFrame));
	Log(Format("FirstLightClimate probe frame=%d trap_wet water=%d", iFrame, water));
	return true;
}

global func FxRunTestTimer(target, effect, time)
{
	// world creation in the first driver step: material writes at Initialize
	// are overwritten by the landscape bake (balance-driver precedent)
	if (FrameCounter() == 35)
	{
		if (GetScenarioVal("Climate", "Weather", 0) != 0)
			FatalError("FirstLightClimate FAIL: scenario [Weather] Climate != 0,0");
		// internal climate = 100 - scenario.Climate - 50 (C4Weather.cpp Init);
		// seasonal T = Climate - 30*cos(2*pi*Season/100) has floor +20 here,
		// vs the Water->Ice BelowTempConvert = -10 (Water.c4m)
		if (GetClimate() != 50)
			FatalError(Format("FirstLightClimate FAIL: engine climate %d != 50 (Climate=0,0)", GetClimate()));
		CarveHomesteadPond(360);
	}

	if (FrameCounter() == 11200 || FrameCounter() == 12600 || FrameCounter() == 14700)
		ClimateProbe(FrameCounter());

	if (FrameCounter() == 14700)
	{
		Log("FirstLightClimate PASS");
		GameOver();
		return -1;
	}
	return 1;
}
