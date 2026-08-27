/*-- PickerSmoke.c4s -- headless smoke test for ObjectsAppend --*/

#strict 2

/* C4ID constants the scenario asserts against. */
static const C4ID OAEN = C4Id("OAEN");
static const C4ID OAGR = C4Id("OAGR");
static const C4ID OACH = C4Id("OACH");
static const C4ID OABD = C4Id("OABD");
static const C4ID OALN = C4Id("OALN");
static const C4ID OAPP = C4Id("OAPP");

static g_iStep;

protected func Initialize()
{
	g_iStep = 0;
	AddEffect("RunTest", this, 1, 35, this);
	return true;
}

func FxRunTestStart(target, effect, temp) { return 1; }

func FxRunTestTimer(object target, int effect, int timer)
{
	if (g_iStep == 0)
	{
		/* Spawn the master rule + all 5 child rules in the section. */
		CreateObject(OAPP, 0, 0, NO_OWNER);
		CreateObject(OAEN,  0, 0, NO_OWNER);
		CreateObject(OAGR,  0, 0, NO_OWNER);
		CreateObject(OACH,  0, 0, NO_OWNER);
		CreateObject(OABD,  0, 0, NO_OWNER);
		CreateObject(OALN,  0, 0, NO_OWNER);

		var pClonk = GetHiRank(GetPlayerByIndex(0));
		if (!pClonk) FatalError("PickerSmoke FAIL step 0: no clonk spawned");
		var pMaster = FindObject(OAPP);
		if (!pMaster) FatalError("PickerSmoke FAIL step 0: OAPP not in section");

		/* Right-click self: GetContextMenuItems(pTarget=pClonk, pCaller=pClonk). */
		var items = pMaster->~GetContextMenuItems(pClonk, pClonk);
		if (GetLength(items) != 5)
			FatalError(Format("PickerSmoke FAIL step 0: expected 5 items got %d", GetLength(items)));
		Log("PickerSmoke step 0 PASS: 5 context items aggregated");
	}

	if (g_iStep == 1)
	{
		/* Chop family: place 3 trees within reach of the clonk, assert >=2 -> "..." suffix. */
		var pClonk = GetHiRank(GetPlayerByIndex(0));
		var t1 = CreateObject(TRE1, GetX(pClonk) - 15, GetY(pClonk) - 10, NO_OWNER);
		var t2 = CreateObject(TRE1, GetX(pClonk) + 15, GetY(pClonk) - 10, NO_OWNER);
		var t3 = CreateObject(TRE1, GetX(pClonk) +  5, GetY(pClonk) - 20, NO_OWNER);
		if (!t1 || !t2 || !t3) FatalError("PickerSmoke FAIL step 1: could not spawn trees");

		var pChop = FindObject(OACH);
		var items = pChop->~OA_GetItems(pClonk, pClonk);
		if (GetLength(items) != 1)
			FatalError(Format("PickerSmoke FAIL step 1: expected 1 tuple got %d", GetLength(items)));
		var cap = items[0][0];
		/* >=2 candidates -> caption is the "$CtxChop$" stringtbl entry "Chop..." */
		if (!WildcardMatch("*...*", cap) && !WildcardMatch("*…*", cap))
			FatalError(Format("PickerSmoke FAIL step 1: caption '%s' missing ellipsis", cap));
		Log("PickerSmoke step 1 PASS: Chop tuple has ellipsis for >=2 candidates");
	}

	if (g_iStep == 2)
	{
		/* Edge case #5: candidate disappears mid-menu. */
		var pClonk = GetHiRank(GetPlayerByIndex(0));
		var pChop  = FindObject(OACH);
		var trees  = FindObjects(TRE1);
		if (GetLength(trees) < 2) FatalError("PickerSmoke FAIL step 2: not enough trees");
		var t2 = trees[1];
		RemoveObject(t2);
		/* OA_Fire on the removed target must return false. */
		var rc = OA_Fire(pChop, t2, "Chop", pClonk);
		if (rc != false)
			FatalError(Format("PickerSmoke FAIL step 2: OA_Fire returned %v for dead target", rc));
		Log("PickerSmoke step 2 PASS: OA_Fire rejects vanished target");
	}

	if (g_iStep == 3)
	{
		Log("PickerSmoke PASS");
		GameOver();
		return -1;
	}
	++g_iStep;
	return 1;
}
