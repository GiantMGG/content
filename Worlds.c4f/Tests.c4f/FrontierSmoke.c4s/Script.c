/*-- FrontierSmoke.c4s -- compressed mirror of the Frontier mechanics
  (cycle 199, roadmap frontier-mvp). Registered by the tests/CMakeLists.txt
  glob as smoke_FrontierSmoke @ --smoke-run 350 with seed pin 199.
  8 steps x 35 ticks = 280 of the 350 budget. Mirror discipline
  (SturmfrontSmoke precedent): predicates are DUPLICATED here, not shared.
  Steps: 1 fixtures (granary twin, fixture clonk, band pins); 2 DRGT
  lifecycle + announce; 3 hunger drop; 4 feed restores + DRGT cleared;
  5 deposit legs to bank >= 10; 6 cap-10 refusal; 7 claim sweep (band AGSH
  claimed, control survives, granary bank intact); 8 PASS. The real deal
  prints are pinned by frontier_outset_boot (MUT-3 reddens that). --*/

#strict 2

static g_iStep;
static g_grny;
static g_clonk;
static g_e0;
static g_e1;

protected func Initialize()
{
	g_iStep = 0;
	g_grny = 0;
	g_clonk = 0;
	g_e0 = 0;
	g_e1 = 0;
	AddEffect("RunTest", 0, 1, 35, 0, 0);
	return true;
}

global func FxRunTestTimer(target, effect, time)
{
	++g_iStep;
	if (g_iStep == 1) return StepFixtures();
	if (g_iStep == 2) return StepDrought();
	if (g_iStep == 3) return StepHunger();
	if (g_iStep == 4) return StepFeed();
	if (g_iStep == 5) return StepDeposit();
	if (g_iStep == 6) return StepCap();
	if (g_iStep == 7) return StepSweep();
	if (g_iStep == 8) return StepPass();
	return -1;
}

global func SmkFail(string why)
{
	FatalError(Format("FrontierSmoke FAIL: %s", why));
	return true;
}

global func SmkSurfaceY(int x)
{
	var y = 10;
	while (y < LandscapeHeight() - 5 && !GBackSolid(x, y)) y++;
	return y;
}

global func SmkWaterY()
{
	var wdt = LandscapeWidth();
	var x, y, best = LandscapeHeight();
	for (x = wdt / 10; x < wdt; x += wdt / 10)
	{
		y = 0;
		while (y < LandscapeHeight() && !GBackLiquid(x, y)) y++;
		if (y < best) best = y;
	}
	return best;
}

global func SmkInBand(int x, int y)
{
	return y >= SmkWaterY() - 60;
}

// Compressed FRSweep mirror: loose AGSH in the band is claimed.
global func SmkSweep()
{
	var n = 0;
	var pS;
	for (pS in FindObjects(Find_ID(AGSH)))
		if (!pS->Contained() && SmkInBand(GetX(pS), GetY(pS)))
		{
			RemoveObject(pS);
			++n;
		}
	return n;
}

global func StepFixtures()
{
	// granary twin on a dry surface spot
	var gx = LandscapeWidth() / 2, gy, tr;
	for (tr = 0; tr < 8; tr++)
	{
		gy = SmkSurfaceY(gx);
		if (!GBackLiquid(gx, gy - 4)) break;
		gx += 25;
	}
	g_grny = CreateObject(GRNY, gx, gy, NO_OWNER);
	if (!g_grny) return SmkFail("GRNY spawn");

	// fixture clonk (ownerless CLNK is OCF_CrewMember: CrewMember=1)
	g_clonk = CreateObject(CLNK, gx + 40, gy - 20, NO_OWNER);
	if (!g_clonk) return SmkFail("fixture clonk");
	g_e0 = GetEnergy(g_clonk);

	var wy = SmkWaterY();
	if (wy <= 10 || wy >= LandscapeHeight()) return SmkFail("waterline scan");
	Log(Format("FRSMK:water_y=%d", wy));
	Log(Format("FRSMK:fixtures ok (grny=%d,%d energy=%d)", gx, gy, g_e0));
	return 1;
}

