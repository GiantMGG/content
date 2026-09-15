/*-- Pillow: the Lava room. A deep water basin; the lava tap builds a  */
/* walkable Rock crust on the water surface (LSProduct=Rock) - the     */
/* classic pillow-lava land-building trick. Mastery: crust spans the   */
/* basin edge-to-edge.                                                 */

#strict 2

static g_iRock0;

protected func Initialize()
{
	// Basin: rock box x[100,300] y[60,160]; water fill x[110,290] y[130,150].
	DrawMaterialQuad("Rock", 100, 60, 110, 60, 110, 160, 100, 160);
	DrawMaterialQuad("Rock", 290, 60, 300, 60, 300, 160, 290, 160);
	DrawMaterialQuad("Rock", 100, 150, 300, 150, 300, 160, 100, 160);
	DrawMaterialQuad("Water", 110, 130, 290, 130, 290, 150, 110, 150);

	g_iRock0 = CountMatRegion(Material("Rock"), 100, 60, 300, 160);

	// Lava tap; nozzle centered over the basin.
	var pTap = CreateObject(RTAP, 330, 185, NO_OWNER);
	pTap->SetAction("Idle");
	pTap->SetTapMaterial("Lava");
	pTap->SetTapAmount(10);
	pTap->SetTapOffset(-115, -80);

	AddEffect("MasteryWatch", 0, 1, 35, 0, 0);
	Log("$Intro$");
	return true;
}

global func FxMasteryWatchTimer(object target, int effect, int timer)
{
	var iRock = CountMatRegion(Material("Rock"), 100, 60, 300, 160);
	if (timer > 35 && iRock - g_iRock0 >= 150)
	{
		Log("$Mastery$");
		GameOver();
		return -1;
	}
	if (timer % 175 == 0)
		Log(Format("$Instr$ %d", iRock - g_iRock0));
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
