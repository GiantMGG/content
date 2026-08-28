/*-- AirshipSmoke.c4s -- headless content integration test. --*/
/*                                                              */
/* Exercises the three airship vehicles: Steampack (STPK),       */
/* Cargo Zeppelin (ZEPN), and Hot-Air Balloon (HABL). Follows    */
/* the SiegeSmoke.c4s contract: direct calls from Initialize,    */
/* FatalError on assertion failure, Log PASS + GameOver.         */
/*                                                              */
/* On any assertion failure, FatalError produces a non-zero exit  */
/* code, failing the CTest entry.                                */

#strict 2

protected func Initialize()
{
	RunSmokeSteps();
	return true;
}

func RunSmokeSteps()
{
	var pSteampack, pClonk, pZeppelin, pBalloon;

	pSteampack = CreateObject(STPK, 50, 30, NO_OWNER);
	if (!pSteampack) FatalError("AirshipSmoke FAIL step 0: could not spawn STPK");
	pClonk = CreateObject(CLNK, 50, 25, NO_OWNER);
	if (!pClonk) FatalError("AirshipSmoke FAIL step 0: could not spawn CLNK");
	Enter(pClonk, pSteampack);
	pSteampack->Activate(pClonk);
	if (!pSteampack->IsActive())
		FatalError("AirshipSmoke FAIL step 0: steampack not active after Activate");
	Log("AirshipSmoke step 0 PASS: steampack activated");

	pZeppelin = CreateObject(ZEPN, 50, 20, NO_OWNER);
	if (!pZeppelin) FatalError("AirshipSmoke FAIL step 1: could not spawn ZEPN");
	pZeppelin->CreateContents(COAL);
	pZeppelin->CreateContents(WOOD);
	if (pZeppelin->ContentsCount() < 2)
		FatalError("AirshipSmoke FAIL step 1: zeppelin contents < 2");
	var f = GetPhysical("Float", 0, pZeppelin);
	if (f != 100)
		FatalError("AirshipSmoke FAIL step 1: zeppelin Float != 100");
	Log("AirshipSmoke step 1 PASS: zeppelin spawned with Float=100");

	pZeppelin->BurnerTimer();
	f = GetPhysical("Float", 0, pZeppelin);
	if (f <= 0)
		FatalError("AirshipSmoke FAIL step 2: zeppelin Float not sustained");
	Log("AirshipSmoke step 2 PASS: zeppelin burner running");

	pBalloon = CreateObject(HABL, 50, 20, NO_OWNER);
	if (!pBalloon) FatalError("AirshipSmoke FAIL step 3: could not spawn HABL");
	pBalloon->CreateContents(WOOD);
	pBalloon->BurnerTimer();
	var h = pBalloon->LocalN("heat");
	if (h <= 0)
		FatalError("AirshipSmoke FAIL step 3: balloon heat not rising");
	f = GetPhysical("Float", 0, pBalloon);
	if (f <= 0)
		FatalError("AirshipSmoke FAIL step 3: balloon Float not scaling with heat");
	Log("AirshipSmoke step 3 PASS: balloon heat and Float rising");

	Log("AirshipSmoke PASS");
	GameOver();
}
