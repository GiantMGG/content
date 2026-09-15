/*-- Quench: the Water room. Lava pool in a basalt basin; the water  */
/* tap's stream quenches it to Rock + Steam (Rate=100). Mastery: the */
/* ENTIRE pool is quenched.                                           */

#strict 2

static g_iRock0;
static g_iLava0;

protected func Initialize()
{
	// Basin: rock box x[100,300] y[60,160]; lava fill x[110,290] y[130,150].
	DrawMaterialQuad("Rock", 100, 60, 110, 60, 110, 160, 100, 160);
	DrawMaterialQuad("Rock", 290, 60, 300, 60, 300, 160, 290, 160);
	DrawMaterialQuad("Rock", 100, 150, 300, 150, 300, 160, 100, 160);
	DrawMaterialQuad("Lava", 110, 130, 290, 130, 290, 150, 110, 150);

	g_iRock0 = CountMatRegion(Material("Rock"), 100, 60, 300, 160);
	g_iLava0 = CountMatRegion(Material("Lava"), 100, 60, 300, 160);

	// Water tap on the ground beside the basin; nozzle aims over the rim.
	var pTap = CreateObject(RTAP, 330, 185, NO_OWNER);
	pTap->SetAction("Idle");
	pTap->SetTapMaterial("Water");
	pTap->SetTapAmount(15);
	pTap->SetTapOffset(-60, -80);

	AddEffect("MasteryWatch", 0, 1, 35, 0, 0);
	Log("$Intro$");
	return true;
}

global func FxMasteryWatchTimer(object target, int effect, int timer)
{
	var iLava = CountMatRegion(Material("Lava"), 100, 60, 300, 160);
	if (timer > 35 && iLava == 0)
	{
		Log("$Mastery$");
		GameOver();
		return -1;
	}
	if (timer % 175 == 0)
		Log(Format("$Instr$ %d", iLava));
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
