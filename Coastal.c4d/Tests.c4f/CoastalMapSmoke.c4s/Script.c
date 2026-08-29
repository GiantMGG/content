/*-- CoastalMapSmoke.c4s -- map-preset connectivity smoke. --*/
/* Loads the Coastal.c4d/Landscape.txt preset (copied into this scenario    */
/* folder because the map creator reads Landscape.txt only from the         */
/* scenario's own group). Asserts the generated map has expected dimensions, */
/* water present along the bottom, and that OnMapGenerated() stamps the      */
/* SLMR + DKST markers. Uses the direct-call RunSmokeSteps() pattern.        */

#strict 2

protected func Initialize()
{
	RunSmokeSteps();
	return true;
}

func RunSmokeSteps()
{
	// ---- Assertion 1: map dimensions match the preset (100*10 x 40*10) ----
	if (LandscapeWidth() != 1000)
		FatalError(Format("CoastalMapSmoke FAIL 1: LandscapeWidth %d != 1000", LandscapeWidth()));
	if (LandscapeHeight() != 400)
		FatalError(Format("CoastalMapSmoke FAIL 1: LandscapeHeight %d != 400", LandscapeHeight()));

	// ---- Assertion 2: water present along the bottom of the map ----
	var iWater = 0;
	var waterMat = Material("Water");
	for (var x = 0; x < LandscapeWidth(); x += 50)
	{
		if (GetMaterial(x, LandscapeHeight() - 10) == waterMat) iWater++;
	}
	if (iWater <= 0)
		FatalError("CoastalMapSmoke FAIL 2: no Water found along the bottom");

	// ---- Assertion 3: OnMapGenerated() stamps SLMR pair + DKST ----
	OnMapGenerated();
	if (!FindObject(SLMR))
		FatalError("CoastalMapSmoke FAIL 3: no SLMR stamped by OnMapGenerated");
	if (!FindObject(DKST))
		FatalError("CoastalMapSmoke FAIL 3: no DKST stamped by OnMapGenerated");

	Log("CoastalMapSmoke PASS");
	GameOver();
}
