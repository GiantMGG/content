/*-- ClimbDigRelease.c4s -- jump-and-run control pin (climb-drop-fix).

  Proves: a jump-and-run (AutoStop) controlled clonk in DFA_SCALE holding
  a flint that receives COM_Dig lets go of the wall (1) without dropping
  the flint (2) and without a delayed spurious dig firing on the released
  clonk (3). No "Smoke" suffix on purpose: the tests/CMakeLists.txt
  auto-glob must not double-register this scenario (the player-fixture
  smoke is registered explicitly as climb_dig_release_smoke). --*/

#strict 2

static g_iStage;       // 0=join-wait, 1=settle, 2=observe
static g_iWait;        // per-stage frame counter
static g_pClonk;       // the crew clonk under test
static g_pFlint;       // the flint it holds
static g_fSpuriousDig; // saw procedure DIG after the release

static func PrepareSite()
{
	// Solid earth floor, top edge at y=280, under the whole test site
	DrawMaterialQuad("Earth", 380, 280, 620, 280, 620, 320, 380, 320, false);
	// Solid earth wall, left face at x=500, rising from the floor
	DrawMaterialQuad("Earth", 500, 120, 560, 120, 560, 300, 500, 300, false);
	// Carve the standing pocket to free space (base terrain independent)
	DrawMaterialQuad("Earth", 430, 150, 500, 150, 500, 279, 430, 279, true);
	return true;
}

protected func Initialize()
{
	g_iStage = 0; g_iWait = 0;
	g_pClonk = 0; g_pFlint = 0; g_fSpuriousDig = false;
	PrepareSite();
	AddEffect("RunTest", 0, 1, 1); // per-frame driver
	return true;
}

global func FxRunTestTimer(target, effect, time)
{
	var plr;
	// Stage 0: wait for the fixture player and its cursor clonk to join
	if (g_iStage == 0)
	{
		if (time > 200)
			FatalError("ClimbDigRelease FAIL(setup): player did not join in time");
		plr = GetPlayerByIndex(0);
		if (plr == NO_OWNER) return 1;
		g_pClonk = GetCursor(plr);
		if (!g_pClonk) return 1;
		// Teleport the clonk to the prepared wall face and force Scale
		g_pClonk->SetPosition(493, 270);
		g_pClonk->SetDir(DIR_Right);
		g_pClonk->SetComDir(COMD_Stop);
		g_pClonk->SetAction("Scale");
		g_pFlint = g_pClonk->CreateContents(FLNT);
		if (!g_pFlint)
			FatalError("ClimbDigRelease FAIL(setup): no flint created");
		Log(Format("ClimbDigRelease setup: action=%s procedure=%s",
			g_pClonk->GetAction(), GetProcedure(g_pClonk)));
		g_iStage = 1; g_iWait = 0;
		return 1;
	}
	// Stage 1: give the forced Scale action a few frames to settle
	if (g_iStage == 1)
	{
		g_iWait++;
		if (g_iWait < 5) return 1;
		if (GetProcedure(g_pClonk) != "SCALE")
			FatalError(Format("ClimbDigRelease FAIL(setup): Scale did not stick (procedure=%s)",
				GetProcedure(g_pClonk)));
		plr = GetPlayerByIndex(0);
		// Inject a raw COM_Dig. Engine com values are not script constants:
		// COM_Dig = 6 (src/C4Constants.h). Never inject _S/_D synthetics.
		if (!SimulatePlrCom(plr, 6))
			FatalError("ClimbDigRelease FAIL(setup): SimulatePlrCom rejected the com");
		Log("ClimbDigRelease: COM_Dig injected");
		g_iStage = 2; g_iWait = 0;
		return 1;
	}
	// Stage 2: observe the aftermath frame by frame
	if (g_iStage == 2)
	{
		g_iWait++;
		// (1) released-from-scale: no longer scaling shortly after the com
		if (g_iWait == 8)
			if (GetProcedure(g_pClonk) == "SCALE")
				FatalError("ClimbDigRelease FAIL(release): clonk still scaling after COM_Dig");
		// (2) item-retained: the flint was not dropped
		if (g_iWait == 8)
			if (g_pClonk->Contents(0) != g_pFlint)
				FatalError("ClimbDigRelease FAIL(item): flint dropped on let-go");
		// (3) spurious-dig watch: C4DoubleClick (=10) window and beyond
		if (g_iWait >= 10 && g_iWait <= 45)
			if (GetProcedure(g_pClonk) == "DIG")
				g_fSpuriousDig = true;
		if (g_iWait >= 45)
		{
			if (g_fSpuriousDig)
				FatalError("ClimbDigRelease FAIL(spurious): delayed dig fired on released clonk");
			Log("ClimbDigRelease PASS");
			GameOver();
			return -1;
		}
		return 1;
	}
	return 1;
}
