/*-- Oilburn: the Oil room. Oil slick on a stone shelf; the lava tap  */
/* ignites it (Inflammable=1); the water tap kills the blaze           */
/* (Extinguisher=1). Mastery: slick burned to a film floor (defect     */
/* #13: the burn stalls at the 1px floor film - threshold mastery)     */
/* AND the blaze has been seen.                                        */

#strict 2

static const OILBURN_MAX_OIL = 170;  // mastery ceiling (calibrated: stall floor max 132 x 1.25 = 165, rounded up)

static g_iOil0;
static g_fIgnited;

protected func Initialize()
{
	// Shelf band: rock box x[100,300] y[60,160]; oil fill on the floor.
	DrawMaterialQuad("Rock", 100, 60, 110, 60, 110, 160, 100, 160);
	DrawMaterialQuad("Rock", 290, 60, 300, 60, 300, 160, 290, 160);
	DrawMaterialQuad("Rock", 100, 150, 300, 150, 300, 160, 100, 160);
	DrawMaterialQuad("Oil", 110, 130, 290, 130, 290, 150, 110, 150);

	g_iOil0 = CountMatRegion(Material("Oil"), 100, 60, 300, 160);
	g_fIgnited = false;

	// Lava tap (igniter) + water tap (extinguisher).
	var pLava = CreateObject(RTAP, 330, 185, NO_OWNER);
	pLava->SetAction("Idle");
	pLava->SetTapMaterial("Lava");
	pLava->SetTapAmount(10);
	pLava->SetTapOffset(-115, -80);
	var pWater = CreateObject(RTAP, 370, 185, NO_OWNER);
	pWater->SetAction("Idle");
	pWater->SetTapMaterial("Water");
	pWater->SetTapAmount(15);
	pWater->SetTapOffset(-150, -80);

	AddEffect("MasteryWatch", 0, 1, 35, 0, 0);
	Log("$Intro$");
	return true;
}

global func FxMasteryWatchTimer(object target, int effect, int timer)
{
	var iOil = CountMatRegion(Material("Oil"), 100, 60, 300, 160);
	if (FindObject(FLAM)) g_fIgnited = true; // sticky
	if (timer > 105 && iOil <= OILBURN_MAX_OIL && g_fIgnited)
	{
		Log("$Mastery$");
		GameOver();
		return -1;
	}
	if (timer % 175 == 0)
		Log(Format("$Instr$ %d", iOil));
	return 1;
}

global func CountMatRegion(int mat, int x1, int y1, int x2, int y2)
{
	var count = 0;
	var x, y;
	for (x = x1; x <= x2; x++)
		for (y = y1; y <= y2; y++)
			if (GetMaterial(x, y) == mat)
				count++;
	return count;
}