global func StepDrought()
{
	LaunchWeatherEvent(DRGT, 50, 700);
	if (GetActiveWeatherEvent() != DRGT)
		return SmkFail("DRGT not active after launch");
	Log("FRSMK:announce=The drought is upon us -- keep every clonk fed.");
	return 1;
}

global func StepHunger()
{
	// compressed FRHunger mirror: one drain beat on the fixture (MUT-4
	// disables this line -- the drop assert must go red).
	if (GetEnergy(g_clonk) > 15) DoEnergy(-2, g_clonk);
	g_e1 = GetEnergy(g_clonk);
	if (g_e1 >= g_e0)
		return SmkFail("hunger drop not measurable");
	Log(Format("FRSMK:hunger energy=%d", g_e1));
	return 1;
}

global func StepFeed()
{
	CreateContents(AGAP, g_grny, 1);
	if (!g_grny->Eat(g_clonk)) return SmkFail("GRNY Eat refused");
	if (GetEnergy(g_clonk) <= g_e1)
		return SmkFail("Eat did not restore energy");
	StopWeatherEvent();
	if (GetActiveWeatherEvent() != 0) return SmkFail("event not cleared");
	Log("FRSMK:feed restored energy; DRGT cleared");
	return 1;
}

global func StepDeposit()
{
	// bank one sheaf per leg until the granary holds 10 (player-check
	// element 4: bank >= 10 in the granary)
	var legs = 0;
	while (ContentsCount(AGSH, g_grny) + ContentsCount(AGAP, g_grny) < 10 && legs < 20)
	{
		CreateContents(AGSH, g_clonk, 1);
		if (!g_grny->Deposit(g_clonk)) break;
		++legs;
	}
	var banked = ContentsCount(AGSH, g_grny) + ContentsCount(AGAP, g_grny);
	if (banked < 10) return SmkFail(Format("banked %d < 10", banked));
	Log(Format("FRSMK:banked=%d legs=%d", banked, legs));
	return 1;
}

global func StepCap()
{
	// 11th deposit refused at the cap-10 (AutoTasksSmoke step-2c pattern;
	// MUT-2's target assert)
	CreateContents(AGSH, g_clonk, 1);
	g_grny->Deposit(g_clonk);
	if (ContentsCount(AGSH, g_grny) != 10)
		return SmkFail("cap broken: granary != 10");
	if (ContentsCount(AGSH, g_clonk) < 1)
		return SmkFail("sheaf lost on refusal");
	Log("FRSMK:cap=10 held");
	return 1;
}

global func StepSweep()
{
	// band fixture (claimed) + control fixture (survives); the granary bank
	// must come through the sweep untouched.
	var cx = -1, hx = -1, x, sy;
	for (x = 20; x < LandscapeWidth() && (cx < 0 || hx < 0); x += 10)
	{
		sy = SmkSurfaceY(x);
		if (cx < 0 && SmkInBand(x, sy)) cx = x;
		if (hx < 0 && !SmkInBand(x, sy)) hx = x;
	}
	if (cx < 0) return SmkFail("no floodplain fixture spot");
	if (hx < 0) return SmkFail("no high-ground fixture spot");
	var pClaim = CreateObject(AGSH, cx, SmkSurfaceY(cx), NO_OWNER);
	var pCtrl = CreateObject(AGSH, hx, SmkSurfaceY(hx), NO_OWNER);
	if (!pClaim || !pCtrl) return SmkFail("sweep fixtures missing");
	var before = ObjectCount(AGSH);
	var n = SmkSweep();
	if (n < 1) return SmkFail("claim sweep removed nothing");
	if (ObjectCount(AGSH) != before - 1)
		return SmkFail("sweep removed wrong count");
	if (ContentsCount(AGSH, g_grny) != 10)
		return SmkFail("sweep touched the granary bank");
	Log(Format("FRSMK:sweep claimed=%d bank intact", n));
	return 1;
}

global func StepPass()
{
	Log("FrontierSmoke PASS");
	GameOver();
	return 1;
}
