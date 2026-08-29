/*-- FuelSmoke.c4s -- headless smoke test for FuelSystem.c4d --*/

#strict 3

// DBURN is a 5-char def id; the engine truncates to 4 chars (DBUR).
// DBUR is a C4ID literal that resolves to the same C4ID.
static const DBURN = DBUR;

static g_pBurner;

protected func Initialize()
{
	g_pBurner = 0;
	RunSmokeSteps();
	return true;
}

func RunSmokeSteps()
{
	/* Step 0: spawn the dummy burner + one of each default fuel in its contents. */
	g_pBurner = CreateObject(DBURN, 50, 30, NO_OWNER);
	if (!g_pBurner) FatalError("FuelSmoke FAIL step 0: could not spawn DBURN");
	g_pBurner->CreateContents(COAL);
	g_pBurner->CreateContents(WOOD);
	g_pBurner->CreateContents(OBRL);
	if (CountContents(g_pBurner) != 3)
		FatalError(Format("FuelSmoke FAIL step 0: expected 3 contents got %d",
		                  CountContents(g_pBurner)));
	Log("FuelSmoke step 0 PASS: burner spawned with COAL+WOOD+OBRL");

	/* Step 1: Burn 50: OBRL (eff 1.5, value 200) is consumed first -> BARL.
	   Residual banked: 200 - 50 = 150. COAL + WOOD untouched. */
	var ok = Burn_Consume(g_pBurner, 50);
	if (!ok) FatalError("FuelSmoke FAIL step 1: Burn_Consume returned false");
	if (FindInBurner(OBRL))
		FatalError("FuelSmoke FAIL step 1: OBRL not consumed");
	if (!FindInBurner(BARL))
		FatalError("FuelSmoke FAIL step 1: OBRL not converted to BARL");
	if (!FindInBurner(COAL))
		FatalError("FuelSmoke FAIL step 1: COAL wrongly consumed");
	if (!FindInBurner(WOOD))
		FatalError("FuelSmoke FAIL step 1: WOOD wrongly consumed");
	var res = g_pBurner->LocalN("fuel_residual");
	if (res != 150)
		FatalError(Format("FuelSmoke FAIL step 1: expected residual 150 got %v", res));
	Log("FuelSmoke step 1 PASS: OBRL->BARL, residual=150, COAL+WOOD remain");

	/* Step 2: Burn 50 again: residual (150) covers it -> residual = 100.
	   No fuel item destroyed. */
	ok = Burn_Consume(g_pBurner, 50);
	if (!ok) FatalError("FuelSmoke FAIL step 2: Burn_Consume returned false");
	res = g_pBurner->LocalN("fuel_residual");
	if (res != 100)
		FatalError(Format("FuelSmoke FAIL step 2: expected residual 100 got %v", res));
	if (CountContents(g_pBurner) != 3)
		FatalError(Format("FuelSmoke FAIL step 2: expected 3 contents got %d",
		                  CountContents(g_pBurner)));
	Log("FuelSmoke step 2 PASS: residual drained to 100, no item destroyed");

	/* Step 3: Burn 200: residual 100 drained first (->0), then need=100 remains.
	   COAL (eff 1.2, value 100) is more efficient than WOOD (eff 0.7,
	   value 40), so COAL is consumed next. provided=100 == need=100 ->
	   no overshoot. Result: COAL destroyed, WOOD remains, residual=0. */
	ok = Burn_Consume(g_pBurner, 200);
	if (!ok) FatalError("FuelSmoke FAIL step 3: Burn_Consume returned false");
	if (FindInBurner(COAL))
		FatalError("FuelSmoke FAIL step 3: COAL not consumed");
	if (!FindInBurner(WOOD))
		FatalError("FuelSmoke FAIL step 3: WOOD wrongly consumed");
	res = g_pBurner->LocalN("fuel_residual");
	if (res)
		FatalError(Format("FuelSmoke FAIL step 3: expected residual 0 got %v", res));
	Log("FuelSmoke step 3 PASS: COAL consumed before WOOD, residual=0");

	Log("FuelSmoke PASS");
	GameOver();
	return true;
}

/* Count objects inside pContainer via engine-native Contents() iteration. */
func CountContents(object pContainer)
{
	var n = 0;
	while (pContainer->Contents(n)) ++n;
	return n;
}

/* True if an object of id is inside the burner, via Contents() iteration. */
func FindInBurner(id id)
{
	var i, obj;
	for (i = 0; obj = g_pBurner->Contents(i); ++i)
		if (GetID(obj) == id) return true;
	return false;
}
