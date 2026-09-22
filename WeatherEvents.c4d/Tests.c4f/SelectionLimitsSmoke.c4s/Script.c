/*-- SelectionLimitsSmoke.c4s — headless content integration test. --*/
/*                                                                */
/* Pins the raised command-stack cap (spec mouse-selection-limits, */
/* cycle 165): 60 consecutive AddCommand("Wait") calls on one      */
/* object must all return true — 60 > 35 (old cap), <= 200 (new).  */
/* AddCommand is the exact C4Object::AddCommand path that control  */
/* packets and scripts share, so this covers the "no frozen clonk" */
/* half of the player check.                                       */
/*                                                                */
/* On any assertion failure, FatalError produces a non-zero exit   */
/* code, failing the CTest entry.                                  */

#strict 3

protected func Initialize()
{
	RunSmokeSteps();
	return true;
}

func RunSmokeSteps()
{
	var pTest = CreateObject(AGWH, 50, 30, NO_OWNER);
	if (!pTest)
		FatalError("SelectionLimitsSmoke FAIL: could not spawn test object");
	var i;
	for (i = 1; i <= 60; i++)
	{
		if (!AddCommand(pTest, "Wait"))
			FatalError(Format("SelectionLimitsSmoke FAIL: AddCommand #%d refused", i));
	}
	Log("SelectionLimitsSmoke PASS");
	GameOver();
	return true;
}
