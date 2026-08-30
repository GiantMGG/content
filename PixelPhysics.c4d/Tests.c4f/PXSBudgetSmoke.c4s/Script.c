/*-- PXSBudgetSmoke.c4s — headless PXS budget stress smoke. --*/
/*                                                             */
/* Guards cycle 75 spec §4.1: sustained water-PXS spam onto a  */
/* static lava field drives the water -> steam -> condensation */
/* -> rain -> insert cycle for the full 350-tick window. The   */
/* in-flight population must stay within the 10k hard budget   */
/* at every sampled checkpoint and the engine must stay alive  */
/* (the FAIL_REGULAR_EXPRESSION FatalError|[error]|[fatal]     */
/* backstop catches crashes).                                   */
/*                                                             */
/* Spawn math: 50 PXS/frame x ~315 frames ~ 15,750 spawns -    */
/* far beyond the 10k cap cumulatively; steady-state in-flight */
/* population ~ spawn_rate x resolution_time ~ 50 x ~70 ticks  */
/* ~ 3,500, comfortably under the cap.                          */
/*                                                             */
/* Driver note: the per-step timers are GLOBAL effects with    */
/* GLOBAL callbacks - the plain AddEffect(this) pattern does   */
/* not resolve callbacks in scenario scripts. Objects.c4d is   */
/* loaded so System.c4g links cleanly.                          */
/*                                                             */
/* On any assertion failure, FatalError produces a non-zero    */
/* exit code, failing the CTest entry.                          */

#strict 2

static g_iStep;

protected func Initialize()
{
	// Wide basin: rock box x[100,500] y[60,160] with a static lava
	// fill x[150,450] y[135,150] (300x15 = 4500 lava pixels).
	DrawMaterialQuad("Rock", 100, 60, 110, 60, 110, 160, 100, 160);
	DrawMaterialQuad("Rock", 490, 60, 500, 60, 500, 160, 490, 160);
	DrawMaterialQuad("Rock", 100, 150, 500, 150, 500, 160, 100, 160);
	DrawMaterialQuad("Lava", 150, 135, 450, 135, 450, 150, 150, 150);

	g_iStep = 0;
	AddEffect("RunTest", 0, 1, 35, 0, 0);
	AddEffect("PxsSpam", 0, 1, 1, 0, 0);
	return true;
}

global func FxRunTestStart(target, effect, temp) { return 1; }

global func FxPxsSpamTimer(object target, int effect, int timer)
{
	// 50 water PXS per frame, spread over the lava field.
	CastPXS("Water", 50, 100, 300, 100);
	return 1;
}

global func FxRunTestTimer(object target, int effect, int timer)
{
	++g_iStep;

	// Budget guard at every checkpoint.
	if (GetPXSCount() > 10000)
		FatalError(Format("PXSBudgetSmoke FAIL step %d: PXS budget exceeded (%d)",
		                  g_iStep, GetPXSCount()));
	// Steam stays PXS-only even under spam.
	if (GetMaterialCount(Material("Steam")) != 0)
		FatalError(Format("PXSBudgetSmoke FAIL step %d: static steam appeared (%d)",
		                  g_iStep, GetMaterialCount(Material("Steam"))));

	if (g_iStep >= 9)
	{
		Log("PXSBudgetSmoke PASS");
		GameOver();
		return -1;
	}
	return 1;
}
