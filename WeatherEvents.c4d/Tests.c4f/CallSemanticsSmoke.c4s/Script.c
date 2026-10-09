/*-- CallSemanticsSmoke.c4s -- headless C4CMD_Call semantics pin. --*/
/*                                                                  */
/* Pins the honest C4CMD_Call idiom documented at C4Command::Call() */
/* (cycle 212, chore call-return-failed-propagation):               */
/*   PIN A (load-bearing): a normal-path Call finishes with success */
/*     BEFORE the callee runs, and the callee's return value is     */
/*     never read. SetCommand(clonk,"Call",target,0,0,nil,"FnX")    */
/*     where FnX returns false must still exit the queue cleanly,   */
/*     log CX:ran, and NEVER fire ~FnXFailed (CX:failed absent).    */
/*   PIN B: the degenerate no-target Call (Target=nil) drains with  */
/*     the standard localized failure message; CallFailed() early-  */
/*     outs so ~FnXFailed still never fires, and no [error] logs.   */
/*                                                                  */
/* The scenario-local CTGT def hosts FnX/~FnXFailed (BOIL pattern). */
/* On any assertion failure, FatalError produces a non-zero exit    */
/* code, failing the CTest entry (AGENTS.md smoke contract).        */

#strict 3

static g_iStep;
static g_clonk;
static g_target;

global func SmokeSurfaceY(int x)
{
	var y = 10;
	while (y < LandscapeHeight() - 5 && !GBackSolid(x, y)) y++;
	return y;
}

protected func Initialize()
{
	g_iStep = 0;
	StepFixtures();
	AddEffect("RunTest", this, 1, 35, this);
	return true;
}

global func FxRunTestTimer(target, effect, time)
{
	++g_iStep;
	if (g_iStep == 1) return StepIssueA();
	if (g_iStep == 2) return StepAssertA();
	if (g_iStep == 3) return StepAssertB();
	if (g_iStep == 4) return StepPass();
	return -1;
}

/* Step 0 (in Initialize): fixtures -- a clonk and the CTGT target */
/* on a dry solid-surface spot, clear of map edges.                */
global func StepFixtures()
{
	var gx = LandscapeWidth() / 2;
	var gy = 0;
	var tr;
	for (tr = 0; tr < 16; tr++)
	{
		gy = SmokeSurfaceY(gx);
		if (gy < 5 || gy >= LandscapeHeight() - 5) { gx += 40; continue; }
		if (GBackLiquid(gx, gy - 4)) { gx += 40; continue; }
		break;
	}
	if (!(gx < LandscapeWidth() - 60))
		FatalError("CallSemanticsSmoke FAIL step 0: no dry surface spot found");

	g_clonk = CreateObject(CLNK, gx, gy, NO_OWNER);
	if (!g_clonk)
		FatalError("CallSemanticsSmoke FAIL step 0: could not spawn clonk (CLNK)");
	g_target = CreateObject(CTGT, gx + 20, gy, NO_OWNER);
	if (!g_target)
		FatalError("CallSemanticsSmoke FAIL step 0: could not spawn call target (CTGT)");

	Log(Format("CSMK:fixtures clonk=(%d,%d) target=(%d,%d)", gx, gy, gx + 20, gy));
	return true;
}

/* PIN A, issue leg: normal-path Call, callee returns false. */
global func StepIssueA()
{
	SetCommand(g_clonk, "Call", g_target, 0, 0, nil, "FnX");
	Log("CSMK:issued PIN A Call(FnX)");
	return true;
}

/* PIN A, assert leg: FnX ran (CX:ran), ~FnXFailed absent, queue drained. */
global func StepAssertA()
{
	if (!g_target->LocalN("fRan"))
		FatalError("CallSemanticsSmoke FAIL step A: FnX did not run (no CX:ran)");
	if (g_target->LocalN("fFailed"))
		FatalError("CallSemanticsSmoke FAIL step A: ~FnXFailed fired although Finish(true) precedes the callee (CX:failed present)");
	if (GetCommand(g_clonk, 0, 0) != nil)
		FatalError("CallSemanticsSmoke FAIL step A: Call command left the clonk's queue (GetCommand not 0)");
	Log("CSMK:step A holds -- ran, no failed, queue drained");
	/* PIN B, issue leg: degenerate no-target Call. */
	SetCommand(g_clonk, "Call", nil, 0, 0, nil, "FnX");
	Log("CSMK:issued PIN B Call(FnX) with no target");
	return true;
}

/* PIN B, assert leg: drains, ~FnXFailed still absent, queue empty. */
global func StepAssertB()
{
	if (GetCommand(g_clonk, 0, 0) != nil)
		FatalError("CallSemanticsSmoke FAIL step B: no-target Call did not drain");
	if (g_target->LocalN("fFailed"))
		FatalError("CallSemanticsSmoke FAIL step B: ~FnXFailed fired on no-target Call (CallFailed early-out broken)");
	if (g_target->LocalN("fRanCount") != 1)
		FatalError("CallSemanticsSmoke FAIL step B: FnX ran on the no-target Call (should drain without callee)");
	Log("CSMK:step B holds -- drained, no failed, queue empty");
	return true;
}

/* Step 4: PASS. */
global func StepPass()
{
	Log("CallSemanticsSmoke PASS");
	GameOver();
	return true;
}
