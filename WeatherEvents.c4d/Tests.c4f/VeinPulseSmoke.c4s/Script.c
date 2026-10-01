#strict 2

/*-- VeinPulseSmoke: mirror of Embervein's vein pulse (cycle 185).
     Burn/cool scaled to 2/1 timer ticks (shipped: 20/10 at 35f each);
     scan (grid step 3), damage (40/tick), searing (VeinRich on free GOLD
     in the burning branch) mirror Embervein.c4s/Script.c — KEEP ALIGNED,
     diff the mirrored blocks on any change. Session 2 appends the chest
     fold + first-to-target self-end asserts. --*/

static g_Step, g_Phase, g_Done;
static g_T;
static g_MPhase, g_MBranch, g_MTick, g_MRects;   // mirror pulse state
static g_ClonkIn, g_ClonkOut, g_E0In, g_E0Out;

protected func Initialize()
{
	g_Step = 0; g_Phase = 0; g_Done = 0; g_T = 0;
	MirrorScanBranches();
	g_MPhase = 0; g_MBranch = 0; g_MTick = 0;
	AddEffect("RunTest", 0, 1, 35);
	return true;
}

global func FxRunTestTimer(target, effect, time)
{
	++g_Step;
	if (g_Phase == 0) return MirrorStepSetup();
	if (g_Phase == 1) return MirrorStepTransitions();
	return 1;
}

// MIRROR of VeinScanBranches (same grid step 3), hard-fatal when the scan
// finds nothing (the shipped fallback does not apply to a test fixture).
global func MirrorScanBranches()
{
	var minX = -1, minY = -1, maxX = -1, maxY = -1;
	var x, y;
	for (x = 0; x < LandscapeWidth(); x += 3)
		for (y = 0; y < LandscapeHeight(); y += 3)
			if (GetMaterial(x, y) == Material("Gold"))
			{
				if (minX < 0 || x < minX) minX = x;
				if (minY < 0 || y < minY) minY = y;
				if (maxX < 0 || x > maxX) maxX = x;
				if (maxY < 0 || y > maxY) maxY = y;
			}
	if (minX < 0)
		FatalError("VeinPulseSmoke FAIL: gold scan found no gold in the landscape");
	var bx = minX, by = minY, bw = maxX - minX + 1, bh = maxY - minY + 1;
	var third = bh / 3;
	g_MRects = [
		bx, by,             bw, third,
		bx, by + third,     bw, third,
		bx, by + 2*third,   bw, bh - 2*third ];
	return 1;
}

// MIRROR of VeinBurnBranch (same damage constant, same filters).
global func MirrorBurnBranch(int b)
{
	var rx = g_MRects[b*4 + 0], ry = g_MRects[b*4 + 1];
	var rw = g_MRects[b*4 + 2], rh = g_MRects[b*4 + 3];
	var i, obj;
	var clonks = FindObjects(Find_InRect(rx, ry, rw, rh), Find_OCF(OCF_Alive), Find_NoContainer());
	for (i = 0; i < GetLength(clonks); i++)
	{
		obj = clonks[i];
		obj->DoEnergy(-40);
		obj->Incinerate();
		CastParticles("MSpark", 10, 25, GetX(obj), GetY(obj), 15, 15);
	}
	var gold = FindObjects(Find_ID(GOLD), Find_InRect(rx, ry, rw, rh), Find_NoContainer());
	for (i = 0; i < GetLength(gold); i++)
	{
		if (!GetEffect("VeinRich", gold[i]))
		{
			AddEffect("VeinRich", gold[i], 2);
			Log(Format("VeinPulseSmoke: seared gold on branch %d", b + 1));
		}
	}
	return 1;
}

