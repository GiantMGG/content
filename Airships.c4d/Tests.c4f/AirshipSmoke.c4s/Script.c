/*-- AirshipSmoke.c4s -- headless content integration test. --*/

#strict 2

static const C4ID STRM = C4Id("STRM");

static g_iStep;
static g_pSteampack;
static g_pClonk;
static g_pZeppelin;
static g_pBalloon;

protected func Initialize()
{
	g_iStep = 0;
	AddEffect("RunTest", this, 1, 35, this);
	return true;
}

func FxRunTestStart(target, effect, temp) { return 1; }

func FxRunTestTimer(object target, int effect, int timer)
{
	if (g_iStep == 0)
	{
		g_pSteampack = CreateObject(STPK, 50, 30, NO_OWNER);
		if (!g_pSteampack) FatalError("AirshipSmoke FAIL step 0: could not spawn STPK");
		g_pClonk = CreateObject(CLNK, 50, 25, NO_OWNER);
		if (!g_pClonk) FatalError("AirshipSmoke FAIL step 0: could not spawn CLNK");
		Enter(g_pClonk, g_pSteampack);
		g_pSteampack->Activate(g_pClonk);
		if (!g_pSteampack->IsActive())
			FatalError("AirshipSmoke FAIL step 0: steampack not active after Activate");
		Log("AirshipSmoke step 0 PASS: steampack activated");
	}

	if (g_iStep == 1)
	{
		g_pSteampack->CreateContents(COAL);
		var ok = Burn_Consume(g_pSteampack, 10);
		if (!ok) FatalError("AirshipSmoke FAIL step 1: Burn_Consume returned false");
		var res = g_pSteampack->LocalN("fuel_residual");
		if (res <= 0)
			FatalError("AirshipSmoke FAIL step 1: fuel_residual not banked");
		Log("AirshipSmoke step 1 PASS: COAL burned, residual banked");
	}

	if (g_iStep == 2)
	{
		g_pZeppelin = CreateObject(ZEPN, 50, 20, NO_OWNER);
		if (!g_pZeppelin) FatalError("AirshipSmoke FAIL step 2: could not spawn ZEPN");
		g_pZeppelin->CreateContents(COAL);
		g_pZeppelin->CreateContents(WOOD);
		if (g_pZeppelin->ContentsCount() < 2)
			FatalError("AirshipSmoke FAIL step 2: zeppelin contents < 2");
		var f = GetPhysical("Float", 0, g_pZeppelin);
		if (f != 100)
			FatalError("AirshipSmoke FAIL step 2: zeppelin Float != 100");
		Log("AirshipSmoke step 2 PASS: zeppelin spawned with Float=100");
	}

	if (g_iStep == 3)
	{
		LaunchWeatherEvent(STRM, 100, 100);
		if (GetActiveWeatherEvent() != STRM)
			FatalError("AirshipSmoke FAIL step 3: STRM not active");
		g_pZeppelin->BurnerTimer();
		var xdir = Abs(GetXDir(g_pZeppelin));
		if (xdir > 20)
			FatalError("AirshipSmoke FAIL step 3: zeppelin XDir not clamped to 20");
		Log("AirshipSmoke step 3 PASS: STRM wind shear clamped XDir");
	}

	if (g_iStep == 4)
	{
		g_pBalloon = CreateObject(HABL, 50, 20, NO_OWNER);
		if (!g_pBalloon) FatalError("AirshipSmoke FAIL step 4: could not spawn HABL");
		g_pBalloon->CreateContents(WOOD);
		g_pBalloon->BurnerTimer();
		var h = g_pBalloon->LocalN("heat");
		if (h <= 0)
			FatalError("AirshipSmoke FAIL step 4: balloon heat not rising");
		var f = GetPhysical("Float", 0, g_pBalloon);
		if (f <= 0)
			FatalError("AirshipSmoke FAIL step 4: balloon Float not scaling with heat");
		Log("AirshipSmoke step 4 PASS: balloon heat and Float rising");
	}

	if (g_iStep == 5)
	{
		SetWind(0);
		g_pBalloon->BurnerTimer();
		if (GetXDir(g_pBalloon) != 0)
			FatalError("AirshipSmoke FAIL step 5: balloon XDir != 0 with no wind");
		Log("AirshipSmoke step 5 PASS: balloon becalmed at XDir=0");
	}

	if (g_iStep == 6)
	{
		StopWeatherEvent();
		if (GetActiveWeatherEvent() != nil)
			FatalError("AirshipSmoke FAIL step 6: weather event not stopped");
		Log("AirshipSmoke step 6 PASS: weather stopped, vehicles nominal");
	}

	if (g_iStep == 7)
	{
		Log("AirshipSmoke PASS");
		GameOver();
		return -1;
	}

	++g_iStep;
	return 1;
}
