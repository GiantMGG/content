/*-- SeaTradeSmoke.c4s -- headless Phase-1 smoke. --*/
/* Exercises: CGSH container + BuyGoodAt, MoveTo drive, STRM launch +   */
/* IsStormActive, PSLP spawn + IsPirateShip, east-dock SellGoodAt.      */
/* On assertion failure, FatalError -> non-zero exit.                   */

#strict 2

static g_iStep;
static pWestPost, pEastPost, pCog;
static iWealthAfterStep1;

protected func Initialize() {
	g_iStep = 0;
	// West producer post (sells SILK), east consumer post (buys SILK).
	pWestPost = CreateObject(MKTS, 20, 30, NO_OWNER);
	pEastPost = CreateObject(MKTS, 80, 30, NO_OWNER);
	RegisterTradeGood(SILK, pWestPost, 10);
	RegisterTradeGood(SILK, pEastPost, 10);
	// Cargo ship at west dock (y=20 hovers in air; DFA_NONE -> no gravity).
	pCog = CreateObject(CGSH, 25, 20, NO_OWNER);
	AddEffect("RunTest", this, 1, 35, this);
	return true;
}

func FxRunTestStart(target, effect, temp) { return 1; }

func FxRunTestTimer(object target, int effect, int timer) {
	if (g_iStep == 0) {
		// Assertion 1: CGSH spawned + container enumerable (edge case #1).
		if (!pCog) FatalError("SeaTradeSmoke FAIL step 0: CGSH not spawned");
		if (ContentsCount(SILK, pCog) != 0)
			FatalError("SeaTradeSmoke FAIL step 0: container not empty");
		// Assertion 2: BuyGoodAt loads SILK + wealth decreases.
		var iPlr = GetAnyPlayer();
		if (iPlr < 0) FatalError("SeaTradeSmoke FAIL step 0: no player");
		var wBefore = GetWealth(iPlr);
		if (!BuyGoodAt(SILK, pCog, pWestPost))
			FatalError("SeaTradeSmoke FAIL step 0: BuyGoodAt failed");
		if (ContentsCount(SILK, pCog) < 1)
			FatalError("SeaTradeSmoke FAIL step 0: SILK not in cog hold");
		var wAfter = GetWealth(iPlr);
		if (wAfter >= wBefore)
			FatalError("SeaTradeSmoke FAIL step 0: wealth did not decrease");
		iWealthAfterStep1 = wAfter;
	}
	else if (g_iStep == 1) {
		// Assertion 3 (part A): issue MoveTo toward east dock.
		var eastX = GetX(pEastPost);
		SetCommand(pCog, "MoveTo", 0, eastX, GetY(pCog));
	}
	else if (g_iStep == 2) {
		// Assertion 3 (part B): cog X advanced.
		if (Abs(GetX(pCog) - 25) <= 0)
			FatalError("SeaTradeSmoke FAIL step 2: cog did not move");
		// Assertion 4: launch STRM, assert active + IsStormActive.
		LaunchWeatherEvent(STRM, 50, 100);
		if (GetActiveWeatherEvent() != STRM)
			FatalError("SeaTradeSmoke FAIL step 2: STRM not active");
		if (!IsStormActive())
			FatalError("SeaTradeSmoke FAIL step 2: IsStormActive false");
	}
	else if (g_iStep == 3) {
		// Stop storm, assert cleared.
		StopWeatherEvent();
		if (GetActiveWeatherEvent() != nil)
			FatalError("SeaTradeSmoke FAIL step 3: storm not stopped");
		if (IsStormActive())
			FatalError("SeaTradeSmoke FAIL step 3: IsStormActive true after stop");
		// Assertion 5: PSLP spawned + IsPirateShip.
		var pPirate = CreateObject(PSLP, 50, 20, NO_OWNER);
		if (!pPirate) FatalError("SeaTradeSmoke FAIL step 3: PSLP not spawned");
		if (!pPirate->IsPirateShip())
			FatalError("SeaTradeSmoke FAIL step 3: IsPirateShip false");
	}
	else if (g_iStep == 4) {
		// Assertion 6: force cog to east dock, SellGoodAt, wealth rises.
		SetPosition(GetX(pEastPost), GetY(pEastPost), pCog);
		var iPlr = GetAnyPlayer();
		var wBefore = GetWealth(iPlr);
		if (!SellGoodAt(SILK, pCog, pEastPost))
			FatalError("SeaTradeSmoke FAIL step 4: SellGoodAt failed");
		var wAfter = GetWealth(iPlr);
		if (wAfter <= wBefore)
			FatalError("SeaTradeSmoke FAIL step 4: wealth did not rise");
		if (wAfter <= iWealthAfterStep1)
			FatalError("SeaTradeSmoke FAIL step 4: wealth not > step1");
	}
	else if (g_iStep == 5) {
		// Assertion 7: pass + end.
		Log("SeaTradeSmoke PASS");
		GameOver();
		return -1;
	}
	++g_iStep;
	return 1;
}
