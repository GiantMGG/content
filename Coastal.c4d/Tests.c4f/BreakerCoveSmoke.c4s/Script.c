/*-- BreakerCoveSmoke.c4s -- 4-leg fog+beacon smoke for the C1 Beacon */
/* round (cycle 217, frontier-cove). Mirror discipline (FrontierSmoke  */
/* precedent): the Cove's predicates are DUPLICATED here, never       */
/* imported from Worlds.c4f/BreakerCove.c4s.                          */
/*                                                                    */
/* Legs (spec frontier-cove 6.1), one per RunTest beat:               */
/*   1 dark-start -- spawn LHGT, mirrored LocalN flip, IsLit()==0;    */
/*   2 negative   -- compressed FOGG window (50, 35), active; after   */
/*                   auto-stop the mirrored verdict on the dark      */
/*                   tower reports lost;                              */
/*   3 kindling   -- CLNK carrying WOOD within 45 px, mirrored        */
/*                   kindling rule fires: IsLit()==1, WOOD consumed;  */
/*   4 relight    -- second compressed FOGG window; at its end the    */
/*                   mirrored verdict reports won.                    */
/*                                                                    */
/* Beat arithmetic (StormMillSmoke precedent): a duration-D event     */
/* launched at frame T auto-stops at ~T + D*35/34 (the EventDuration  */
/* decrement skips the Tick35 wrap frames). Duration 35 launched at a */
/* beat clears ~1 frame AFTER the next beat -- so every verdict beat  */
/* sits one full beat past the next: fog #1 launched at step 2 (t=70, */
/* clears ~106) is verdict-safe at step 4 (t=140); fog #2 launched at */
/* step 6 (t=210, clears ~246) at step 8 (t=280). 8 steps x 35 = 280 */
/* of the 350 budget.                                                 */

#strict 3

static g_iStep;
static g_tower;   // the smoke's dark-start lighthouse fixture

// Mirrored constants (BreakerCove.c4s/Script.c: BC_RELIGHT_RADIUS).
static const BCSMK_RELIGHT_RADIUS = 45;

protected func Initialize()
{
	g_iStep = 0;
	g_tower = 0;
	AddEffect("RunTest", this, 1, 35, this);
	return true;
}

global func FxRunTestTimer(target, effect, time)
{
	++g_iStep;
	if (g_iStep == 1) return StepDarkStart();
	if (g_iStep == 2) return StepFogOne();
	if (g_iStep == 3) return StepFogOneRun();
	if (g_iStep == 4) return StepNegativeVerdict();
	if (g_iStep == 5) return StepKindling();
	if (g_iStep == 6) return StepFogTwo();
	if (g_iStep == 7) return StepFogTwoRun();
	if (g_iStep == 8) return StepRelightVerdict();
	return -1;
}

/* Mirrored surface scan (StormMillSmoke SmokeSurfaceY / BreakerCove
   BCSurfaceY). */
global func BcsmkSurfaceY(int x)
{
	var y = 10;
	while (y < LandscapeHeight() - 5 && !GBackSolid(x, y)) y++;
	return y;
}

/* Mirrored kindling rule (duplicate of BreakerCove.c4s BCKindleCheck):
   while the tower is dark, any CLNK carrying WOOD within the relight
   radius burns one WOOD and lights the beacon. */
global func BcsmkKindleCheck()
{
	if (!g_tower || LocalN("bLit", g_tower)) return false;
	var pClonk, pWood;
	for (pClonk in FindObjects(Find_ID(CLNK)))
	{
		if (Distance(GetX(pClonk), GetY(pClonk), GetX(g_tower), GetY(g_tower)) > BCSMK_RELIGHT_RADIUS)
			continue;
		pWood = FindContents(WOOD, pClonk);
		if (!pWood) continue;
		RemoveObject(pWood);
		LocalN("bLit", g_tower) = 1;
		return true;
	}
	return false;
}

/* Mirrored C1 verdict (duplicate of BreakerCove.c4s BCEvalC1): a lit
   LHGT exists at fog end -> won, else lost. */
global func BcsmkVerdict()
{
	var lit = 0;
	var pB;
	for (pB in FindObjects(Find_ID(LHGT), Find_Func("IsLit")))
		lit = 1;
	if (lit) return "won";
	return "lost";
}

/* Leg 1 -- dark-start: spawn LHGT (default lit, the bLit=1 anchor),
   flip it dark the Cove's way, assert IsLit()==0. */
global func StepDarkStart()
{
	// Dry surface spot (StormMillSmoke scan pattern).
	var gx = LandscapeWidth() / 2;
	var gy = 0;
	var tr;
	for (tr = 0; tr < 16; tr++)
	{
		gy = BcsmkSurfaceY(gx);
		if (gy < 5 || gy >= LandscapeHeight() - 5) { gx += 40; continue; }
		if (GBackLiquid(gx, gy - 4)) { gx += 40; continue; }
		break;
	}
	if (!(gx < LandscapeWidth() - 60))
		FatalError("BreakerCoveSmoke FAIL step 1: no dry surface spot found");

	g_tower = CreateObject(LHGT, gx, gy - 20, NO_OWNER);
	if (!g_tower)
		FatalError("BreakerCoveSmoke FAIL step 1: could not spawn lighthouse (LHGT)");
	if (!g_tower->IsLit())
		FatalError("BreakerCoveSmoke FAIL step 1: LHGT default not lit (bLit=1 anchor)");

	// Mirrored dark-start flip (BreakerCove.c4s BCStampCove).
	LocalN("bLit", g_tower) = 0;
	if (g_tower->IsLit())
		FatalError("BreakerCoveSmoke FAIL step 1: dark-start flip did not dark the tower");
	Log("BCSMK:step1 dark-start -- tower dark");
	return true;
}

