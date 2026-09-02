/*-- OilImmobileRepro.c4s — cycle-90 symptom-1 repro (spec §4.0-4.1). --*/
/* Basin: painted fill over a diggable Sandstone floor; step 1     */
/* digs the player-analogue slot; step 9 asserts the drain.        */
#strict 2

static const FillMatName = "Oil";
static const FamBName = "";
static const DRAIN_MIN = 20;
static const FALL_MIN = 50;
static const DigDelay = 35;   // tick at which the slot is dug

static g_iStep;
static g_fCalibrate;
static g_fDug;
static g_iDigStep;
static g_iBaseline;
static g_matFill;
static g_matFamB;

protected func Initialize()
{
	DrawMaterialQuad("Rock", 100, 60, 110, 60, 110, 160, 100, 160);
	DrawMaterialQuad("Rock", 290, 60, 300, 60, 300, 160, 290, 160);
	DrawMaterialQuad("Sandstone", 100, 150, 300, 150, 300, 160, 100, 160);
	DrawMaterialQuad(FillMatName, 111, 100, 289, 100, 289, 149, 111, 149);
	g_matFill = Material(FillMatName);
	if (FamBName != "")
		g_matFamB = Material(FamBName);
	else
		g_matFamB = -1;
	g_iBaseline = CountFamilyTank();
	if (g_iBaseline < 8000)
		FatalError(Format("OilImmobileRepro FAIL: fill short (%d)", g_iBaseline));
	g_iStep = 0;
	g_fCalibrate = 0;
	AddEffect("RunTest", 0, 1, 35, 0, 0);
	return true;
}

global func FxRunTestStart(target, effect, temp) { return 1; }

global func FxRunTestTimer(object target, int effect, int timer)
{
	++g_iStep;
	if (!g_fDug && timer >= DigDelay)
	{
		DigFreeRect(190, 150, 8, 10);
		g_fDug = 1;
		g_iDigStep = g_iStep;
	}
	else
	{
		if (GetPXSCount() > 10000)
			FatalError(Format("OilImmobileRepro FAIL step %d: PXS budget exceeded (%d)", g_iStep, GetPXSCount()));
		if (GetMaterialCount(Material("Steam")) != 0)
			FatalError(Format("OilImmobileRepro FAIL step %d: static steam appeared (%d)", g_iStep, GetMaterialCount(Material("Steam"))));
	}
	var do_assert = !g_fCalibrate && g_fDug && g_iStep >= g_iDigStep + 8;
	if (g_fCalibrate || do_assert)
	{
		var tank = CountFamilyTank();
		var fall = CountFamilyFall();
		var drain = 100 - tank * 100 / g_iBaseline;
		if (g_fCalibrate)
			Log(Format("[CAL] step %d tank %d fall %d famB %d pxs %d drain %d dug %d", g_iStep,
				tank, fall, CountFamBTank(), GetPXSCount(), drain, g_fDug * g_iDigStep));
		if (do_assert)
		{
			if (drain < DRAIN_MIN)
				FatalError(Format("OilImmobileRepro FAIL: tank drain %d pct < %d (painted %s frozen)", drain, DRAIN_MIN, FillMatName));
			if (fall < FALL_MIN)
				FatalError(Format("OilImmobileRepro FAIL: fall-zone family %d < %d", fall, FALL_MIN));
			Log("OilImmobileRepro PASS");
			GameOver();
			return -1;
		}
	}
	return 1;
}

global func CountFamilyTank()
{
	var n = 0, x, y;
	for (x = 111; x <= 289; x++)
		for (y = 60; y <= 159; y++)
			if (IsFamily(GetMaterial(x, y))) n++;
	return n;
}

global func CountFamilyFall()
{
	var n = 0, x, y;
	for (x = 0; x <= 999; x++)
		for (y = 160; y <= 199; y++)
			if (IsFamily(GetMaterial(x, y))) n++;
	return n;
}

global func CountFamBTank()
{
	if (g_matFamB == -1) return 0;
	var n = 0, x, y;
	for (x = 111; x <= 289; x++)
		for (y = 60; y <= 159; y++)
			if (GetMaterial(x, y) == g_matFamB) n++;
	return n;
}

global func IsFamily(int m)
{
	return m == g_matFill || (g_matFamB != -1 && m == g_matFamB);
}
