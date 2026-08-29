/*-- SavegameLandscapeSmoke.c4s - landscape save/load fidelity smoke. --*/

#strict 2

// Landscape-fidelity characterization of the rollback round-trip
// (C4GameSave::SaveRuntimeDataToBuffer -> LoadRuntimeDataFromBuffer),
// pinning the cycle-52 fix surface: the InitMaterialTexture loop
// (C4GameSave.cpp:859-872) must run before the Landscape.Init loop
// (:874-892), or restored sections crash Mat2Pal / fail to re-save.
//
// Execution model (critical): RollbackLoadState() replaces
// Game.Sections and destroys the old sections, so the running frame's
// section pointer goes stale. Post-restore landscape queries therefore
// run only in Script<N> callbacks, which the engine calls in the
// restored first active section (C4GameScriptHost::Execute).
// ScriptGo(true) is armed BEFORE the first save so Go/Counter
// round-trip through the savegame's Game.txt [Script] section and the
// ScriptN chain survives every restore (Counter chains Script0 ->
// Script1 -> ... across restores).
//
// Frame budget: ScriptN fires every 10 ticks (Script0 at tick 10,
// Script1 at 20, Script2 at 30), but a scenario with no players
// (MaxPlayer=0) is auto-ended by C4Game::GameOverCheck at the first
// Tick35 wrap (tick 35). The chain therefore stops at Script2: the
// fourth round-trip (RT3b) runs in Script2 and its save/load success
// is asserted there, while its landscape fidelity is covered by the
// identical RT1/RT2/RT3a verifies. PASS + GameOver() fire in Script2
// at tick 30, before the auto-game-over.
//
// Fingerprint layout (35 entries):
//   [gx*4+gy], gx 0..7, gy 0..3: GetMaterial at the interior 8x4 grid
//     point x=(gx*2+1)*W/16, y=(gy*2+1)*H/8
//   [32] GetMaterialCount(Earth,  true)  (raw pixel counts)
//   [33] GetMaterialCount(Tunnel, true)
//   [34] GetMaterialCount(Granite, true)

static g_aFingerprint;
static g_matEarth, g_matTunnel, g_matGranite;

// Mutation geometry (1000x400 map from MapWidth/Zoom x MapHeight/Zoom):
// both shapes sit in the deep-Earth body and cover fingerprint grid
// points (437,350) and (687,350).
static const SLS_DIG_X = 420;
static const SLS_DIG_Y = 340;
static const SLS_DIG_WDT = 40;
static const SLS_DIG_HGT = 30;
static const SLS_GRX1 = 660;
static const SLS_GRY1 = 340;
static const SLS_GRX2 = 720;
static const SLS_GRY2 = 370;

protected func Initialize()
{
	// Step 0: resolve material indices (Earth/Tunnel/Granite come from
	// Material.c4g, symlinked into build/ by tests/CMakeLists.txt).
	g_matEarth = Material("Earth");
	g_matTunnel = Material("Tunnel");
	g_matGranite = Material("Granite");
	if (g_matEarth < 0 || g_matTunnel < 0 || g_matGranite < 0)
		FatalError("SavegameLandscapeSmoke FAIL step 0: Earth/Tunnel/Granite missing from material map");

	// Step 1: pre-state fingerprint.
	g_aFingerprint = LandscapeFingerprint();
	if (g_aFingerprint[32] <= 0)
		FatalError("SavegameLandscapeSmoke FAIL step 1: landscape has no Earth pixels (bad map?)");

	// Arm the Script<N> driver BEFORE saving: Go/Counter are compiled
	// into the savegame's Game.txt [Script] section, so the driver
	// survives every restore and Script0 fires in the restored game.
	ScriptGo(true);

	// Step 2: save state.
	if (!RollbackSaveState())
		FatalError("SavegameLandscapeSmoke FAIL step 2: RollbackSaveState failed");

	// Step 3: mutate + anti-vacuity. The fingerprint MUST change, or
	// the restore assertions would pass vacuously.
	DigFreeRect(SLS_DIG_X, SLS_DIG_Y, SLS_DIG_WDT, SLS_DIG_HGT);
	if (!DrawMaterialQuad("Granite", SLS_GRX1, SLS_GRY1, SLS_GRX2, SLS_GRY1, SLS_GRX2, SLS_GRY2, SLS_GRX1, SLS_GRY2))
		FatalError("SavegameLandscapeSmoke FAIL step 3: DrawMaterialQuad(\"Granite\") failed");

	var fm = LandscapeFingerprint();
	if (FingerprintsEqual(fm, g_aFingerprint))
		FatalError("SavegameLandscapeSmoke FAIL step 3: mutation did not change the fingerprint (vacuous test)");
	if (fm[33] <= g_aFingerprint[33])
		FatalError("SavegameLandscapeSmoke FAIL step 3: DigFreeRect created no Tunnel pixels");
	if (fm[34] <= g_aFingerprint[34])
		FatalError("SavegameLandscapeSmoke FAIL step 3: Granite pixel count did not increase");
	if (fm[32] >= g_aFingerprint[32])
		FatalError("SavegameLandscapeSmoke FAIL step 3: Earth pixel count did not decrease");
	if (GetMaterial(437, 350) != g_matTunnel)
		FatalError("SavegameLandscapeSmoke FAIL step 3: dug rect not observable as Tunnel");
	if (GetMaterial(687, 350) != g_matGranite)
		FatalError("SavegameLandscapeSmoke FAIL step 3: Granite quad not observable via GetMaterial");

	// Step 4: restore #1. From here to the end of this frame, no
	// landscape queries: the frame's section pointer is stale now.
	if (!RollbackLoadState())
		FatalError("SavegameLandscapeSmoke FAIL step 4: RollbackLoadState failed");
	Log("SavegameLandscapeSmoke step 4: first restore OK");
	return true;
}

