/*-- Duneworks: the Sand room. The sand tap feeds a raised sand-slab   */
/* runway; wind 80 (vane) saltates the grains east along the slab top  */
/* and off its open east edge into the lee, where a deposition dune     */
/* accumulates around the granite boulder. Mastery: the lee band east   */
/* of the slab's east edge holds >= 40 static sand px.                  */
/*                                                                     */
/* Runway class (defect #18): a slab-runway with an OPEN EAST EDGE,     */
/* the validated DuneBurial / ReactionLabSmoke beat-4 saltation         */
/* pattern - never a self-built ramp against a wall (the original       */
/* boulder-in-the-open geometry stalled at a 0-px deposition zone).     */

#strict 2

static const DW_DEPOSIT_MIN = 40;

protected func Initialize()
{
	// Raised sand-slab runway x[240,340]: top surface y150, base on the
	// earth top (y200). Grains saltating east fall off the open east
	// edge (x340) into the lee band below/behind it.
	DrawMaterialQuad("Sand", 240, 150, 340, 150, 340, 200, 240, 200, false);

	// Scenery: granite boulder in the lee band, east of the slab edge.
	DrawMaterialQuad("Granite", 380, 165, 400, 165, 400, 200, 380, 200);

	// Sand tap (west, on the ground) feeding the slab's west top; the
	// nozzle hangs ~30px above the slab so the wind-stream spreads the
	// falling grains across the upwind slab top.
	var pSand = CreateObject(RTAP, 210, 185, NO_OWNER);
	pSand->SetAction("Idle");
	pSand->SetTapMaterial("Sand");
	pSand->SetTapAmount(20);
	pSand->SetTapOffset(30, -73);
	// Wind vane (east of the scene), wind 80 as planned.
	var pVane = CreateObject(RTAP, 530, 185, NO_OWNER);
	pVane->SetAction("Idle");
	pVane->SetTapVane(80);
	pVane->SetTapAmount(0);

	AddEffect("MasteryWatch", 0, 1, 35, 0, 0);
	Log("$Intro$");
	return true;
}

global func FxMasteryWatchTimer(object target, int effect, int timer)
{
	var iDune = CountMatRegion(Material("Sand"), 345, 150, 520, 200);
	if (timer > 105 && iDune >= DW_DEPOSIT_MIN)
	{
		Log("$Mastery$");
		GameOver();
		return -1;
	}
	if (timer % 175 == 0)
		Log(Format("$Instr$ %d", iDune));
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
