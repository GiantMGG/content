/*-- EventSmoke.c4s — headless content integration test. --*/

#strict 3

// On any assertion failure, FatalError produces a non-zero exit code,
// failing the test.

// BLZD, DRGT, STRM, HTWV, FLDD are C4ID literals (def ids in this pack).

protected func Initialize()
{
	RunSmokeSteps();
	return true;
}

func RunSmokeSteps()
{
	/* Steps 0-4: launch each weather event, assert it becomes active,
	   stop it, assert it clears. */
	var i;
	for (i = 0; i <= 4; i++)
	{
		var idToLaunch = [STRM, BLZD, DRGT, HTWV, FLDD][i];
		LaunchWeatherEvent(idToLaunch, 50, 100);
		if (GetActiveWeatherEvent() != idToLaunch)
			FatalError(Format("EventSmoke FAIL step %d: expected %s got %s",
			                  i, C4IdText(idToLaunch),
			                  C4IdText(GetActiveWeatherEvent())));
		StopWeatherEvent();
		if (GetActiveWeatherEvent() != nil)
			FatalError(Format("EventSmoke FAIL step %d: event not cleared after stop",
			                  i));
	}

	/* Step 5: crop death -- a Seedling-stage wheat under BLZD must die. */
	var pWheat = CreateObject(AGWH, 50, 30, NO_OWNER);
	if (!pWheat) FatalError("EventSmoke FAIL step 5: could not spawn wheat");
	/* Construction() calls SetAction("Seedling") but the engine forces
	   ActIdle because Con<FullCon at construction time.  Re-set the
	   action now that the object is fully constructed. */
	pWheat->SetAction("Seedling");
	LaunchWeatherEvent(BLZD, 50, 100);
	pWheat->~Grow();
	if (FindObject(AGWH)) FatalError("EventSmoke FAIL step 5: seedling wheat did not die under BLZD");
	StopWeatherEvent();

	Log("EventSmoke PASS");
	GameOver();
	return true;
}
