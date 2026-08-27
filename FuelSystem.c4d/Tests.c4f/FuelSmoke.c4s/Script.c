/*-- FuelSmoke.c4s -- headless smoke test for FuelSystem.c4d --*/

#strict 2

static g_iStep;
static g_pBurner;

protected func Initialize()
{
	g_iStep = 0;
	g_pBurner = 0;
	AddEffect("RunTest", this, 1, 35, this);
	return true;
}

func FxRunTestStart(target, effect, temp) { return 1; }

func FxRunTestTimer(object target, int effect, int timer)
{
	if (g_iStep == 0)
	{
		/* Spawn the dummy burner + one of each default fuel in its contents. */
		g_pBurner = CreateObject(DBURN, 50, 30, NO_OWNER);
		if (!g_pBurner) FatalError("FuelSmoke FAIL step 0: could not spawn DBURN");
		g_pBurner->CreateContents(COAL);
		g_pBurner->CreateContents(WOOD);
		g_pBurner->CreateContents(OBRL);
		if (CountContents(g_pBurner) != 3)
			FatalError(Format("FuelSmoke FAIL step 0: expected 3 contents got %d",
			                  CountContents(g_pBurner)));
		Log("FuelSmoke step 0 PASS: burner spawned with COAL+WOOD+OBRL");
	}

	if (g_iStep == 1)
	{
		/* Burn 50: OBRL (eff 1.5, value 200) is consumed first -> BARL.
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
	}

	if (g_iStep == 2)
	{
		/* Burn 50 again: residual (150) covers it -> residual = 100.
		   No fuel item destroyed. */
		var ok = Burn_Consume(g_pBurner, 50);
		if (!ok) FatalError("FuelSmoke FAIL step 2: Burn_Consume returned false");
		var res = g_pBurner->LocalN("fuel_residual");
		if (res != 100)
			FatalError(Format("FuelSmoke FAIL step 2: expected residual 100 got %v", res));
		if (CountContents(g_pBurner) != 3)
			FatalError(Format("FuelSmoke FAIL step 2: expected 3 contents got %d",
			                  CountContents(g_pBurner)));
		Log("FuelSmoke step 2 PASS: residual drained to 100, no item destroyed");
	}

	if (g_iStep == 3)
	{
		/* Burn 200: residual 100 drained first (->0), then need=100 remains.
		   COAL (eff 1.2, value 100) is more efficient than WOOD (eff 0.7,
		   value 40), so COAL is consumed next. provided=100 == need=100 ->
		   no overshoot. Result: COAL destroyed, WOOD remains, residual=0. */
		var ok = Burn_Consume(g_pBurner, 200);
		if (!ok) FatalError("FuelSmoke FAIL step 3: Burn_Consume returned false");
		if (FindInBurner(COAL))
			FatalError("FuelSmoke FAIL step 3: COAL not consumed");
		if (!FindInBurner(WOOD))
			FatalError("FuelSmoke FAIL step 3: WOOD wrongly consumed");
		var res = g_pBurner->LocalN("fuel_residual");
		if (res != 0)
			FatalError(Format("FuelSmoke FAIL step 3: expected residual 0 got %v", res));
		Log("FuelSmoke step 3 PASS: COAL consumed before WOOD, residual=0");
	}

	if (g_iStep == 4)
	{
		Log("FuelSmoke PASS");
		GameOver();
		return -1;
	}
	++g_iStep;
	return 1;
}

/* Count objects inside pContainer via engine-native Contents() iteration. */
func CountContents(object pContainer)
{
	var n = 0;
	while (pContainer->Contents(n)) ++n;
	return n;
}

/* True if an object of id is inside the burner, via Contents() iteration. */
func FindInBurner(C4ID id)
{
	var i, obj;
	for (i = 0; obj = g_pBurner->Contents(i); ++i)
		if (GetID(obj) == id) return true;
	return false;
}
