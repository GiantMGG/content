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

	// Starting kit into the workbench: 2 fish traps, 6 wheat seeds (sow 6,
	// harvest 2, keep 4 standing for the field leg), a sickle, 14 wood +
	// 1 metal (windmill 7W/1M, smokehouse 7W; critic FIX-NOW 1, cycle 168,
	// FIX-NOW 2 re-scope: rope work replaced by timber — Rope.c4d is not
	// loaded by this mission's def set, so only resolvable materials are
	// funded). The workbench stands at x=500.
	var gy = 100;
	while (gy < 550 && !GBackSolid(500, gy)) gy++;
	var pBase = CreateObject(WRKS, 500, gy, NO_OWNER);
	if (pBase)
	{
		CreateContents(AGFT, pBase);
		CreateContents(AGFT, pBase);
		CreateContents(AGWS, pBase);
		CreateContents(AGWS, pBase);
		CreateContents(AGWS, pBase);
		CreateContents(AGWS, pBase);
		CreateContents(AGWS, pBase);
		CreateContents(AGWS, pBase);
		CreateContents(AGSK, pBase);
		CreateContents(WOOD, pBase);
		CreateContents(WOOD, pBase);
		CreateContents(WOOD, pBase);
		CreateContents(WOOD, pBase);
		CreateContents(WOOD, pBase);
		CreateContents(WOOD, pBase);
		CreateContents(WOOD, pBase);
		CreateContents(WOOD, pBase);
		CreateContents(WOOD, pBase);
		CreateContents(WOOD, pBase);
		CreateContents(WOOD, pBase);
		CreateContents(WOOD, pBase);
		CreateContents(WOOD, pBase);
		CreateContents(WOOD, pBase);
		CreateContents(METL, pBase);
		// Chapter-1 fishery: the pond west of the homestead
		CarveHomesteadPond(360, gy);
	}

	Log("$MsgArrival$");
	AddEffect("Story", 0, 1, 35, 0, 0);
	AddEffect("Audit", 0, 1, 2100, 0, 0);
	AddEffect("Caravan", 0, 1, 2100, 0, 0);
	AddEffect("LedgerHUD", 0, 1, 35, 0, 0);
	HomesteadDrawLedgerHUD();
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

// The stall's registered smoked-fish stock is materialized as contained
// AGSF (RegisterTradeGood) and must not satisfy the fishery census
// (review H1-Flash). Only uncontained AGSF counts as loose fish.
global func FishStock()
{
	var n = ObjectCount(FISH) + ObjectCount(DFSH);
	var p;
	for (var p in FindObjects(Find_ID(AGSF)))
		if (!p->Contained()) n++;
	return n;
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

/* ---- live ledger HUD + red-audit naming (cycle 183) ---- */

// Ripe-wheat census for the HUD's Field segment. Display-only: the
// field predicate (HomesteadFieldGreen) is unchanged.
global func HomesteadRipeWheatCount()
{
	var ripe = 0, pW;
	for (var pW in FindObjects(Find_ID(AGWH)))
		if (pW->IsRipe()) ripe++;
	return ripe;
}

// Trade leg display text: frames since the last caravan sale, or
// "none" before the first sale. All composition via Format() - the
// ".."-on-bool quirk is designed out (roadmap.md:246).
global func HomesteadTradeAgeText()
{
	if (g_last_sale < 0) return "none";
	return Format("%df", FrameCounter() - g_last_sale);
}

// One HUD line: four leg segments joined with " · ", state words are
// literally green/dark. Single source of truth - reads the same
// Homestead*Green() predicates the audit reads, so the HUD can never
// disagree with the ledger.
global func HomesteadLedgerHUDText()
{
	// State words computed with if/else: this fork's parser has no
	// C-style ternary (the '?' token is STRICT3 safe-navigation only),
	// so the green/dark wording must be set branch-wise.
	var sFishery = "dark";
	if (HomesteadFisheryGreen()) sFishery = "green";
	var sField = "dark";
	if (HomesteadFieldGreen()) sField = "green";
	var sMill = "dark";
	if (HomesteadMillGreen()) sMill = "green";
	var sTrade = "dark";
	if (HomesteadTradeGreen()) sTrade = "green";
	return Format("Fishery %d/8 %s · Field %d/4 %d/2 %s · Mill %d %s · Trade %s %s",
		FishStock(), sFishery,
		ObjectCount(AGWH), HomesteadRipeWheatCount(),
		sField,
		g_mill_total, sMill,
		HomesteadTradeAgeText(), sTrade);
}

// Dark-leg list for red audits: each dark leg is named with its
// numbers, green legs are omitted. Segments joined with ", ".
global func HomesteadRedLegsText()
{
	var txt = "", sep = "";
	if (!HomesteadFisheryGreen())
	{
		txt = Format("%s%sFishery %d/8 dark", txt, sep, FishStock());
		sep = ", ";
	}
	if (!HomesteadFieldGreen())
	{
		txt = Format("%s%sField %d/4 %d/2 dark", txt, sep,
			ObjectCount(AGWH), HomesteadRipeWheatCount());
		sep = ", ";
	}
	if (!HomesteadMillGreen())
	{
		txt = Format("%s%sMill %d dark", txt, sep, g_mill_total);
		sep = ", ";
	}
	if (!HomesteadTradeGreen())
	{
		txt = Format("%s%sTrade %s dark", txt, sep, HomesteadTradeAgeText());
		sep = ", ";
	}
	return txt;
}

// Draw the HUD: one player-0 global-player CustomMessage, bottom-left,
// width-capped. Re-issuing with the same player + positioning flags
// replaces the prior line in place (engine C4GameMessageList::New
// clears same-player/same-flags first; an empty string only deletes).
// Offsets are percentages of the viewport under MSG_XRel/MSG_YRel;
// tune the three numbers (-46, 8, 40) only against the 1080p shot.
global func HomesteadDrawLedgerHUD()
{
	CustomMessage(HomesteadLedgerHUDText(), 0, 0, -46, 8, 0xffffff, 0, 0,
		MSG_Bottom | MSG_Left | MSG_ALeft | MSG_XRel | MSG_YRel | MSG_WidthRel, 40);
	return true;
}

// HUD refresh: re-issue every 35 frames. On win (two consecutive green
// audits) one empty-string issue with the same flags deletes the
// overlay and stops the effect.
global func FxLedgerHUDTimer(target, effect, time)
{
	if (HomesteadLedgerFulfilled())
	{
		CustomMessage("", 0, 0, 0, 0, 0xffffff, 0, 0,
			MSG_Bottom | MSG_Left | MSG_ALeft | MSG_XRel | MSG_YRel | MSG_WidthRel, 0);
		return -1;
	}
	HomesteadDrawLedgerHUD();
	return 1;
}

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
	else Log(Format("$MsgLedgerRed$ %s", HomesteadRedLegsText()));
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
