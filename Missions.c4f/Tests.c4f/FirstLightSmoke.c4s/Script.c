/*-- FirstLightSmoke.c4s -- headless mirror of FirstLight.c4s (cycle 127). --*/
/* Drives all four ledger legs synchronously, then two green audits.        */

#strict 2

static g_iStep;
static g_last_sale;
static g_mill_total;
static g_flou_prev;
static g_green_audits;

protected func Initialize()
{
	SetWind(20);
	g_iStep = 0;
	g_last_sale = -1;
	g_mill_total = 0;
	g_flou_prev = 0;
	g_green_audits = 0;
	AddEffect("RunTest", 0, 1, 35, 0, 0);
	return true;
}

global func FxRunTestStart(target, effect, temp) { return 1; }

/* ---- ledger leg predicates (mirror of FirstLight.c4s) ---- */

global func HomesteadFisheryGreen()
{
	var fWet = false, pTrap;
	for (var pTrap in FindObjects(Find_ID(AGFT)))
		if (pTrap->InLiquid()) { fWet = true; break; }
	if (!fWet)
		for (var pTrap in FindObjects(Find_ID(LBTP)))
			if (pTrap->InLiquid()) { fWet = true; break; }
	if (!fWet) return false;
	var stock = ObjectCount(FISH) + ObjectCount(DFSH) + ObjectCount(AGSF);
	return stock >= 8;
}

global func HomesteadFieldGreen()
{
	if (ObjectCount(AGWH) < 4) return false;
	var ripe = 0, pW;
	for (var pW in FindObjects(Find_ID(AGWH)))
		if (pW->IsRipe()) ripe++;
	return ripe >= 2;
}

global func HomesteadMillGreen()
{
	var pMill = FindObject(AGWM);
	if (!pMill) return false;
	if (GetCon(pMill) < 100) return false;
	return g_mill_total >= 1;
}

global func HomesteadTradeGreen()
{
	if (!FindObject(MKTS)) return false;
	return FrameCounter() - g_last_sale <= 2100;
}

global func HomesteadAudit()
{
	var green = HomesteadFisheryGreen() && HomesteadFieldGreen()
	          && HomesteadMillGreen() && HomesteadTradeGreen();
	if (green) ++g_green_audits;
	else g_green_audits = 0;
	return green;
}

global func HomesteadLedgerFulfilled() { return g_green_audits >= 2; }

/* ---- caravan factor mirror (playerless: sale recorded, no wealth) ---- */

global func HomesteadFindLooseGoods()
{
	var p;
	for (var p in FindObjects(Find_ID(AGSF)))
		if (!p->Contained()) return p;
	for (var p in FindObjects(Find_ID(FLOU)))
		if (!p->Contained()) return p;
	return 0;
}

global func CaravanFactorTick(object pStall)
{
	if (!pStall) return 0;
	var pGoods = HomesteadFindLooseGoods();
	if (!pGoods) return 0;
	var iPrice = GetMarketPriceAt(GetID(pGoods), pStall);
	RemoveObject(pGoods);
	g_last_sale = FrameCounter();
	return 1;
}

/* ---- driver ---- */

