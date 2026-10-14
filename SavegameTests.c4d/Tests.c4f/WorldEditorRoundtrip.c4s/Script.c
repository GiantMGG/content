/*-- WorldEditorRoundtrip.c4s - world-editor save round-trip gate fixture. --*/
/* Pins: paint (EMDrawTool) -> C4Console::SaveGame(false) -> reload -> the  */
/* painted pixels persist (spec worldeditor-save-roundtrip). The engine     */
/* flag --smoke-scenario-save-at:<tick> (fired by the driver, phase A)      */
/* triggers the pinned console scenario save at the given tick.             */
/*                                                                         */
/* Phase-aware script, one file, two behaviors (spec risk 2 / correction 3: */
/* a reopened saved scenario is NOT a pristine open - Game.txt restores     */
/* global effects AND Initialize re-runs):                                  */
/*   pristine run: Initialize sees sky at MARK1 -> arms the paint ladder    */
/*                 (steps 1-3: Rock at MARK1, Gold at MARK2, Earth-negative */
/*                 probe), then idles while the engine flag saves at t210.  */
/*   reload run:   Initialize sees painted Rock at MARK1 (live probe =      */
/*                 ground truth) -> AssertPersistedPaint() -> PASS ->      */
/*                 GameOver(), all inside Initialize, before any restored  */
/*                 effect can interfere.                                    */
/* Restored-effect hygiene: FxRunTestTimer starts with                      */
/* if (g_fReloaded) return -1; so a stale ladder effect restored from the  */
/* saved Game.txt self-terminates on its first tick instead of              */
/* double-driving g_iStep.                                                  */

#strict 2

static g_iStep, g_fReloaded;
static g_matRock, g_matEarth, g_matGold;

protected func Initialize()
{
	g_matRock = Material("Rock");
	g_matEarth = Material("Earth");
	g_matGold = Material("Gold");
	if (g_matRock == -1 || g_matEarth == -1 || g_matGold == -1)
		FatalError("WorldEditorRoundtrip FAIL: material lookup failed");
	g_fReloaded = (GetMaterial(200, 100) == g_matRock);   // live probe = ground truth
	g_iStep = 0;
	if (g_fReloaded)
	{
		AssertPersistedPaint();        // reload run: assert NOW, in Initialize
		Log("WorldEditorRoundtrip PASS");
		GameOver();
	}
	else
	{
		AddEffect("RunTest", 0, 1, 35, 0, 0);  // pristine run: paint ladder
	}
	return true;
}

global func AssertPersistedPaint()
{
	if (GetMaterial(200, 100) != g_matRock)
		FatalError(Format("WorldEditorRoundtrip FAIL reload MARK1 (200,100)=%d expected Rock=%d", GetMaterial(200, 100), g_matRock));
	if (GetMaterial(700, 100) != g_matGold)
		FatalError(Format("WorldEditorRoundtrip FAIL reload MARK2 (700,100)=%d expected Gold=%d", GetMaterial(700, 100), g_matGold));
	if (GetMaterial(400, 200) != g_matEarth)
		FatalError(Format("WorldEditorRoundtrip FAIL reload NEG (400,200)=%d expected Earth=%d", GetMaterial(400, 200), g_matEarth));
	Log("WorldEditorRoundtrip reload: MARK1=Rock MARK2=Gold NEG=Earth");
	return true;
}

global func FxRunTestStart(target, effect, temp) { return 1; }

global func FxRunTestTimer(object target, int effect, int timer)
{
	if (g_fReloaded) return -1;   // restored-effect hygiene (spec risk 2)
	++g_iStep;
	if (g_iStep == 1)
	{
		EMDrawTool(1, 200, 100, 0, 0, 30, 1, "Rock", "Rock", -1);
		if (GetMaterial(200, 100) != g_matRock)
			FatalError(Format("WorldEditorRoundtrip FAIL step 1: MARK1 (200,100)=%d expected Rock=%d", GetMaterial(200, 100), g_matRock));
		Log("WorldEditorRoundtrip step 1: Brush painted Rock at MARK1 (200,100)");
	}
	else if (g_iStep == 2)
	{
		/* Static TexMap pair (45=Gold-Rough in Material.c4g/TEXMAP.TXT);
		   a dynamic Gold-Gold AddEntry fails Init (no Gold texture png),
		   so the brush would silently no-op (cycle-230 T2 finding). */
		EMDrawTool(1, 700, 100, 0, 0, 30, 1, "Gold", "Rough", -1);
		if (GetMaterial(700, 100) != g_matGold)
			FatalError(Format("WorldEditorRoundtrip FAIL step 2: MARK2 (700,100)=%d expected Gold=%d", GetMaterial(700, 100), g_matGold));
		Log("WorldEditorRoundtrip step 2: Brush painted Gold at MARK2 (700,100)");
	}
	else if (g_iStep == 3)
	{
		if (GetMaterial(400, 200) != g_matEarth)
			FatalError(Format("WorldEditorRoundtrip FAIL step 3: NEG (400,200)=%d expected Earth=%d", GetMaterial(400, 200), g_matEarth));
		Log("WorldEditorRoundtrip step 3: unpainted interior (400,200) is Earth");
		Log("WorldEditorRoundtrip painted; awaiting engine save");
	}
	return 1;
}
