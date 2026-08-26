/*-- Cave Ambience: per-stratum SetGamma + ambient drip --*/

#strict

// Strata boundaries (landscape px y). Spec: 0..100 top, 100..200 lake,
// 200..300 crystal, 300..400 lava. These map to a 400px-tall scenario.
static const CAVE_Stratum0_Top    = 0;
static const CAVE_Stratum1_Top    = 100;
static const CAVE_Stratum2_Top    = 200;
static const CAVE_Stratum3_Top    = 300;
static const CAVE_Stratum_Bottom  = 400;

local current_stratum;  // last applied stratum (avoid re-applying every tick)

protected func Initialize()
{
	current_stratum = -1;
	SetPosition(0, 0);
}

/* TimerCall (every frame): update gamma based on deepest living crew member */
protected func AmbienceTick()
{
	// Find deepest living crew member (DeepAbyss FxDepthLightTimer pattern)
	var deepest = 0, dy = -1;
	for (var c in FindObjects(Find_Category(C4D_Living), Find_OCF(OCF_Alive)))
	{
		if (GetY(c) > dy) { dy = GetY(c); deepest = c; }
	}
	if (!deepest) return;

	var y = GetY(deepest);
	var stratum;
	if (y < CAVE_Stratum1_Top)       stratum = 0;
	else if (y < CAVE_Stratum2_Top)  stratum = 1;
	else if (y < CAVE_Stratum3_Top)  stratum = 2;
	else                             stratum = 3;

	if (stratum != current_stratum)
	{
		ApplyStratumGamma(stratum);
		current_stratum = stratum;
	}

	// Ambient drip particles in strata 0-2 (cool blue-grey drips)
	if (stratum < 3 && !Random(10))
	{
		var dx = Random(LandscapeWidth());
		CastParticles("Drip", 1, 30, dx - GetX(), 0 - GetY(), 30, 30,
			RGB(80, 90, 120), RGB(40, 50, 70));
	}
}