global func FxRunTestTimer(target, effect, time)
{
	++g_iStep;
	// mill-watch: tally newly milled flour
	var f = ObjectCount(FLOU);
	if (f > g_flou_prev) g_mill_total += f - g_flou_prev;
	g_flou_prev = f;

	if (g_iStep == 1)
	{
		// mill leg: start grinding early (flour lands ~t=195)
		var gx = 500, gy = 100;
		while (gy < 550 && !GBackSolid(gx, gy)) gy++;
		var pMill = CreateObject(AGWM, gx, gy - 60, NO_OWNER);
		if (!pMill) FatalError("FirstLightSmoke FAIL: step 1 - no mill");
		CreateContents(AGSH, pMill);
		if (!pMill->ProductionStart())
			FatalError("FirstLightSmoke FAIL: step 1 - ProductionStart refused");
		if (GetAction(pMill) != "Grinding")
			FatalError("FirstLightSmoke FAIL: step 1 - mill not grinding");
	}

	if (g_iStep == 2)
	{
		// fishery setup: dig pocket, fill water, drop trap (settles by step 3)
		var gx = 300, gy = 100;
		while (gy < 550 && !GBackSolid(gx, gy)) gy++;
		DigFreeRect(gx - 40, gy - 4, 80, 40);
		var px, py, water_mat = Material("Water");
		for (py = gy + 34; py > gy - 3; py--)
			for (px = gx - 39; px < gx + 40; px++)
				InsertMaterial(water_mat, px, py);
		var pTrap = CreateObject(AGFT, gx, gy + 26, NO_OWNER);
		if (!pTrap) FatalError("FirstLightSmoke FAIL: step 2 - no trap");
	}

	if (g_iStep == 3)
	{
		// fishery leg: trap settled, spawn fish at the trap, Attract
		var pTrap = FindObject(AGFT);
		if (!pTrap) FatalError("FirstLightSmoke FAIL: step 3 - trap vanished");
		if (!pTrap->InLiquid())
			FatalError("FirstLightSmoke FAIL: step 3 - trap not in liquid");
		var i;
		for (i = 0; i < 8; i++)
			CreateObject(FISH, GetX(pTrap) - 8 + i * 2, GetY(pTrap) + 4, NO_OWNER);
		pTrap->Attract();
		if (ContentsCount(FISH, pTrap) < 1)
			FatalError("FirstLightSmoke FAIL: step 3 - trap caught nothing");
		if (!HomesteadFisheryGreen())
			FatalError("FirstLightSmoke FAIL: step 3 - fishery leg not green");
	}

	if (g_iStep == 4)
	{
		// field leg: 4 wheat, two Grow calls each (EventSmoke precedent)
		var gx = 650, gy = 100;
		while (gy < 550 && !GBackSolid(gx, gy)) gy++;
		var i, pW;
		for (i = 0; i < 4; i++)
		{
			pW = CreateObject(AGWH, gx + i * 15, gy - 10, NO_OWNER);
			if (!pW) FatalError("FirstLightSmoke FAIL: step 4 - no wheat");
			pW->SetAction("Seedling");
			pW->~Grow();
			pW->~Grow();
		}
		if (!HomesteadFieldGreen())
			FatalError("FirstLightSmoke FAIL: step 4 - field leg not green");
	}

	if (g_iStep == 5)
	{
		// trade leg: stall, register, loose goods, caravan factor
		var gx = 800, gy = 100;
		while (gy < 550 && !GBackSolid(gx, gy)) gy++;
		var pStall = CreateObject(MKTS, gx, gy - 20, NO_OWNER);
		if (!pStall) FatalError("FirstLightSmoke FAIL: step 5 - no stall");
		RegisterTradeGood(AGSF, pStall, 10);
		RegisterTradeGood(FLOU, pStall, 10);
		CreateObject(AGSF, gx - 30, gy - 5, NO_OWNER);  // loose goods near the stall
		CaravanFactorTick(pStall);
		if (g_last_sale < 0)
			FatalError("FirstLightSmoke FAIL: step 5 - no sale recorded");
		if (!HomesteadTradeGreen())
			FatalError("FirstLightSmoke FAIL: step 5 - trade leg not green");
	}

	if (g_iStep == 6)
	{
		// mill leg: flour must exist by now (grind 160 frames, started t=35)
		if (ObjectCount(FLOU) < 1)
			FatalError("FirstLightSmoke FAIL: step 6 - no flour milled");
		if (!HomesteadMillGreen())
			FatalError("FirstLightSmoke FAIL: step 6 - mill leg not green");
	}

	if (g_iStep == 7)
	{
		// audit 1: all four legs must hold simultaneously
		if (!HomesteadAudit())
			FatalError("FirstLightSmoke FAIL: step 7 - audit 1 not green");
		if (g_green_audits != 1)
			FatalError("FirstLightSmoke FAIL: step 7 - audit counter wrong");
	}

	if (g_iStep == 8)
	{
		// audit 2: two consecutive green audits fulfill the ledger
		if (!HomesteadAudit())
			FatalError("FirstLightSmoke FAIL: step 8 - audit 2 not green");
		if (!HomesteadLedgerFulfilled())
			FatalError("FirstLightSmoke FAIL: step 8 - ledger not fulfilled after two green audits");
		Log("FirstLightSmoke PASS");
		GameOver();
		return -1;
	}

	return 1;
}