// RT1 verification + RT2 (second round-trip).
func Script0()
{
	// Step 5a: exact fingerprint restoration.
	AssertFingerprintRestored();

	// Step 5b: texture validity at material-bearing grid points (the
	// InitMaterialTexture contract).
	CheckTextureValidity();

	// Step 6 (RT2): save -> load; re-asserted in Script1.
	if (!RollbackSaveState())
		FatalError("SavegameLandscapeSmoke FAIL step 6a: second RollbackSaveState failed");
	if (!RollbackLoadState())
		FatalError("SavegameLandscapeSmoke FAIL step 6b: second RollbackLoadState failed");
	Log("SavegameLandscapeSmoke step 6: second round-trip OK");
	return true;
}

// RT2 verification + RT3 part 1.
func Script1()
{
	AssertFingerprintRestored();
	if (!RollbackSaveState())
		FatalError("SavegameLandscapeSmoke FAIL step 7a: third RollbackSaveState failed");
	if (!RollbackLoadState())
		FatalError("SavegameLandscapeSmoke FAIL step 7b: third RollbackLoadState failed");
	Log("SavegameLandscapeSmoke step 7: third round-trip part 1 OK");
	return true;
}

// RT3 part 1 verification + RT3 part 2 (idempotence of the buffer)
// + final success. PASS and GameOver() must fire here (tick 30):
// Script3 would fire at tick 40, after the no-player auto-game-over
// at the first Tick35 wrap (tick 35) has already ended the game.
func Script2()
{
	AssertFingerprintRestored();
	if (!RollbackSaveState())
		FatalError("SavegameLandscapeSmoke FAIL step 7c: fourth RollbackSaveState failed");
	if (!RollbackLoadState())
		FatalError("SavegameLandscapeSmoke FAIL step 7d: fourth RollbackLoadState failed");
	Log("SavegameLandscapeSmoke step 7: third round-trip part 2 OK");
	Log("SavegameLandscapeSmoke PASS");
	GameOver();
	return true;
}

func LandscapeFingerprint()
{
	var w = LandscapeWidth(), h = LandscapeHeight();
	var fp = CreateArray(35);
	for (var gx = 0; gx < 8; gx++)
		for (var gy = 0; gy < 4; gy++)
			fp[gx*4+gy] = GetMaterial((gx*2+1)*w/16, (gy*2+1)*h/8);
	fp[32] = GetMaterialCount(g_matEarth, true);
	fp[33] = GetMaterialCount(g_matTunnel, true);
	fp[34] = GetMaterialCount(g_matGranite, true);
	return fp;
}

func FingerprintsEqual(a, b)
{
	return FingerprintFirstDiff(a, b) < 0;
}

func FingerprintFirstDiff(a, b)
{
	for (var i = 0; i < GetLength(a); i++)
		if (a[i] != b[i]) return i;
	return -1;
}

func AssertFingerprintRestored()
{
	var f = LandscapeFingerprint();
	var diff = FingerprintFirstDiff(f, g_aFingerprint);
	if (diff >= 0)
		FatalError(Format("SavegameLandscapeSmoke FAIL: fingerprint drift after restore (index %d: %d != %d)",
		                  diff, f[diff], g_aFingerprint[diff]));
}

func CheckTextureValidity()
{
	var w = LandscapeWidth(), h = LandscapeHeight();
	for (var gx = 0; gx < 8; gx++)
		for (var gy = 0; gy < 4; gy++)
		{
			var x = (gx*2+1)*w/16, y = (gy*2+1)*h/8;
			if (GetMaterial(x, y) >= 0 && !GetTexture(x, y))
				FatalError(Format("SavegameLandscapeSmoke FAIL: GetTexture(%d,%d) nil at material-bearing point", x, y));
		}
}
