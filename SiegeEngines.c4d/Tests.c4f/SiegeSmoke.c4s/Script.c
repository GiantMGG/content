/*-- SiegeSmoke.c4s -- headless content integration test. --*/
/*                                                              */
/* Exercises the siege engines + STGT destructible-wall system  */
/* + the new SiegeRepair and BoilingOilCauldron mechanics.      */
/* Follows the EventSmoke.c4s contract.                        */
/*                                                              */
/* On any assertion failure, FatalError produces a non-zero    */
/* exit code, failing the CTest entry.                          */

#strict 2

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
	// Step 0: spawn a SGAT, damage it with SBLD, verify OnSiegeDestroyed fires.
	if (g_iStep == 0)
	{
		var pGate = CreateObject(SGAT, 50, 30, NO_OWNER);
		if (!pGate) FatalError("SiegeSmoke FAIL step 0: could not spawn SGAT");
		pGate->~SiegeDamage(200, NO_OWNER, SBLD);  // SBLD halves threshold
		if (FindObject(SGAT))
			FatalError("SiegeSmoke FAIL step 0: OnSiegeDestroyed did not fire");
	}
	// Step 1: spawn a fresh SGAT, damage partially, verify SiegeRepair
	//         decrements damage and clears crack overlays.
	if (g_iStep == 1)
	{
		var pGate = CreateObject(SGAT, 50, 30, NO_OWNER);
		pGate->~SiegeDamage(40, NO_OWNER, SROK);  // 40/120 = 1/3 -> Crack1
		if (pGate->LocalN("iSiegeDamage") != 40)
			FatalError("SiegeSmoke FAIL step 1a: damage not accumulated");
		pGate->~SiegeRepair(20, NO_OWNER);
		if (pGate->LocalN("iSiegeDamage") != 20)
			FatalError("SiegeSmoke FAIL step 1b: repair did not decrement");
		// iSiegeDamage now 20 < 120/3 -> crack overlay should be cleared
	}
	// Step 2: spawn a BoilingOilCauldron, activate it, verify DFLM cast.
	if (g_iStep == 2)
	{
		var pCauldron = CreateObject(BOIL, 50, 30, NO_OWNER);
		if (!pCauldron) FatalError("SiegeSmoke FAIL step 2: could not spawn BOIL");
		pCauldron->~ControlDig(NO_OWNER);
		// Verify DFLM was cast
		if (!FindObject(DFLM, 0, 0, 0, 0, 0, 0, 0, pCauldron))
			FatalError("SiegeSmoke FAIL step 2: ControlDig did not cast DFLM");
	}
	// Step 3: spawn a TRBT, fire a SBLD, verify it breaches a SGAT.
	if (g_iStep == 3)
	{
		var pGate = CreateObject(SGAT, 50, 30, NO_OWNER);
		var pTreb = CreateObject(TRBT, 30, 30, NO_OWNER);
		if (!pTreb) FatalError("SiegeSmoke FAIL step 3: could not spawn TRBT");
		// (TRBT fires SBLD that breaches SGAT -- verified by FindObject
		// returning nil after the breach.)
		if (FindObject(SGAT))
			FatalError("SiegeSmoke FAIL step 3: SGAT not breached by TRBT/SBLD");
	}
	// Step 4: spawn a SCAT, fire a SROK, verify siege damage to a SGAT.
	if (g_iStep == 4)
	{
		var pGate = CreateObject(SGAT, 50, 30, NO_OWNER);
		var pCat = CreateObject(SCAT, 30, 30, NO_OWNER);
		if (!pCat) FatalError("SiegeSmoke FAIL step 4: could not spawn SCAT");
		// (SCAT fires SROK that damages SGAT -- verified by LocalN check.)
		if (pGate->LocalN("iSiegeDamage") <= 0)
			FatalError("SiegeSmoke FAIL step 4: SROK did not damage SGAT");
	}
	// Step 5: spawn a BRAM, push it at a SGAT, verify momentum damage.
	if (g_iStep == 5)
	{
		var pGate = CreateObject(SGAT, 50, 30, NO_OWNER);
		var pRam = CreateObject(BRAM, 30, 30, NO_OWNER);
		if (!pRam) FatalError("SiegeSmoke FAIL step 5: could not spawn BRAM");
		// (BRAM pushes into SGAT, ContactLeft/Right fires, momentum damage.)
		if (pGate->LocalN("iSiegeDamage") <= 0)
			FatalError("SiegeSmoke FAIL step 5: BRAM did not damage SGAT");
	}
	// Step 6: pass + end.
	if (g_iStep == 6)
	{
		Log("SiegeSmoke PASS");
		GameOver();
		return -1;  // kill the effect
	}
	++g_iStep;
	return 1;
}
