/*-- LandscapeSeed.c4s — headless Seed=/Amplitude= override determinism pin. --*/

#strict 2

static g_matEarth, g_matWater;

protected func Initialize()
{
	// resolve material indices (the SavegameLandscapeSmoke step-0 pattern)
	g_matEarth = Material("Earth");
	g_matWater = Material("Water");
	if (g_matEarth < 0 || g_matWater < 0)
		FatalError("LandscapeSeed FAIL: Earth/Water missing from material map");
	AddEffect("RunTest", 0, 1, 35);
	return true;
}

global func FxRunTestTimer(target, effect, time)
{
	// anti-vacuity: the classic generator must have produced earth and water
	if (GetMaterialCount(g_matEarth, true) <= 0)
		FatalError("LandscapeSeed FAIL: no Earth material in landscape");
	if (GetMaterialCount(g_matWater, true) <= 0)
		FatalError("LandscapeSeed FAIL: no Water material in landscape");
	var checksum = LandscapeChecksum();
	if (checksum != 464412)
		FatalError(Format("LandscapeSeed FAIL: checksum %d does not match pinned %d", checksum, 464412));
	Log(Format("LandscapeSeed checksum: %d", checksum));
	Log("LandscapeSeed PASS");
	GameOver();
	return -1;
}

global func LandscapeChecksum()
{
	// bounded checksum over an 8x4 GetMaterial probe grid + material
	// counts (the SavegameLandscapeSmoke fingerprint pattern). Deterministic
	// order, small magnitudes — no overflow-dependent arithmetic.
	var w = LandscapeWidth(), h = LandscapeHeight();
	var sum = 0;
	for (var gx = 0; gx < 8; gx++)
		for (var gy = 0; gy < 4; gy++)
			sum = (sum * 3 + GetMaterial((gx * 2 + 1) * w / 16, (gy * 2 + 1) * h / 8) + 1) % 1000003;
	sum = (sum * 3 + GetMaterialCount(g_matEarth, true)) % 1000003;
	sum = (sum * 3 + GetMaterialCount(g_matWater, true)) % 1000003;
	return sum;
}
