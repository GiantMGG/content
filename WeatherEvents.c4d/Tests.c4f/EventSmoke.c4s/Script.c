/*-- EventSmoke.c4s — headless content integration test. --*/

#strict 2

// On any assertion failure, FatalError produces a non-zero exit code,
// failing the test.

static const C4ID BLZD = C4Id("BLZD");
static const C4ID DRGT = C4Id("DRGT");
static const C4ID STRM = C4Id("STRM");
static const C4ID HTWV = C4Id("HTWV");
static const C4ID FLDD = C4Id("FLDD");

static g_iStep;

protected func Initialize()
{
	g_iStep = 0;
	// Run one test step every 35 frames (~1s).
	AddEffect("RunTest", this, 1, 35, this);
	return true;
}

func FxRunTestStart(target, effect, temp, v1, v2, v3, v4) { return 1; }

func FxRunTestTimer(object target, int effect, int timer)
{
	if (g_iStep >= 0 && g_iStep <= 4)
	{
		var idToLaunch = [STRM, BLZD, DRGT, HTWV, FLDD][g_iStep];
		LaunchWeatherEvent(idToLaunch, 50, 100);
		if (GetActiveWeatherEvent() != idToLaunch)
			FatalError(Format("EventSmoke FAIL step %d: expected %s got %s",
			                  g_iStep, C4IdText(idToLaunch),
			                  C4IdText(GetActiveWeatherEvent())));
		StopWeatherEvent();
		if (GetActiveWeatherEvent() != nil)
			FatalError(Format("EventSmoke FAIL step %d: event not cleared after stop",
			                  g_iStep));
	}
	if (g_iStep == 5)
	{
		// Crop death: a Seedling-stage wheat under BLZD must remove itself.
		var pWheat = CreateObject(AGWH, 50, 30, NO_OWNER);
		if (!pWheat) FatalError("EventSmoke FAIL step 5: could not spawn wheat");
		LaunchWeatherEvent(BLZD, 50, 100);
		pWheat->~Grow();
		if (FindObject(AGWH)) FatalError("EventSmoke FAIL step 5: seedling wheat did not die under BLZD");
		StopWeatherEvent();
	}
	if (g_iStep == 6)
	{
		Log("EventSmoke PASS");
		GameOver();
		return -1;  // kill the effect
	}
	++g_iStep;
	return 1;
}
