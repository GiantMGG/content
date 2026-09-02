/*-- FeuerstaudammMirrorSmoke.c4s — cycle-90 symptom-6 integration —*/
/* mirror (spec §4.6): 1:1 headless mirror of the shipped           */
/* Feuerstaudamm.c4s. MaxPlayer=0 (Player1/Crew dropped).           */
#strict 2

static const FinalStep = 9;
static g_iStep;
static g_fCalibrate;
static g_iLava0;

protected func Initialize()
{
	var dir = CreateObject(CDIR, 0, 0, NO_OWNER);
	dir->SetGroundRow(300);
	dir->SetLaunchStorm(true);
	dir->SetSeedSea(true);
	dir->PaintCascade(300);
	CreateObject(SCRP, 84, 250, NO_OWNER);
	CreateObject(SCRP, 87, 250, NO_OWNER);
	CreateObject(SDRF, 60, 245, NO_OWNER);
	SetWind(20);
	g_iLava0 = MirrorCount(Material("Lava"), 120, 170, 290, 200)
	         + MirrorCount(Material("Ashes"), 120, 170, 290, 200);
	if (g_iLava0 < 4500)
		FatalError(Format("FeuerstaudammMirrorSmoke FAIL: lava reservoir short (%d)", g_iLava0));
	g_iStep = 0;
	g_fCalibrate = 0;
	AddEffect("RunTest", 0, 1, 35, 0, 0);
	return true;
}

global func FxRunTestStart(target, effect, temp) { return 1; }

global func FxRunTestTimer(object target, int effect, int timer)
{
	++g_iStep;
	if (g_iStep == 1)
		DigFreeRect(200, 200, 8, 10);  // keystone band y[200,210) at G=300


	if (g_fCalibrate)
	{
		var dir = FindObject(CDIR);
		var lava = MirrorCount(Material("Lava"), 120, 170, 290, 200)
		         + MirrorCount(Material("Ashes"), 120, 170, 290, 200);
		Log(Format("[CAL] step %d br=%d fl=%d rk=%d rn=%d bu=%d lava=%d", g_iStep,
			dir->IsBreached(), dir->HasFlam(), dir->HasRock(), dir->HasRain(),
			dir->HasBuried(), lava));
	}
	if (!g_fCalibrate && g_iStep >= FinalStep)
	{
		var dir = FindObject(CDIR);
		if (!dir)
			FatalError("FeuerstaudammMirrorSmoke FAIL: no CDIR");
		if (!dir->IsBreached())
			FatalError("FeuerstaudammMirrorSmoke FAIL: breach beat never fired");
		if (!dir->HasFlam())
			FatalError("FeuerstaudammMirrorSmoke FAIL: FLAM beat never fired");
		if (!dir->HasRock())
			FatalError("FeuerstaudammMirrorSmoke FAIL: rock beat never fired");
		if (!dir->HasRain())
			FatalError("FeuerstaudammMirrorSmoke FAIL: rain beat never fired");
		if (!dir->HasBuried())
			FatalError("FeuerstaudammMirrorSmoke FAIL: burial beat never fired");
		var lava = MirrorCount(Material("Lava"), 120, 170, 290, 200)
		         + MirrorCount(Material("Ashes"), 120, 170, 290, 200);
		if (lava * 100 > g_iLava0 * 25)
			FatalError(Format("FeuerstaudammMirrorSmoke FAIL: reservoir pour %d pct < 25 (frozen reservoir)", 100 - lava * 100 / g_iLava0));
		Log("FeuerstaudammMirrorSmoke PASS");
		GameOver();
		return -1;
	}
	return 1;
}

global func MirrorCount(int mat, int x1, int y1, int x2, int y2)
{
	var count = 0, x, y;
	for (x = x1; x <= x2; x++)
		for (y = y1; y <= y2; y++)
			if (GetMaterial(x, y) == mat) count++;
	return count;
}
