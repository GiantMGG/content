/*-- ScorpionArtSmoke.c4s — cycle-125 24x14 six-phase re-pin (spec    --*/
/*-- scorpion-scale-animation-fix) + cycle-145 live phase-progression --*/
/*-- leg. Synchronous RunSmokeSteps() for the wiring pins (DesertSmoke/--*/
/*-- ScorpionStagingSmoke norm); the walk-gait leg runs on a GLOBAL    --*/
/*-- effect timer (the one timer pattern that fires reliably in this   --*/
/*-- smoke harness — DuneBurialSmoke precedent) and drains to          --*/
/*-- "ScorpionArtSmoke PASS" + GameOver only after ALL legs pass.      --*/
/*-- Any missed assertion calls FatalError; inside a timer that logs   --*/
/*-- "[error] User error: ..." so the CTest FAIL regex bites even      --*/
/*-- though the process exits 0 (engine-behavior-gotchas contract).    --*/
#strict 3

static g_walker;
static g_iPhaseLegTick;
static g_iPhaseMask;
static g_iPhaseLegStage;   /* 0 = walk right, 1 = walk left; effect removed after stage 1 completes */

protected func Initialize()
{
	RunSmokeSteps();
	StartPhaseWalkLeg();
	if (g_walker)
		AddEffect("PhaseWalk", nil, 1, 1, nil, nil);
	return true;
}

func RunSmokeSteps()
{
	/* Step 0: def loads + Picture rect is the walk phase-0 sprite. */
	var scorp = CreateObject(SCRP, 50, 30, NO_OWNER);
	if (!scorp)
		FatalError("ScorpionArtSmoke FAIL step 0: SCRP not spawned");
	if (GetDefCoreVal("Picture", "DefCore", SCRP, 0) != 0
	 || GetDefCoreVal("Picture", "DefCore", SCRP, 1) != 0
	 || GetDefCoreVal("Picture", "DefCore", SCRP, 2) != 24
	 || GetDefCoreVal("Picture", "DefCore", SCRP, 3) != 14)
		FatalError("ScorpionArtSmoke FAIL step 0: Picture mismatch");

	/* Step 1: Walk wiring — Length=6, Delay=2, facet + tx/ty. */
	if (GetActMapVal("Length", "Walk", SCRP) != 6)
		FatalError("ScorpionArtSmoke FAIL step 1: Walk Length != 6");
	if (GetActMapVal("Directions", "Walk", SCRP) != 2)
		FatalError("ScorpionArtSmoke FAIL step 1: Walk Directions != 2");
	if (GetActMapVal("FlipDir", "Walk", SCRP) != 1)
		FatalError("ScorpionArtSmoke FAIL step 1: Walk FlipDir != 1");
	if (GetActMapVal("Delay", "Walk", SCRP) != 2)
		FatalError("ScorpionArtSmoke FAIL step 1: Walk Delay != 2");
	if (GetActMapVal("Facet", "Walk", SCRP, 0) != 0
	 || GetActMapVal("Facet", "Walk", SCRP, 1) != 0
	 || GetActMapVal("Facet", "Walk", SCRP, 2) != 24
	 || GetActMapVal("Facet", "Walk", SCRP, 3) != 14
	 || GetActMapVal("Facet", "Walk", SCRP, 4) != -2
	 || GetActMapVal("Facet", "Walk", SCRP, 5) != -2)
		FatalError("ScorpionArtSmoke FAIL step 1: Walk Facet mismatch");

	/* Step 1b: per-action row bands + Length. */
	if (GetActMapVal("Facet", "Turn", SCRP, 0) != 0
	 || GetActMapVal("Facet", "Turn", SCRP, 1) != 14
	 || GetActMapVal("Facet", "Turn", SCRP, 2) != 24
	 || GetActMapVal("Facet", "Turn", SCRP, 3) != 14
	 || GetActMapVal("Facet", "Turn", SCRP, 4) != -2
	 || GetActMapVal("Facet", "Turn", SCRP, 5) != -2)
		FatalError("ScorpionArtSmoke FAIL step 1b: Turn Facet mismatch");
	if (GetActMapVal("Length", "Turn", SCRP) != 2)
		FatalError("ScorpionArtSmoke FAIL step 1b: Turn Length != 2");
	if (GetActMapVal("Facet", "Jump", SCRP, 0) != 0
	 || GetActMapVal("Facet", "Jump", SCRP, 1) != 28
	 || GetActMapVal("Facet", "Jump", SCRP, 2) != 24
	 || GetActMapVal("Facet", "Jump", SCRP, 3) != 14
	 || GetActMapVal("Facet", "Jump", SCRP, 4) != -2
	 || GetActMapVal("Facet", "Jump", SCRP, 5) != -2)
		FatalError("ScorpionArtSmoke FAIL step 1b: Jump Facet mismatch");
	if (GetActMapVal("Length", "Jump", SCRP) != 2)
		FatalError("ScorpionArtSmoke FAIL step 1b: Jump Length != 2");
	if (GetActMapVal("Facet", "Tumble", SCRP, 0) != 0
	 || GetActMapVal("Facet", "Tumble", SCRP, 1) != 42
	 || GetActMapVal("Facet", "Tumble", SCRP, 2) != 24
	 || GetActMapVal("Facet", "Tumble", SCRP, 3) != 14
	 || GetActMapVal("Facet", "Tumble", SCRP, 4) != -2
	 || GetActMapVal("Facet", "Tumble", SCRP, 5) != -2)
		FatalError("ScorpionArtSmoke FAIL step 1b: Tumble Facet mismatch");
	if (GetActMapVal("Length", "Tumble", SCRP) != 2)
		FatalError("ScorpionArtSmoke FAIL step 1b: Tumble Length != 2");
	if (GetActMapVal("Facet", "Swim", SCRP, 0) != 0
	 || GetActMapVal("Facet", "Swim", SCRP, 1) != 56
	 || GetActMapVal("Facet", "Swim", SCRP, 2) != 24
	 || GetActMapVal("Facet", "Swim", SCRP, 3) != 14
	 || GetActMapVal("Facet", "Swim", SCRP, 4) != -2
	 || GetActMapVal("Facet", "Swim", SCRP, 5) != -2)
		FatalError("ScorpionArtSmoke FAIL step 1b: Swim Facet mismatch");
	if (GetActMapVal("Length", "Swim", SCRP) != 2)
		FatalError("ScorpionArtSmoke FAIL step 1b: Swim Length != 2");

	/* Step 2: phase round-trip 0-5, both dirs. */
	scorp->SetAction("Walk");
	var d = 0;
	while (d < 2)
	{
		scorp->SetDir(d);
		scorp->SetAction("Walk");   /* SetDir fired TurnAction=Turn; re-enter Walk */
		var p = 0;
		while (p < 6)
		{
			scorp->SetPhase(p);
			if (scorp->GetPhase() != p)
				FatalError("ScorpionArtSmoke FAIL step 2: phase round-trip");
			p++;
		}
		d++;
	}

	return true;
}

