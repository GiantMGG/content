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
	var pGate, pCauldron, pTreb, pCat, pRam, pWall;

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
	// SetPhase clamps to Length-1 = 6, so a successful call leaves phase
	// exactly 6; anything else means SetPhase silently failed (stuck at 0).
	if (pTreb->GetPhase() != 6)
		FatalError("SiegeSmoke FAIL step 8b: SetPhase(6) did not take (phase < 6 or out of band)");

	// Step 9: FIX-2 wall setup -- destroy a castle wall (CPW2, no Ruin
	// sheet) now; SiegeCheck10 asserts it is gone while the step-0/6d
	// gates (Ruin sheet) still linger.
	// Spawn method: probe.c4s (cycle 181) proved bare CreateObject(CPW2)
	// yields a live object (findable at frame 5), so no CreateConstruction
	// is needed here.
	pWall = CreateObject(CPW2, 25, 30, NO_OWNER);
	// Spawn verification: a destroyed-object handle stays truthy, so the
	// old `if (!pWall)` guard passed even after the wall was removed.
	// Pin the spawn by reading a property instead -- CreateObject returns
	// a live object, so this fails loudly on a nil/dead handle.
	if (!pWall) FatalError("SiegeSmoke FAIL step 9: could not spawn CPW2");
	if (GetX(pWall) != 25)
		FatalError("SiegeSmoke FAIL step 9: CPW2 handle not live at spawn");
	pWall->~SiegeDamage(9999, NO_OWNER, SBLD);
	// FIX-2 immediate-remove: a def WITHOUT a Ruin sheet is removed
	// synchronously inside OnSiegeDestroyed, so the wall is already gone
	// here. ObjectCount is the dead-handle-safe probe -- reading a Local
	// off the removed object would raise 'Object call: target is zero!'
	// (the smoke_fix1.log signature) instead of asserting.
	if (ObjectCount(CPW2))
		FatalError("SiegeSmoke FAIL step 9: CPW2 ghosting after destroy (immediate-remove branch broken)");

	// Pass is deferred to SiegeCheck10 (frame-based tail below).
	Schedule("SiegeCheck10()", 12, 0, this());
}

// Frame-based tail (FIX-2 regression): the ruin-swap branch in
// STGT::OnSiegeDestroyed must remove a def WITHOUT a Ruin sheet (castle
// wall CPW2) immediately -- no 35-frame intact-sprite ghost -- while a
// def WITH one (SGAT) keeps the linger. The wall spawns+destroys in step
// 0's frame; the destroyed gates from steps 0/6d are still in their
// 35-frame linger when this runs.
func SiegeCheck10()
{
	// Wall (no Ruin sheet): ObjectCount must be 0 now -- it was removed
	// in its destroy frame, not scheduled 35 frames later.
	if (ObjectCount(CPW2))
		FatalError("SiegeSmoke FAIL step 10: no-Ruin wall still ghosting (not removed immediately)");
	// Gate (Ruin sheet): at least one destroyed gate must still be
	// present in its linger window (removal lands at ~frame 35).
	var pG, iDestroyed = 0;
	for (pG in FindObjects(Find_ID(SGAT)))
		if (pG->LocalN("fSiegeDestroyed"))
			++iDestroyed;
	if (!iDestroyed)
		FatalError("SiegeSmoke FAIL step 10: Ruin gate was removed instantly (linger branch broken)");

	Log("SiegeSmoke PASS");
	GameOver();
}
