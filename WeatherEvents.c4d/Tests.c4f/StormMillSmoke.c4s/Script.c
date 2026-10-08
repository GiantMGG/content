/*-- StormMillSmoke.c4s -- headless C6 storm-watch leg test. --*/
/*                                                              */
/* Exercises the STRM lifecycle end-to-end in 35-tick steps     */
/* (the smoke needs REAL ticks: the STRM event's TimerCall must */
/* run and the EventDuration auto-stop must fire):              */
/*   0 fixtures   -- an AGWM windmill + a granary stand-in (a   */
/*                  second AGWM: the real GRNY def is           */
/*                  Outset-local, not loadable here --          */
/*                  documented substitution, spec §4.6; the      */
/*                  GRNY-specific leg is covered by the real-   */
/*                  round gate + playtest). Both are flammable  */
/*                  structures (ContactIncinerate=4), which is  */
/*                  what the C6 predicate discriminates on.     */
/*   1 control    -- C6 predicate reports both fixtures intact. */
/*   2 storm      -- LaunchWeatherEvent(STRM, 50, 60); active.   */
/*   3 run        -- no-op: the storm runs (raging-phase         */
/*                  lightning is live; seed 199 governs).        */
/*   4 auto-clear -- EventDuration auto-stop fired: no active   */
/*                  event, both fixtures still intact.          */
/*   5 burn leg   -- Incinerate(standIn): predicate reports it  */
/*                  lost while the mill stays intact.           */
/*   6 PASS       -- "StormMillSmoke PASS"; GameOver().         */
/*                                                              */
/* The C6 predicate is DUPLICATED here -- mirror discipline,    */
/* never imported from Outset.c4s (FrontierSmoke precedent).    */
/* On any assertion failure, FatalError produces a non-zero     */
/* exit code, failing the CTest entry.                          */

#strict 3

static g_iStep;
static g_mill;
static g_standIn;

/* Mirrored C6 predicate (duplicate of the Outset FREvalC6
   mill/granary leg): intact = exists and not on fire. */
global func Intact(object obj)
{
	return obj && !OnFire(obj);
}

global func SmokeSurfaceY(int x)
{
	var y = 10;
	while (y < LandscapeHeight() - 5 && !GBackSolid(x, y)) y++;
	return y;
}

protected func Initialize()
{
	g_iStep = 0;
	g_mill = 0;
	g_standIn = 0;
	StepFixtures();
	AddEffect("RunTest", this, 1, 35, this);
	return true;
}

global func FxRunTestTimer(target, effect, time)
{
	++g_iStep;
	if (g_iStep == 1) return StepControl();
	if (g_iStep == 2) return StepStormLaunch();
	if (g_iStep == 3) return StepStormRun();
	if (g_iStep == 4) return StepAutoClear();
	if (g_iStep == 5) return StepBurn();
	if (g_iStep == 6) return StepPass();
	return -1;
}

/* Step 0 (in Initialize): fixtures -- windmill + granary
   stand-in on a dry surface spot, well clear of map edges. */
global func StepFixtures()
{
	// scan for a dry solid-surface spot in the middle band of the map
	var gx = LandscapeWidth() / 2;
	var gy = 0;
	var tr;
	for (tr = 0; tr < 16; tr++)
	{
		gy = SmokeSurfaceY(gx);
		if (gy < 5 || gy >= LandscapeHeight() - 5) { gx += 40; continue; }
		if (GBackLiquid(gx, gy - 4)) { gx += 40; continue; }
		break;
	}
	if (!(gx < LandscapeWidth() - 60))
		FatalError("StormMillSmoke FAIL step 0: no dry surface spot found");

	g_mill = CreateObject(AGWM, gx, gy, NO_OWNER);
	if (!g_mill)
		FatalError("StormMillSmoke FAIL step 0: could not spawn windmill (AGWM)");

	g_standIn = CreateObject(AGWM, gx + 60, gy, NO_OWNER);
	if (!g_standIn)
		FatalError("StormMillSmoke FAIL step 0: could not spawn granary stand-in (AGWM)");

	Log(Format("SMSMK:fixtures mill=(%d,%d) standIn=(%d,%d)", gx, gy, gx + 60, gy));
	return true;
}

/* Step 1: C6-predicate control -- both fixtures report intact. */
global func StepControl()
{
	if (!Intact(g_mill))
		FatalError("StormMillSmoke FAIL step 1: windmill not intact");
	if (!Intact(g_standIn))
		FatalError("StormMillSmoke FAIL step 1: granary stand-in not intact");
	Log("SMSMK:step1 C6 control -- both fixtures intact");
	return true;
}

/* Step 2: launch the storm; assert it became active. */
global func StepStormLaunch()
{
	// Duration tuned for the engine's EventDuration countdown
	// (C4Weather.cpp:182-186): the decrement runs on the 34 non-wrap
	// frames of every 35 (iTick35 == 0 on frames 105/140/... skips), so
	// an event launched at ~70 with duration D clears at ~70 + D*35/34.
	// With D = 60 that is ~frame 131: inside the step 3 -> step 4 window
	// (auto-stop proven by step 4; the storm still rages over step 3).
	LaunchWeatherEvent(STRM, 50, 60);
	if (GetActiveWeatherEvent() != STRM)
		FatalError(Format("StormMillSmoke FAIL step 2: expected active event STRM got %s",
		                  C4IdText(GetActiveWeatherEvent())));
	Log("SMSMK:step2 STRM launched + active");
	return true;
}

/* Step 3: no-op -- the storm runs under the pinned seed; the
   scene is intentionally not asserted here (raging-phase
   lightning is live; seed 199 governs determinism). */
global func StepStormRun()
{
	Log("SMSMK:step3 storm running (no-op step)");
	return true;
}

/* Step 4: the EventDuration auto-stop must have fired
   (C4Weather.cpp:182-186: no active event), and the C6
   predicate must still report both fixtures intact. */
global func StepAutoClear()
{
	if (GetActiveWeatherEvent() != nil)
		FatalError(Format("StormMillSmoke FAIL step 4: event not auto-cleared, still %s",
		                  C4IdText(GetActiveWeatherEvent())));
	if (!Intact(g_mill))
		FatalError("StormMillSmoke FAIL step 4: windmill not intact after storm");
	if (!Intact(g_standIn))
		FatalError("StormMillSmoke FAIL step 4: granary stand-in not intact after storm");
	Log("SMSMK:step4 event auto-cleared; both fixtures intact");
	return true;
}

/* Step 5: burn leg -- Incinerate the stand-in; the predicate
   must now report it lost while the mill stays intact. */
global func StepBurn()
{
	if (!Intact(g_standIn))
		FatalError("StormMillSmoke FAIL step 5: stand-in not intact before burn");
	Incinerate(g_standIn);
	if (Intact(g_standIn))
		FatalError("StormMillSmoke FAIL step 5: C6 predicate did not flag the burned stand-in as lost");
	if (!Intact(g_mill))
		FatalError("StormMillSmoke FAIL step 5: C6 predicate lost the intact windmill");
	Log("SMSMK:step5 burn leg -- stand-in flagged lost, mill intact");
	return true;
}

/* Step 6: PASS. */
global func StepPass()
{
	Log("StormMillSmoke PASS");
	GameOver();
	return true;
}
