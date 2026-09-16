/*-- Dissolve: the Acid room. Four narrow 10px sample islands -          */
/* Sulphur (Corrode=90), Ice (60), Sand (30) - on overhanging Rock        */
/* pedestals, plus a Granite control (no Corrode -> immune). Twin          */
/* short-range acid tap volleys per island (nozzles aimed at the plate-    */
/* top row, the only delivery that etches under shipped physics) eat the   */
/* soluble plates down to their permanent rim + base crust; the granite    */
/* discriminator stays STRICT (iG == g_iG0).                               */
/*                                                                        */
/* Ceilings, not zeros (defect #17): with shipped physics an acid stream   */
/* stalls at a residual floor - gentle rain never etches (Acid MaxSlide    */
/* = 10000, C4Material.cpp:615) and the deposits seal the pit (static      */
/* acid never corrodes, C4Material.cpp:801 "No corrosion..."), so a        */
/* strict-zero mastery is unreachable through the intended interaction.    */
/* Per-plate calibrated ceilings follow the Oilburn defect-#13 precedent:  */
/* stalls are measured headless across 12 seeds driving the intended       */
/* interaction (open all acid taps), ceiling = max observed x 1.25         */
/* rounded up to a clean multiple of 5. Calibration table (stall S/I/Sa):  */
/*   seeds 100..42: (64,34,33)(67,48,29)(32,56,41)(53,65,6)(39,34,37)      */
/*   (45,57,13)(35,53,48)(76,47,55)(41,65,19)(44,43,21)(38,65,25)(49,54,22) */
/*   -> max (76,65,55) -> ceilings (95,85,70). The I and Sa ceilings are   */
/*   strictly below the 90-px baseline, so leaving the taps closed (all    */
/*   plates at baseline) can never satisfy mastery.                        */

#strict 2

static const DISSOLVE_MAX_SULPHUR = 95;   /* stall max 76 x 1.25 = 95.0 (seed 314)  */
static const DISSOLVE_MAX_ICE     = 85;   /* stall max 65 x 1.25 = 81.25, clean 85 (seeds 2024/2718/9139) */
static const DISSOLVE_MAX_SAND    = 70;   /* stall max 55 x 1.25 = 68.75, clean 70 (seed 314) */

static g_iS0, g_iI0, g_iSa0, g_iG0;
static g_pS, g_pI, g_pSa, g_pG, g_pS2, g_pI2, g_pSa2, g_pG2;