/* Leg 2 -- the negative window: compressed FOGG (50, 35) active. */
global func StepFogOne()
{
	LaunchWeatherEvent(FOGG, 50, 35);
	if (GetActiveWeatherEvent() != FOGG)
		FatalError(Format("BreakerCoveSmoke FAIL step 2: expected active event FOGG got %s",
		                  C4IdText(GetActiveWeatherEvent())));
	Log("BCSMK:step2 FOGG launched + active");
	return true;
}

/* No-op beat: the duration-35 window launched at t=70 auto-stops at
   ~t=106 (C4Weather.cpp:181-186); the verdict beat (t=140) sits safely
   past it. */
global func StepFogOneRun()
{
	Log("BCSMK:step3 fog window running down");
	return true;
}

/* Leg 2 verdict: window auto-stopped; the mirrored C1 verdict on the
   dark tower reports lost. */
global func StepNegativeVerdict()
{
	if (GetActiveWeatherEvent() != nil)
		FatalError(Format("BreakerCoveSmoke FAIL step 4: fog #1 not auto-stopped, still %s",
		                  C4IdText(GetActiveWeatherEvent())));
	if (BcsmkVerdict() != "lost")
		FatalError("BreakerCoveSmoke FAIL step 4: dark tower must verdict lost");
	Log("BCSMK:step4 negative verdict -- dark tower lost");
	return true;
}

/* Leg 3 -- kindling: a CLNK carrying WOOD inside the relight radius
   lights the dark tower; the WOOD is consumed. */
global func StepKindling()
{
	if (g_tower->IsLit())
		FatalError("BreakerCoveSmoke FAIL step 5: tower already lit before kindling");

	// Negative control: an empty clonk in radius must not light it.
	var pEmpty = CreateObject(CLNK, GetX(g_tower) + 30, GetY(g_tower) + 10, NO_OWNER);
	if (!pEmpty)
		FatalError("BreakerCoveSmoke FAIL step 5: could not spawn empty clonk");
	BcsmkKindleCheck();
	if (g_tower->IsLit())
		FatalError("BreakerCoveSmoke FAIL step 5: kindling fired without WOOD");

	// Positive: the wood-carrying clonk in radius lights it; one WOOD burnt.
	var pClonk = CreateObject(CLNK, GetX(g_tower) - 30, GetY(g_tower) + 10, NO_OWNER);
	if (!pClonk)
		FatalError("BreakerCoveSmoke FAIL step 5: could not spawn clonk");
	CreateContents(WOOD, pClonk, 1);
	if (!BcsmkKindleCheck())
		FatalError("BreakerCoveSmoke FAIL step 5: mirrored kindling rule did not fire");
	if (!g_tower->IsLit())
		FatalError("BreakerCoveSmoke FAIL step 5: kindled tower not lit");
	if (FindContents(WOOD, pClonk))
		FatalError("BreakerCoveSmoke FAIL step 5: kindling WOOD not consumed");
	Log("BCSMK:step5 kindling -- tower lit, WOOD consumed");
	return true;
}

/* Leg 4 -- the relight window: second compressed FOGG, now over a lit
   tower (the Cove's endgame state). */
global func StepFogTwo()
{
	LaunchWeatherEvent(FOGG, 50, 35);
	if (GetActiveWeatherEvent() != FOGG)
		FatalError(Format("BreakerCoveSmoke FAIL step 6: expected active event FOGG got %s",
		                  C4IdText(GetActiveWeatherEvent())));
	Log("BCSMK:step6 second FOGG launched + active");
	return true;
}

/* No-op beat: fog #2 (launched t=210) auto-stops at ~t=246; the
   verdict beat (t=280) sits safely past it. */
global func StepFogTwoRun()
{
	Log("BCSMK:step7 fog window running down");
	return true;
}

/* Leg 4 verdict: window auto-stopped; the lit tower verdicts won.
   PASS + GameOver. */
global func StepRelightVerdict()
{
	if (GetActiveWeatherEvent() != nil)
		FatalError(Format("BreakerCoveSmoke FAIL step 8: fog #2 not auto-stopped, still %s",
		                  C4IdText(GetActiveWeatherEvent())));
	if (!g_tower->IsLit())
		FatalError("BreakerCoveSmoke FAIL step 8: tower lost its light before verdict");
	if (BcsmkVerdict() != "won")
		FatalError("BreakerCoveSmoke FAIL step 8: lit tower must verdict won");
	Log("BreakerCoveSmoke PASS");
	GameOver();
	return true;
}