/* Step 3 (cycle 145): live phase-progression leg. A fresh scorpion is
   sent walking via SetComDir and the engine's own phase clock is sampled
   every tick; the assertion is that the live Walk action visits all six
   phase values in BOTH directions. The walker's WLF_HuntPrey AI is
   dropped first so nothing else steers it.
   Determinism: the scenario runs on the DEFAULT flat map (no Dunes
   Landscape.txt — DuneBurialSmoke precedent), whose surface is
   seed-stable, and a flat Sandstone platform is painted in the
   guaranteed sky band (top row y=150, sky above it on every seed). */
func StartPhaseWalkLeg()
{
	DrawMaterialQuad("Sandstone", 100, 150, 900, 150, 900, 190, 100, 190, false);
	if (GetMaterial(250, 150) != Material("Sandstone"))
		FatalError("ScorpionArtSmoke FAIL step 3: platform top not Sandstone");
	if (GetMaterial(250, 149) != -1)
		FatalError("ScorpionArtSmoke FAIL step 3: sky above platform not open");
	g_walker = CreateObject(SCRP, 250, 151, NO_OWNER);
	if (!g_walker)
		FatalError("ScorpionArtSmoke FAIL step 3: walker not spawned");
	if (!g_walker->RemoveEffect("WLF_HuntPrey", g_walker))
		FatalError("ScorpionArtSmoke FAIL step 3: walker AI not removable");
	g_walker->SetAction("Walk");
	g_walker->SetComDir(COMD_Right);
	g_iPhaseLegTick = 0;
	g_iPhaseMask = 0;
	g_iPhaseLegStage = 0;
	return true;
}

global func FxPhaseWalkStart(target, effect, temp) { return 1; }

global func FxPhaseWalkTimer(object target, int effect, int timer)
{
	++g_iPhaseLegTick;
	/* Stall budget: six phases at Delay=2 cycle in ~12 ticks; 60 ticks
	   per direction is a generous bound that still fits the smoke's
	   350-tick budget together with the sync leg. */
	if (g_iPhaseLegTick > 60)
		FatalError(Format("ScorpionArtSmoke FAIL step 3: phase stall stage %d tick %d mask %d action %s pos %d,%d",
		                  g_iPhaseLegStage, g_iPhaseLegTick, g_iPhaseMask,
		                  g_walker->GetAction(), g_walker->GetX(), g_walker->GetY()));
	if (!g_walker || !g_walker->GetAlive())
		FatalError("ScorpionArtSmoke FAIL step 3: walker lost");
	/* Re-enter Walk at a direction switch so the phase clock keeps
	   running on the engine's own ComDir accel. */
	if (g_iPhaseLegStage == 0)
		g_walker->SetComDir(COMD_Right);
	else
		g_walker->SetComDir(COMD_Left);

	var ph = g_walker->GetPhase();
	if (ph < 0 || ph >= 6)
		FatalError("ScorpionArtSmoke FAIL step 3: phase out of range");
	g_iPhaseMask = g_iPhaseMask | (1 << ph);
	if (g_iPhaseMask != 63)   /* 0x3F: all six phases seen */
		return 1;

	if (g_iPhaseLegStage == 0)
	{
		Log("ScorpionArtSmoke walk right: all 6 phases live");
		g_iPhaseLegStage = 1;
		g_iPhaseLegTick = 0;
		g_iPhaseMask = 0;
		g_walker->SetDir(1);
		g_walker->SetAction("Walk");   /* re-enter after SetDir turn */
		g_walker->SetComDir(COMD_Left);
		return 1;
	}

	Log("ScorpionArtSmoke walk left: all 6 phases live");
	Log("ScorpionArtSmoke PASS");
	GameOver();
	return -1;
}