protected func Initialize()
{
	// Sample islands: 10px plate slabs on 6px Rock pedestals. The narrow
	// overhang drains pooled acid off the pit floor into the open 30px
	// gaps on all sides, so no sealing pool can settle on or around the
	// plates (defect #17 "under/beside-island drains").
	DrawMaterialQuad("Rock", 112, 146, 117, 146, 117, 196, 112, 196);
	DrawMaterialQuad("Rock", 152, 146, 157, 146, 157, 196, 152, 196);
	DrawMaterialQuad("Rock", 192, 146, 197, 146, 197, 196, 192, 196);
	DrawMaterialQuad("Rock", 232, 146, 237, 146, 237, 196, 232, 196);
	DrawMaterialQuad("Sulphur", 110, 136, 119, 136, 119, 145, 110, 145);
	DrawMaterialQuad("Ice", 150, 136, 159, 136, 159, 145, 150, 145);
	DrawMaterialQuad("Sand", 190, 136, 199, 136, 199, 145, 190, 145);
	DrawMaterialQuad("Granite", 230, 136, 239, 136, 239, 145, 230, 145);

	g_iS0  = CountMatRegion(Material("Sulphur"), 100, 110, 260, 240);
	g_iI0  = CountMatRegion(Material("Ice"), 100, 110, 260, 240);
	g_iSa0 = CountMatRegion(Material("Sand"), 100, 110, 260, 240);
	g_iG0  = CountMatRegion(Material("Granite"), 100, 110, 260, 240);

	// Twin acid taps per island on the ground right of the islands; a
	// player double-click opens each. The nozzles are aimed after the
	// taps settle (TapAim) so every stream volleys at its plate's top
	// row - the short-range delivery that actually etches (defect #17:
	// gentle rain never etches).
	g_pS   = CreateObject(RTAP, 342, 185, NO_OWNER);
	g_pI   = CreateObject(RTAP, 382, 185, NO_OWNER);
	g_pSa  = CreateObject(RTAP, 422, 185, NO_OWNER);
	g_pG   = CreateObject(RTAP, 462, 185, NO_OWNER);
	g_pS2  = CreateObject(RTAP, 352, 185, NO_OWNER);
	g_pI2  = CreateObject(RTAP, 392, 185, NO_OWNER);
	g_pSa2 = CreateObject(RTAP, 432, 185, NO_OWNER);
	g_pG2  = CreateObject(RTAP, 472, 185, NO_OWNER);
	g_pS->SetAction("Idle");
	g_pS2->SetAction("Idle");
	g_pI->SetAction("Idle");
	g_pI2->SetAction("Idle");
	g_pSa->SetAction("Idle");
	g_pSa2->SetAction("Idle");
	g_pG->SetAction("Idle");
	g_pG2->SetAction("Idle");
	g_pS->SetTapMaterial("Acid");
	g_pS2->SetTapMaterial("Acid");
	g_pI->SetTapMaterial("Acid");
	g_pI2->SetTapMaterial("Acid");
	g_pSa->SetTapMaterial("Acid");
	g_pSa2->SetTapMaterial("Acid");
	g_pG->SetTapMaterial("Acid");
	g_pG2->SetTapMaterial("Acid");
	g_pS->SetTapAmount(500);
	g_pS2->SetTapAmount(500);
	g_pI->SetTapAmount(500);
	g_pI2->SetTapAmount(500);
	g_pSa->SetTapAmount(500);
	g_pSa2->SetTapAmount(500);
	g_pG->SetTapAmount(500);
	g_pG2->SetTapAmount(500);

	AddEffect("TapAim", 0, 1, 35, 0, 0);
	AddEffect("MasteryWatch", 0, 1, 35, 0, 0);
	Log("$Intro$");
	return true;
}

global func FxTapAimStart(object target, int effect, int temp) { return 1; }

// After settle: aim every nozzle so its cast point sits on its plate's
// top row (136). The taps rest on terrain whose height varies 1px per
// column, so the offsets are computed from the settled positions.
global func FxTapAimTimer(object target, int effect, int timer)
{
	if (timer != 35) return 1;
	g_pS->SetTapOffset(112 - g_pS->GetX(), 136 - g_pS->GetY() + 5);
	g_pS2->SetTapOffset(117 - g_pS2->GetX(), 136 - g_pS2->GetY() + 5);
	g_pI->SetTapOffset(152 - g_pI->GetX(), 136 - g_pI->GetY() + 5);
	g_pI2->SetTapOffset(157 - g_pI2->GetX(), 136 - g_pI2->GetY() + 5);
	g_pSa->SetTapOffset(192 - g_pSa->GetX(), 136 - g_pSa->GetY() + 5);
	g_pSa2->SetTapOffset(197 - g_pSa2->GetX(), 136 - g_pSa2->GetY() + 5);
	g_pG->SetTapOffset(232 - g_pG->GetX(), 136 - g_pG->GetY() + 5);
	g_pG2->SetTapOffset(237 - g_pG2->GetX(), 136 - g_pG2->GetY() + 5);
	return -1;
}

global func FxMasteryWatchStart(object target, int effect, int temp) { return 1; }

global func FxMasteryWatchTimer(object target, int effect, int timer)
{
	var iS  = CountMatRegion(Material("Sulphur"), 100, 110, 260, 240);
	var iI  = CountMatRegion(Material("Ice"), 100, 110, 260, 240);
	var iSa = CountMatRegion(Material("Sand"), 100, 110, 260, 240);
	var iG  = CountMatRegion(Material("Granite"), 100, 110, 260, 240);
	if (timer > 105 && iS <= DISSOLVE_MAX_SULPHUR && iI <= DISSOLVE_MAX_ICE &&
	    iSa <= DISSOLVE_MAX_SAND && iG == g_iG0)
	{
		Log("$Mastery$");
		GameOver();
		return -1;
	}
	if (timer % 175 == 0)
		Log(Format("$Instr$ S%d I%d Sa%d G%d", iS, iI, iSa, iG));
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
