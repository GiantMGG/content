/*-- FoundrySmoke.c4s -- headless smoke for Foundry's fuel-library path --*/
/*                                                              */
/* Exercises the library path's fuel economy directly via        */
/* Burn_Consume on a real FNDR burner (whose local              */
/* fuel_residual was added in the Foundry refactor).            */
/* Follows the SiegeSmoke.c4s synchronous contract.             */
/*                                                              */
/* ORE1 is deliberately omitted: the Foundry's ContentsCheck    */
/* library branch lights whenever ORE1 is present, so leaving    */
/* ORE1 in the burner would race the test's explicit             */
/* Burn_Consume calls. The ORE1->METL smelt is unchanged legacy  */
/* Burning() behaviour, out of scope for the fuel-path refactor. */
/*                                                              */
/* On any assertion failure, FatalError produces a non-zero     */
/* exit code, failing the CTest entry smoke_FoundrySmoke.        */

#strict 2

protected func Initialize()
{
	RunSmokeSteps();
	return true;
}

func RunSmokeSteps()
{
	/* Step 0: runtime gate must be open (FuelSystem.c4d loaded) and
	   the real FNDR burner must spawn. */
	if (!DefinitionCall(FUEL, "IsFuel"))
		FatalError("FoundrySmoke FAIL step 0: DefinitionCall(FUEL,IsFuel) false - FuelSystem.c4d not loaded?");
	var burner = CreateObject(FNDR, 50, 30, NO_OWNER);
	if (!burner)
		FatalError("FoundrySmoke FAIL step 0: could not spawn FNDR");
	Log("FoundrySmoke step 0 PASS: gate open, FNDR spawned");

	/* Step 1: 1 COAL (value 100) == BURN_NEED(100): one smelt cycle, nothing banked. */
	ClearBurner(burner);
	burner->CreateContents(COAL);
	if (!Burn_Consume(burner, 100))
		FatalError("FoundrySmoke FAIL step 1: Burn_Consume(COAL) returned false");
	if (FindInBurner(burner, COAL))
		FatalError("FoundrySmoke FAIL step 1: COAL not consumed");
	AssertResidual(burner, 0, 1);
	Log("FoundrySmoke step 1 PASS: 1 COAL -> 1 smelt cycle, residual 0");

	/* Step 2: 1 OBRL (value 200): 1st smelt consumes 100, banks 100 residual
	   -> 2nd smelt runs on residual alone. Proves "2 smelts from 1 OBRL". */
	ClearBurner(burner);
	burner->CreateContents(OBRL);
	if (!Burn_Consume(burner, 100))
		FatalError("FoundrySmoke FAIL step 2: Burn_Consume(OBRL) returned false");
	if (FindInBurner(burner, OBRL))
		FatalError("FoundrySmoke FAIL step 2: OBRL not consumed");
	if (!FindInBurner(burner, BARL))
		FatalError("FoundrySmoke FAIL step 2: OBRL not converted to BARL");
	AssertResidual(burner, 100, 2);
	Log("FoundrySmoke step 2 PASS: OBRL->BARL, residual 100 banked");

	/* Step 3: 2nd smelt from the banked residual alone - no fuel item touched. */
	if (!Burn_Consume(burner, 100))
		FatalError("FoundrySmoke FAIL step 3: Burn_Consume(residual) returned false");
	AssertResidual(burner, 0, 3);
	Log("FoundrySmoke step 3 PASS: 2nd smelt drained residual -> 0");

	/* Step 4: 3 WOOD (value 40 each = 120): one smelt cycle (100) + 20 residual banked. */
	ClearBurner(burner);
	burner->CreateContents(WOOD);
	burner->CreateContents(WOOD);
	burner->CreateContents(WOOD);
	if (!Burn_Consume(burner, 100))
		FatalError("FoundrySmoke FAIL step 4: Burn_Consume(WOOD) returned false");
	if (FindInBurner(burner, WOOD))
		FatalError("FoundrySmoke FAIL step 4: WOOD not fully consumed");
	AssertResidual(burner, 20, 4);
	Log("FoundrySmoke step 4 PASS: 3 WOOD -> 1 smelt cycle, residual 20 banked");

	/* Step 5: pass + end. */
	Log("FoundrySmoke PASS");
	GameOver();
}

/* Remove all contents and clear the residual bank so the next test starts clean. */
func ClearBurner(object burner)
{
	var obj;
	while (obj = burner->Contents(0)) RemoveObject(obj);
	burner->LocalN("fuel_residual") = 0;
}

func AssertResidual(object burner, int expected, int step)
{
	var res = burner->LocalN("fuel_residual");
	if (res != expected)
		FatalError(Format("FoundrySmoke FAIL step %d: expected residual %d got %v",
		                  step, expected, res));
}

func FindInBurner(object burner, id what)
{
	var i, obj;
	for (i = 0; obj = burner->Contents(i); ++i)
		if (GetID(obj) == what) return true;
	return false;
}
