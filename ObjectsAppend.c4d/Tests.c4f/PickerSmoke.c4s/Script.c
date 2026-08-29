/*-- PickerSmoke.c4s -- headless smoke test for ObjectsAppend --*/

#strict 2

/* Runtime smoke test for the ObjectsAppend picker system.  Runs
   synchronously in Initialize() (no timer) to avoid the
   GameOverCheck-with-no-players issue that kills timer-based
   scenarios before frame 35. */

static g_iStep;

protected func Initialize()
{
	g_iStep = 0;
	RunSmokeSteps();
	return true;
}

func RunSmokeSteps()
{
	/* Step 0: spawn master rule + all 5 child rules in the section.
	   OALN is a v2 placeholder (not aggregated by OAPP) but spawned
	   here for completeness. */
	CreateObject(OAPP, 0, 0, NO_OWNER);
	CreateObject(OAEN,  0, 0, NO_OWNER);
	CreateObject(OAGR,  0, 0, NO_OWNER);
	CreateObject(OACH,  0, 0, NO_OWNER);
	CreateObject(OABD,  0, 0, NO_OWNER);
	CreateObject(OALN,  0, 0, NO_OWNER);

	var pClonk = CreateObject(CLNK, 50, 50, NO_OWNER);
	if (!pClonk) FatalError("PickerSmoke FAIL step 0: could not spawn CLNK");
	var pMaster = FindObject(OAPP);
	if (!pMaster) FatalError("PickerSmoke FAIL step 0: OAPP not in section");

	/* No candidates have been placed near the clonk yet, so the master
	   aggregates zero items (spec Edge case #1: 0 candidates -> family
	   tuple absent). */
	var items = pMaster->~GetContextMenuItems(pClonk, pClonk);
	if (GetLength(items) != 0)
		FatalError(Format("PickerSmoke FAIL step 0: expected 0 items got %d", GetLength(items)));
	Log("PickerSmoke step 0 PASS: 0 context items with no candidates");

	/* Step 1: Chop family: place 3 trees within reach of the clonk,
	   assert >=2 -> "..." suffix.  Trees have a large y-offset (~35px
	   upward), so create them near the clonk's y so their actual
	   position falls within the 40px chop reach. */
	var t1 = CreateObject(TRE1, GetX(pClonk) - 3, GetY(pClonk), NO_OWNER);
	var t2 = CreateObject(TRE1, GetX(pClonk) + 3, GetY(pClonk), NO_OWNER);
	var t3 = CreateObject(TRE1, GetX(pClonk),     GetY(pClonk) - 3, NO_OWNER);
	if (!t1 || !t2 || !t3) FatalError("PickerSmoke FAIL step 1: could not spawn trees");

	var pChop = FindObject(OACH);
	items = pChop->~OA_GetItems(pClonk, pClonk);
	if (GetLength(items) != 1)
		FatalError(Format("PickerSmoke FAIL step 1: expected 1 tuple got %d", GetLength(items)));
	var cap = items[0][0];
	/* >=2 candidates -> caption is the "$CtxChop$" stringtbl entry "Chop..." */
	if (!WildcardMatch(cap, "*...*") && !WildcardMatch(cap, "*…*"))
		FatalError(Format("PickerSmoke FAIL step 1: caption '%s' missing ellipsis", cap));
	Log("PickerSmoke step 1 PASS: Chop tuple has ellipsis for >=2 candidates");

	/* Step 2: Edge case #5: candidate disappears mid-menu. */
	var pChop2 = FindObject(OACH);
	var trees  = FindObjects(Find_ID(TRE1));
	if (GetLength(trees) < 2) FatalError("PickerSmoke FAIL step 2: not enough trees");
	var t2b = trees[1];
	RemoveObject(t2b);
	/* OA_Fire on the removed target must return false. Invoked via -> so
	   `this` is the child rule, matching the real picker-row command
	   path (H-1: OA_Fire(pTgt, cmd, pCaller), 3 params). */
	var rc = pChop2->OA_Fire(t2b, "Chop", pClonk);
	if (rc != false)
		FatalError(Format("PickerSmoke FAIL step 2: OA_Fire returned %v for dead target", rc));
	Log("PickerSmoke step 2 PASS: OA_Fire rejects vanished target");

	Log("PickerSmoke PASS");
	GameOver();
	return true;
}
