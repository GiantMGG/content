/*-- FirstLight.c4s -- four-chapter homestead vignette (cycle 127). --*/
/* Chapters: First Catch -> First Sowing -> The Mill Turns -> Market  */
/* Day. Finale: the Homestead Ledger must hold all four legs green    */
/* through two consecutive audits; the HMGL goal reads that state.    */
/* Zero engine changes. Every primitive is shipped (spec 1200).      */

#strict 2

static g_chapter;       // 1 FirstCatch .. 4 MarketDay, 5 = finale open
static g_last_sale;     // FrameCounter() of the last caravan sale
static g_mill_total;    // cumulative FLOU milled since the mill leg opened
static g_flou_prev;     // mill-watch delta tracker
static g_green_audits;  // consecutive green audits

protected func Initialize()
{
	g_chapter = 1;
	g_last_sale = -1;
	g_mill_total = 0;
	g_flou_prev = 0;
	g_green_audits = 0;

	// Starting kit into the workbench: 2 fish traps, seeds, a sickle,
	// 4 wood (spec chapter 0). The workbench stands at x=500.
	var gy = 100;
	while (gy < 550 && !GBackSolid(500, gy)) gy++;
	var pBase = CreateObject(WRKS, 500, gy, NO_OWNER);
	if (pBase)
	{
		CreateContents(AGFT, pBase);
		CreateContents(AGFT, pBase);
		CreateContents(AGWS, pBase);
		CreateContents(AGWS, pBase);
		CreateContents(AGSK, pBase);
		CreateContents(WOOD, pBase);
		CreateContents(WOOD, pBase);
		CreateContents(WOOD, pBase);
		CreateContents(WOOD, pBase);
		// Chapter-1 fishery: the pond west of the homestead
		CarveHomesteadPond(360, gy);
	}

	Log("$MsgArrival$");
	AddEffect("Story", 0, 1, 35, 0, 0);
	AddEffect("Audit", 0, 1, 2100, 0, 0);
	AddEffect("Caravan", 0, 1, 2100, 0, 0);
	return true;
}

/* ---- pond carving (F3-proven dig+fill recipe) ---- */

global func CarveHomesteadPond(int iPx, int iPyHint)
{
	// find the ground row near iPx
	var gy = 100;
	while (gy < 550 && !GBackSolid(iPx, gy)) gy++;
	DigFreeRect(iPx - 40, gy - 4, 80, 40);
	var px, py, water_mat = Material("Water");
	// fill bottom-up so the water stacks instead of raining through
	for (py = gy + 34; py > gy - 3; py--)
		for (px = iPx - 39; px < iPx + 40; px++)
			InsertMaterial(water_mat, px, py);
	// seed the pond: 8 fish
	var i;
	for (i = 0; i < 8; i++)
		CreateObject(FISH, iPx - 8 + i * 2, gy + 30, NO_OWNER);
	return true;
}

/* ---- ledger leg predicates ---- */

global func FishStock()
{
	return ObjectCount(FISH) + ObjectCount(DFSH) + ObjectCount(AGSF);
}

// Chapter-1 gate: evidence the player actually used a trap, not the
// seeded pond stock. The pond's 8 fish satisfy FishStock() at spawn, so
// the chapter must key on a trap that holds a caught fish.
global func HomesteadCaughtFish()
{
	var pTrap;
	for (var pTrap in FindObjects(Find_ID(AGFT)))
		if (pTrap->ContentsCount(FISH) >= 1) return true;
	return false;
}

global func HomesteadFisheryGreen()
{
	var fWet = false, pTrap;
	for (var pTrap in FindObjects(Find_ID(AGFT)))
		if (pTrap->InLiquid()) { fWet = true; break; }
	if (!fWet)
		for (var pTrap in FindObjects(Find_ID(LBTP)))
			if (pTrap->InLiquid()) { fWet = true; break; }
	if (!fWet) return false;
	return FishStock() >= 8;
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

// Loose-only flour count: the stall's registered FLOU stock is contained
// and must not satisfy the mill leg (review H1-Flash). Only uncontained
// FLOU counts as "milled".
global func HomesteadLooseFlourCount()
{
	var n = 0, p;
	for (var p in FindObjects(Find_ID(FLOU)))
		if (!p->Contained()) n++;
	return n;
}

global func HomesteadTradeGreen()
{
	if (!FindObject(MKTS)) return false;
	return g_last_sale > -1 && FrameCounter() - g_last_sale <= 2100;
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

/* ---- caravan factor (the trade demand side; spec chapter 4) ---- */

global func HomesteadFindLooseGoods()
{
	// loose only: never buy the stall's own registered stock (edge case 6)
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
	var iPlr = GetOwner(pGoods);
	RemoveObject(pGoods);
	g_last_sale = FrameCounter();
	if (iPlr > -1) DoWealth(iPlr, iPrice);
	return 1;
}

global func OpenMarket()
{
	var pBase = FindObject(WRKS);
	if (!pBase) return 0;
	var pStall = CreateObject(MKTS, GetX(pBase) + 60, GetY(pBase) - 10, NO_OWNER);
	if (!pStall) return 0;
	RegisterTradeGood(AGSF, pStall, 10);
	RegisterTradeGood(FLOU, pStall, 10);
	// The market bell announces the opening
	Sound("MarketBell");
	return 1;
}

/* ---- chapter director ---- */

global func GrantKnowledge(id idDef)
{
	if (GetPlayerCount() > 0) SetPlrKnowledge(0, idDef);
	return 1;
}

global func FxStoryTimer(target, effect, time)
{
	// mill-watch: tally newly milled flour (loose only — the stall's
	// registered stock is contained and must never bump the ledger)
	var f = HomesteadLooseFlourCount();
	if (f > g_flou_prev) g_mill_total += f - g_flou_prev;
	g_flou_prev = f;

	if (g_chapter == 1 && HomesteadCaughtFish())
	{
		g_chapter = 2;
		GrantKnowledge(AGWS);
		GrantKnowledge(AGSK);
		Log("$MsgFirstCatch$");
		Log("$MsgSowHint$");
	}
	else if (g_chapter == 2 && ObjectCount(AGSH) >= 1)
	{
		g_chapter = 3;
		GrantKnowledge(AGWM);
		GrantKnowledge(AGSM);
		Log("$MsgGoldenRows$");
	}
	else if (g_chapter == 3 && ObjectCount(FLOU) >= 1)
	{
		g_chapter = 4;
		OpenMarket();
		Log("$MsgMillTurns$");
	}
	else if (g_chapter == 4 && g_last_sale > -1)
	{
		g_chapter = 5;
		Log("$MsgMarketDay$");
		Log("$MsgLedgerOpen$");
	}
	return 1;
}

/* ---- audit + caravan effects ---- */

global func FxAuditTimer(target, effect, time)
{
	// The ledger only counts once all four chapters have fired.
	if (g_chapter < 5) return 1;
	var green = HomesteadAudit();
	if (green) { Log("$MsgLedgerGreen$"); Sound("LedgerChime"); }
	else Log("$MsgLedgerRed$");
	if (g_green_audits >= 2) Log("$MsgWin$");
	// The engine's goal polling picks up IsFulfilled via HMGL from here.
	return 1;
}

global func FxCaravanTimer(target, effect, time)
{
	// The caravan factor visits only while a stall stands.
	CaravanFactorTick(FindObject(MKTS));
	return 1;
}
