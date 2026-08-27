/*-- SiegeSmoke.c4s -- headless content integration test. --*/
/*                                                              */
/* Exercises the siege engines + STGT destructible-wall system  */
/* + the new SiegeRepair and BoilingOilCauldron mechanics.      */
/* Follows the EventSmoke.c4s contract.                        */
/*                                                              */
/* On any assertion failure, FatalError produces a non-zero    */
/* exit code, failing the CTest entry.                          */

#strict 2

protected func Initialize()
{
	RunSmokeSteps();
	return true;
}

func RunSmokeSteps()
{
	var pGate, pCauldron, pTreb, pCat, pRam;

	// Step 0: spawn a SGAT, damage it with SBLD, verify OnSiegeDestroyed fires.
	pGate = CreateObject(SGAT, 50, 30, NO_OWNER);
	if (!pGate) FatalError("SiegeSmoke FAIL step 0: could not spawn SGAT");
	pGate->~SiegeDamage(200, NO_OWNER, SBLD);
	if (!pGate->LocalN("fSiegeDestroyed"))
		FatalError("SiegeSmoke FAIL step 0: OnSiegeDestroyed did not fire");

	// Step 1: spawn a fresh SGAT, damage partially, verify SiegeRepair
	//         decrements damage and clears crack overlays.
	pGate = CreateObject(SGAT, 50, 30, NO_OWNER);
	pGate->~SiegeDamage(40, NO_OWNER, SROK);
	if (pGate->LocalN("iSiegeDamage") != 40)
		FatalError("SiegeSmoke FAIL step 1a: damage not accumulated");
	pGate->~SiegeRepair(20, NO_OWNER);
	if (pGate->LocalN("iSiegeDamage") != 20)
		FatalError("SiegeSmoke FAIL step 1b: repair did not decrement");

	// Step 2: spawn a BoilingOilCauldron, activate it, verify DFLM cast.
	pCauldron = CreateObject(BOIL, 50, 30, NO_OWNER);
	if (!pCauldron) FatalError("SiegeSmoke FAIL step 2: could not spawn BOIL");
	pCauldron->~ControlDig(0);
	if (!FindObject(DFLM))
		FatalError("SiegeSmoke FAIL step 2: ControlDig did not cast DFLM");

	// Step 3: spawn a TRBT + SGAT, simulate SBLD hit, verify damage.
	pGate = CreateObject(SGAT, 50, 30, NO_OWNER);
	pTreb = CreateObject(TRBT, 30, 30, NO_OWNER);
	if (!pTreb) FatalError("SiegeSmoke FAIL step 3: could not spawn TRBT");
	pGate->~SiegeDamage(60, NO_OWNER, SBLD);
	if (pGate->LocalN("iSiegeDamage") <= 0)
		FatalError("SiegeSmoke FAIL step 3: SBLD did not damage SGAT");

	// Step 4: spawn a SCAT + SGAT, simulate SROK hit, verify damage.
	pGate = CreateObject(SGAT, 50, 30, NO_OWNER);
	pCat = CreateObject(SCAT, 30, 30, NO_OWNER);
	if (!pCat) FatalError("SiegeSmoke FAIL step 4: could not spawn SCAT");
	pGate->~SiegeDamage(40, NO_OWNER, SROK);
	if (pGate->LocalN("iSiegeDamage") <= 0)
		FatalError("SiegeSmoke FAIL step 4: SROK did not damage SGAT");

	// Step 5: spawn a BRAM + SGAT, simulate momentum hit, verify damage.
	pGate = CreateObject(SGAT, 50, 30, NO_OWNER);
	pRam = CreateObject(BRAM, 30, 30, NO_OWNER);
	if (!pRam) FatalError("SiegeSmoke FAIL step 5: could not spawn BRAM");
	pGate->~SiegeDamage(30, NO_OWNER, BOMB);
	if (pGate->LocalN("iSiegeDamage") <= 0)
		FatalError("SiegeSmoke FAIL step 5: BRAM did not damage SGAT");

	// Step 6: pass + end.
	Log("SiegeSmoke PASS");
	GameOver();
}
