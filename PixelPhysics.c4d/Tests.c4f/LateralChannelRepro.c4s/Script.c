/*-- LateralChannelRepro.c4s — cycle-92 lateral-spread repro        --*/
/* (spec 2026-09-02-0258-liquid-flow-hotfix, Pin 2). Three Rock      */
/* basins (water control, oil, lava); per basin a dug lateral        */
/* channel; metric = family count in the far half of the channel.    */
#strict 2

static const CastAmt = 400;  // PXS per basin
static const FarMinWater = 120;  // water control floor (post-fix 160-161)
static const FarMinOil   = 140;  // oil floor (post-fix 162-166; MUT-1 125)
static const FarMinLava  = 108;  // lava floor (post-fix 161-163; MUT-2 57)
static const FinalStep = 9;

static g_iStep;
static g_fCalibrate;
static g_matWater;
static g_matOil;
static g_matLava;
static g_matAshes;

protected func Initialize()
{
	DrawBasin(100);   // water control
	DrawBasin(350);   // oil
	DrawBasin(600);   // lava
	g_matWater = Material("Water");
	g_matOil   = Material("Oil");
	g_matLava  = Material("Lava");
	g_matAshes = Material("Ashes");
	g_iStep = 0;
	g_fCalibrate = 0;   // frozen post-cal (Task 6 far-half floors)
	AddEffect("RunTest", 0, 1, 35, 0, 0);
	return true;
}

global func FxRunTestStart(target, effect, temp) { return 1; }

global func FxRunTestTimer(object target, int effect, int timer)
{
	++g_iStep;
	if (g_iStep == 1)
	{
		DigChannel(100); DigChannel(350); DigChannel(600);
		CastPXS("Water", CastAmt, 10, 200, 100);
		CastPXS("Oil",   CastAmt, 10, 450, 100);
		CastPXS("Lava",  CastAmt, 10, 700, 100);
	}
	else
	{
		if (GetPXSCount() > 10000)
			FatalError(Format("LateralChannelRepro FAIL step %d: PXS budget exceeded (%d)", g_iStep, GetPXSCount()));
	}
	if (g_fCalibrate)
		Log(Format("[CAL] step %d wFar %d oFar %d lFar %d pxs %d", g_iStep,
			CountFar(100, g_matWater, -1), CountFar(350, g_matOil, -1),
			CountFar(600, g_matLava, g_matAshes), GetPXSCount()));
	if (!g_fCalibrate && g_iStep >= FinalStep)
	{
		AssertFar(100, g_matWater, -1, "Water", FarMinWater);
		AssertFar(350, g_matOil, -1, "Oil", FarMinOil);
		AssertFar(600, g_matLava, g_matAshes, "Lava", FarMinLava);
		Log("LateralChannelRepro PASS");
		GameOver();
		return -1;
	}
	return 1;
}

global func DrawBasin(int b)
{
	DrawMaterialQuad("Sandstone", b, 150, b + 200, 150, b + 200, 160, b, 160);
	DrawMaterialQuad("Rock", b, 60, b + 10, 60, b + 10, 150, b, 150);
	DrawMaterialQuad("Rock", b + 190, 60, b + 200, 60, b + 200, 150, b + 190, 150);
}

global func DigChannel(int b)
{
	DigFreeRect(b + 100, 150, 90, 6);   // lateral channel in the floor
}

// Far half of the channel: x in [b+150, b+189], y in [150, 155].
global func CountFar(int b, int mat, int matB)
{
	var n = 0, x, y;
	for (x = b + 150; x <= b + 189; x++)
		for (y = 150; y <= 155; y++)
		{
			var m = GetMaterial(x, y);
			if (m == mat || (matB != -1 && m == matB)) n++;
		}
	return n;
}

global func AssertFar(int b, int mat, int matB, string name, int farMin)
{
	var far = CountFar(b, mat, matB);
	if (far < farMin)
		FatalError(Format("LateralChannelRepro FAIL: %s far-half %d < %d", name, far, farMin));
}