// MIRROR pulse timer, scaled: burn 2 ticks, cool 1 tick.
global func MirrorPulseTick()
{
	++g_MTick;
	if (g_MPhase == 0)
	{
		if (g_MTick > 2)
		{
			g_MPhase = 1; g_MTick = 0;
			Log(Format("VeinPulseSmoke: branch %d cools", g_MBranch + 1));
		}
		else MirrorBurnBranch(g_MBranch);
	}
	else if (g_MTick > 1)
	{
		g_MPhase = 0; g_MTick = 0;
		g_MBranch = Random(3);
		Log(Format("VeinPulseSmoke: branch %d ignites", g_MBranch + 1));
	}
	return 1;
}

global func MirrorStepSetup()
{
	var i;
	// 1) scan found a gold bbox: all twelve branch-rect entries positive
	for (i = 0; i < 12; i++)
		if (g_MRects[i] <= 0)
			FatalError("VeinPulseSmoke FAIL: branch rect entry not positive");
	// 2) stage clonks + nuggets in/out of branch 0's rect
	var rx = g_MRects[0], ry = g_MRects[1], rw = g_MRects[2], rh = g_MRects[3];
	var cx = rx + rw/2, cy = ry + rh/2;
	g_ClonkIn = CreateObject(CLNK, cx, cy, NO_OWNER);
	g_ClonkOut = CreateObject(CLNK, 30, 30, NO_OWNER);
	var nugIn = CreateObject(GOLD, cx, cy + 10, NO_OWNER);
	var nugOut = CreateObject(GOLD, 30, 40, NO_OWNER);
	if (!g_ClonkIn || !g_ClonkOut || !nugIn || !nugOut)
		FatalError("VeinPulseSmoke FAIL: staging failed");
	g_E0In = GetEnergy(g_ClonkIn);
	g_E0Out = GetEnergy(g_ClonkOut);
	// 3) one burn tick: in-branch nugget seared, out-of-branch not; the
	//    in-burn clonk loses >= 40 energy (the DoDamage constant), the
	//    out-of-burn clonk loses nothing
	MirrorBurnBranch(0);
	if (!GetEffect("VeinRich", nugIn))
		FatalError("VeinPulseSmoke FAIL: in-branch nugget not seared");
	if (GetEffect("VeinRich", nugOut))
		FatalError("VeinPulseSmoke FAIL: out-of-branch nugget wrongly seared");
	if (g_E0In - GetEnergy(g_ClonkIn) < 40)
		FatalError(Format("VeinPulseSmoke FAIL: in-burn damage %d < 40", g_E0In - GetEnergy(g_ClonkIn)));
	if (GetEnergy(g_ClonkOut) != g_E0Out)
		FatalError("VeinPulseSmoke FAIL: out-of-burn clonk damaged");
	// 4) two more burn ticks: ~3 s of exposure kills the in-burn clonk
	MirrorBurnBranch(0);
	MirrorBurnBranch(0);
	if (GetAlive(g_ClonkIn))
		FatalError("VeinPulseSmoke FAIL: in-burn clonk survived 3 ticks - damage constant too low");
	if (!GetAlive(g_ClonkOut))
		FatalError("VeinPulseSmoke FAIL: out-of-burn clonk died");
	Log("VeinPulseSmoke step1: scan/damage/seared asserts green");
	g_Phase = 1;
	return 1;
}

global func MirrorStepTransitions()
{
	++g_T;
	MirrorPulseTick();
	// burn 2 ticks then flip to cool (g_T==3); cool 1 tick (g_T==4) then
	// flip back to a fresh Random(3) burn branch (g_T==5). The cool limit is
	// `> 1` mirroring the shipped `> 10` (10:1 scaling, same operator), so
	// the 1 cool tick completes at g_T==4 and the flip is OBSERVED at g_T==5.
	if (g_T == 3 && g_MPhase != 1)
		FatalError("VeinPulseSmoke FAIL: no burn->cool transition after 2 burn ticks");
	if (g_T == 5 && g_MPhase != 0)
		FatalError("VeinPulseSmoke FAIL: no cool->burn transition after 1 cool tick");
	if (g_T >= 5)
	{
		Log("VeinPulseSmoke PASS");
		GameOver();
	}
	return 1;
}
