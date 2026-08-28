/*-- AirshipSmoke.c4s -- headless content integration test. --*/
/*                                                              */
/* Exercises the three airship vehicles: Steampack (STPK),       */
/* Cargo Zeppelin (ZEPN), and Hot-Air Balloon (HABL).            */
/*                                                              */
/* Weather coupling (the spec's "centerpiece") is exercised      */
/* across all three vehicles:                                   */
/*   - Steampack: STRM doubles wind drift                       */
/*   - Zeppelin:  STRM clamps XDir to +/-20 and reduces Float   */
/*   - Balloon:   heat->Float scaling, HTWV reduces Float       */
/*                                                              */
/* Uses the direct-call RunSmokeSteps() pattern (called from     */
/* Initialize). The AddEffect timer pattern does NOT fire in     */
/* the smoke harness -- known engine limitation. See             */
/* SiegeSmoke.c4s for the reference direct-call pattern.        */
/*                                                              */
/* On any assertion failure, FatalError produces a non-zero     */
/* exit code, failing the CTest entry.                          */

#strict 2

protected func Initialize()
{
	RunSmokeSteps();
	return true;
}

func RunSmokeSteps()
{
	/* LaunchWeatherEvent expects int; store C4IDs in vars to avoid
	   type-checker errors (same pattern as EventSmoke.c4s). */
	var ev_strm = STRM;
	var ev_htwv = HTWV;

	/* ---- Step 0: Steampack activation ---- */
	var steampack = CreateObject(STPK, 50, 30, NO_OWNER);
	if (!steampack) FatalError("AirshipSmoke FAIL step 0: could not spawn STPK");
	var clonk = CreateObject(CLNK, 50, 25, NO_OWNER);
	if (!clonk) FatalError("AirshipSmoke FAIL step 0: could not spawn CLNK");
	Enter(clonk, steampack);
	steampack->Activate(clonk);
	if (!steampack->IsActive())
		FatalError("AirshipSmoke FAIL step 0: steampack not active");
	Log("AirshipSmoke step 0 PASS: steampack activated");

	/* ---- Step 1: Steampack fuel burn + residual banking ---- */
	steampack->CreateContents(COAL);
	var ok = Burn_Consume(steampack, 10);
	if (!ok) FatalError("AirshipSmoke FAIL step 1: Burn_Consume returned false");
	var res = steampack->LocalN("fuel_residual");
	if (res <= 0)
		FatalError("AirshipSmoke FAIL step 1: fuel_residual not banked");
	Log("AirshipSmoke step 1 PASS: COAL burned, residual banked");

	/* ---- Step 2: Zeppelin spawn + Float=100 (clear weather) ---- */
	var zeppelin = CreateObject(ZEPN, 50, 20, NO_OWNER);
	if (!zeppelin) FatalError("AirshipSmoke FAIL step 2: could not spawn ZEPN");
	zeppelin->CreateContents(COAL);
	zeppelin->CreateContents(WOOD);
	if (zeppelin->ContentsCount() < 2)
		FatalError("AirshipSmoke FAIL step 2: zeppelin contents < 2");
	var f = GetPhysical("Float", 0, zeppelin);
	if (f != 100)
		FatalError("AirshipSmoke FAIL step 2: zeppelin Float != 100");
	Log("AirshipSmoke step 2 PASS: zeppelin spawned with Float=100");

	/* ---- Step 3: Zeppelin STRM wind shear + Float reduction ---- */
	LaunchWeatherEvent(ev_strm, 100, 100);
	if (GetActiveWeatherEvent() != STRM)
		FatalError("AirshipSmoke FAIL step 3: STRM not active");
	zeppelin->BurnerTimer();
	/* STRM at intensity 100: wind shear clamps XDir to +/-20 */
	if (Abs(GetXDir(zeppelin)) > 20)
		FatalError("AirshipSmoke FAIL step 3: zeppelin XDir not clamped to 20");
	/* STRM at intensity 100: lift_pct=90, Float = 100*90/100 = 90 */
	f = GetPhysical("Float", 0, zeppelin);
	if (f != 90)
		FatalError("AirshipSmoke FAIL step 3: zeppelin Float != 90 under STRM");
	Log("AirshipSmoke step 3 PASS: STRM wind shear + Float reduction");

	/* ---- Step 4: Balloon heat->Float scaling (clear weather) ---- */
	StopWeatherEvent();
	var balloon = CreateObject(HABL, 50, 20, NO_OWNER);
	if (!balloon) FatalError("AirshipSmoke FAIL step 4: could not spawn HABL");
	balloon->CreateContents(WOOD);
	balloon->BurnerTimer();
	var h = balloon->LocalN("heat");
	if (h <= 0)
		FatalError("AirshipSmoke FAIL step 4: balloon heat not rising");
	/* Clear weather: lift_pct=100, Float = heat * 100 / 100 = heat */
	f = GetPhysical("Float", 0, balloon);
	if (f != h)
		FatalError("AirshipSmoke FAIL step 4: Float != heat under clear weather");
	Log("AirshipSmoke step 4 PASS: balloon heat->Float scaling");

	/* ---- Step 5: Balloon HTWV reduces Float ---- */
	LaunchWeatherEvent(ev_htwv, 100, 100);
	if (GetActiveWeatherEvent() != HTWV)
		FatalError("AirshipSmoke FAIL step 5: HTWV not active");
	balloon->BurnerTimer();
	/* HTWV at intensity 100: lift_pct=80, Float = heat * 80 / 100 */
	h = balloon->LocalN("heat");
	f = GetPhysical("Float", 0, balloon);
	if (f != h * 80 / 100)
		FatalError("AirshipSmoke FAIL step 5: Float != heat*80/100 under HTWV");
	Log("AirshipSmoke step 5 PASS: HTWV reduces balloon Float");

	/* ---- Step 6: Steampack STRM wind drift coupling ---- */
	StopWeatherEvent();
	steampack->CreateContents(COAL); /* ensure fuel for burn ticks */
	SetWind(40);
	/* Clear weather: Aero_WindDrift(50) = 40*100*50/40000 = 5 */
	steampack->FxIntSteampackHoverTimer(clonk, 0, 0);
	var xdir_clear = GetXDir(clonk);
	if (xdir_clear != 5)
		FatalError("AirshipSmoke FAIL step 6: steampack XDir != 5 under clear weather");
	/* STRM at intensity 100: wind_pct=200, drift = 40*200*50/40000 = 10 */
	LaunchWeatherEvent(ev_strm, 100, 100);
	steampack->FxIntSteampackHoverTimer(clonk, 0, 0);
	var xdir_strm = GetXDir(clonk);
	if (xdir_strm != 10)
		FatalError("AirshipSmoke FAIL step 6: steampack XDir != 10 under STRM");
	Log("AirshipSmoke step 6 PASS: STRM doubles steampack wind drift");

	/* ---- Step 7: Balloon becalmed (wind=0 -> XDir=0) ---- */
	StopWeatherEvent();
	SetWind(0);
	balloon->BurnerTimer();
	if (GetXDir(balloon) != 0)
		FatalError("AirshipSmoke FAIL step 7: balloon XDir != 0 with no wind");
	Log("AirshipSmoke step 7 PASS: balloon becalmed at XDir=0");

	/* ---- Step 8: Stop weather + pass ---- */
	StopWeatherEvent();
	if (GetActiveWeatherEvent())
		FatalError("AirshipSmoke FAIL step 8: weather event not stopped");
	Log("AirshipSmoke PASS");
	GameOver();
}
