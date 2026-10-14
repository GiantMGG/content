/*-- PaintPinSmoke.c4s — pins the EM paint tools via the EMDrawTool seam. --*/
/* Landscape: shipped Map.bmp => C4LSC_Static; sky above terrain top y=150. */
/* Steps: 1 Brush, 2 Line, 3 Rect, 4 Fill, 5 mode-mismatch negative        */
/* control (brush stamped C4LSC_Exact over a C4LSC_Static landscape must   */
/* be swallowed by C4ControlEMDrawTool::Execute's mode guard).             */

#strict 2

static g_iStep;
static g_matRock;
static g_matEarth;
static g_matGold;

protected func Initialize()
{
	g_matRock = Material("Rock");
	g_matEarth = Material("Earth");
	g_matGold = Material("Gold");
	if (g_matRock == -1)
		FatalError("PaintPinSmoke FAIL: precondition Material(Rock) == -1");
	if (g_matEarth == -1)
		FatalError("PaintPinSmoke FAIL: precondition Material(Earth) == -1");
	if (g_matGold == -1)
		FatalError("PaintPinSmoke FAIL: precondition Material(Gold) == -1");
	Log(Format("PaintPinSmoke precondition: Rock=%d Earth=%d Gold=%d base(200,100)=%d base(400,200)=%d",
	           g_matRock, g_matEarth, g_matGold, GetMaterial(200, 100), GetMaterial(400, 200)));
	g_iStep = 0;
	AddEffect("RunTest", 0, 1, 35, 0, 0);
	return true;
}

global func FxRunTestStart(target, effect, temp) { return 1; }

global func FxRunTestTimer(object target, int effect, int timer)
{
	++g_iStep;
	if (g_iStep == 1)
	{
		/* Brush: grade 30, MapZoom 10 => map radius max(2*30/10,1)=6 cells */
		EMDrawTool(1, 200, 100, 0, 0, 30, 1, "Rock", "Rock", -1);
		if (GetMaterial(200, 100) != g_matRock)
			FatalError(Format("PaintPinSmoke FAIL step 1: Brush probe (200,100)=%d expected Rock=%d",
			                  GetMaterial(200, 100), g_matRock));
		Log("PaintPinSmoke step 1: Brush painted Rock at (200,100)");
	}
	else if (g_iStep == 2)
	{
		EMDrawTool(3, 700, 100, 800, 300, 30, 1, "Rock", "Rock", -1);
		if (GetMaterial(750, 200) != g_matRock)
			FatalError(Format("PaintPinSmoke FAIL step 2: Line probe (750,200)=%d expected Rock=%d",
			                  GetMaterial(750, 200), g_matRock));
		Log("PaintPinSmoke step 2: Line painted Rock (700,100)-(800,300)");
	}
	else if (g_iStep == 3)
	{
		EMDrawTool(4, 100, 250, 300, 350, 30, 1, "Rock", "Rock", -1);
		if (GetMaterial(200, 300) != g_matRock)
			FatalError(Format("PaintPinSmoke FAIL step 3: Rect probe (200,300)=%d expected Rock=%d",
			                  GetMaterial(200, 300), g_matRock));
		Log("PaintPinSmoke step 3: Rect painted Rock (100,250)-(300,350)");
	}
	else if (g_iStep == 4)
	{
		/* Fill: jitter loop scatters iGrade inserts within +-iGrade/2
		   (C4Control.cpp EMDT_Fill). All jitter positions (y 170..229) are
		   inside solid Earth (terrain top y=150), so every insert migrates
		   up through the equal-density Earth and lands as a static Gold
		   pixel on the surface band (y ~147..149) in the same tick. */
		var cnt = 0, x, y;
		EMDrawTool(2, 500, 200, 0, 0, 60, 0, "Gold", "", -1);
		for (x = 460; x <= 540; x++)
			for (y = 140; y <= 165; y++)
				if (GetMaterial(x, y) == g_matGold)
					cnt++;
		Log(Format("PaintPinSmoke step 4: Fill landed %d Gold pixels in (460..540,140..165)", cnt));
		if (cnt < 30)
			FatalError(Format("PaintPinSmoke FAIL step 4: Fill Gold count %d < 30 in landing band", cnt));
	}
	else if (g_iStep == 5)
	{
		EMDrawTool(1, 400, 200, 0, 0, 30, 1, "Rock", "Rock", 3);
		if (GetMaterial(400, 200) != g_matEarth)
			FatalError(Format("PaintPinSmoke FAIL step 5: mode-mismatch Brush leaked (400,200)=%d expected Earth=%d",
			                  GetMaterial(400, 200), g_matEarth));
		Log("PaintPinSmoke step 5: mode-mismatch Brush correctly ignored");
		Log("PaintPinSmoke PASS");
		GameOver();
		return -1;
	}
	return 1;
}
