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
	// (#strict 2 rejects the `eq` operator — strict-2 smoke precedent uses !=.)
	if (pCauldron->GetAction() != "Idle")
		FatalError("SiegeSmoke FAIL step 2: BOIL not in Idle action");
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

	// Step 6: tier machine — the same SetGraphics branch that fires the
	// visual swap must set iSiegeTier; repair recomputes it downward.
	// SGAT MaxSiegeHP=120: Crack1 at 40, Crack2 at 80, destroy at 120.
	pGate = CreateObject(SGAT, 50, 30, NO_OWNER);
	pGate->~SiegeDamage(39, NO_OWNER, SROK);
	if (pGate->~GetSiegeTier() != 0)
		FatalError("SiegeSmoke FAIL step 6a: tier != 0 below Crack1 threshold");
	pGate->~SiegeDamage(1, NO_OWNER, SROK);
	if (pGate->~GetSiegeTier() != 1)
		FatalError("SiegeSmoke FAIL step 6b: Crack1 tier not set at 1/3 crossing");
	pGate->~SiegeDamage(40, NO_OWNER, SROK);
	if (pGate->~GetSiegeTier() != 2)
		FatalError("SiegeSmoke FAIL step 6c: Crack2 tier not set at 2/3 crossing");
	pGate->~SiegeDamage(40, NO_OWNER, SROK);
	if (!pGate->LocalN("fSiegeDestroyed") || pGate->~GetSiegeTier() != 3)
		FatalError("SiegeSmoke FAIL step 6d: destroy tier not set");

	pGate = CreateObject(SGAT, 50, 30, NO_OWNER);
	pGate->~SiegeDamage(80, NO_OWNER, SROK);
	if (pGate->~GetSiegeTier() != 2)
		FatalError("SiegeSmoke FAIL step 6e: expected Crack2 tier at 80");
	pGate->~SiegeRepair(45, NO_OWNER);
	if (pGate->~GetSiegeTier() != 0)
		FatalError("SiegeSmoke FAIL step 6f: repair did not recompute tier to 0");
	if (pGate->LocalN("iSiegeDamage") != 35)
		FatalError("SiegeSmoke FAIL step 6g: repair amount wrong");

	// Step 7: SoundExists probes (FirstLightSoundSmoke pattern).
	if (!SoundExists("SiegeCrack")) FatalError("SiegeSmoke FAIL step 7: SiegeCrack unresolved");
	if (!SoundExists("SiegeBreak")) FatalError("SiegeSmoke FAIL step 7: SiegeBreak unresolved");
	if (!SoundExists("OilSizzle"))  FatalError("SiegeSmoke FAIL step 7: OilSizzle unresolved");
	if (!SoundExists("GateOpen"))   FatalError("SiegeSmoke FAIL step 7: GateOpen unresolved");

	// Step 8: TRBT wiring on the new sheet bands.
	pTreb = CreateObject(TRBT, 30, 30, NO_OWNER);
	if (!pTreb) FatalError("SiegeSmoke FAIL step 8: could not spawn TRBT");
	if (pTreb->GetAction() != "Ready")
		FatalError("SiegeSmoke FAIL step 8a: TRBT not in Ready action");
	pTreb->SetAction("Swing");
	pTreb->SetPhase(6);
	if (pTreb->GetPhase() > 6)
		FatalError("SiegeSmoke FAIL step 8b: TRBT phase out of band bounds");

	// Step 9: pass + end.
	Log("SiegeSmoke PASS");
	GameOver();
}
