/*-- OA_Picker: shared picker include library for ObjectsAppend --*/

#strict

/* ---- Reach defaults (one-line tuning; see spec Edge case #10) ---- */

public func OA_Reach(string cmd)
{
	if (cmd == "Enter") return 40;
	if (cmd == "Grab")  return 20;
	if (cmd == "Chop")  return 40;
	if (cmd == "Build") return 40;
	if (cmd == "Line")  return 40;
	return 30;
}

/* ---- Pagination + batch caps ---- */

public func OA_PageSize() { return 12; }
public func OA_AllCap()   { return 8;  }

/* ---- Picker entrypoint ----
   pCaller     : the acting Clonk (submenu host)
   caption     : menu caption (already localised by caller)
   cmd         : engine command string ("Enter","Grab","Chop","Construct","LineDisconnect")
   candidates  : array of in-reach target objects (already filtered)
   infoFn      : name of a function on pRule that builds a per-candidate info caption
   opts        : proplist { AllowAll = bool }
   pRule       : the calling child rule object (hosts the command dispatchers)
*/

public func OA_OpenPicker(object pCaller, string caption, string cmd,
                          array candidates, string infoFn, proplist opts, object pRule)
{
	if (!pCaller || !candidates) return false;
	var n = GetLength(candidates);
	if (n == 0) return false;

	/* 1-candidate shortcut: fire immediately, no submenu (spec Edge case #2). */
	if (n == 1)
	{
		var tgt = candidates[0];
		if (!tgt || tgt->GetStatus() != C4OS_NORMAL) return false;
		SetCommand(pCaller, cmd, tgt);
		return true;
	}

	/* >=2 candidates: distance-sort and stash state on the rule. */
	var sorted = OA_SortByDistance(candidates, pCaller);
	pRule->LocalN("oa_candidates") = sorted;
	pRule->LocalN("oa_cmd")        = cmd;
	pRule->LocalN("oa_caller")     = pCaller;
	pRule->LocalN("oa_page")       = 0;
	pRule->LocalN("oa_caption")    = caption;
	pRule->LocalN("oa_info_fn")    = infoFn;
	pRule->LocalN("oa_allow_all")  = opts && opts.AllowAll;

	OA_RenderPage(pRule);
	return true;
}

/* Render the current page of the picker submenu on pCaller. */
public func OA_RenderPage(object pRule)
{
	var pCaller   = pRule->LocalN("oa_caller");
	var sorted    = pRule->LocalN("oa_candidates");
	var cmd       = pRule->LocalN("oa_cmd");
	var infoFn    = pRule->LocalN("oa_info_fn");
	var allowAll  = pRule->LocalN("oa_allow_all");
	var page      = pRule->LocalN("oa_page");
	var pagesize  = OA_PageSize();
	var n         = GetLength(sorted);

	var iOldSel = GetMenuSelection(pCaller);
	pCaller->CreateMenu(pRule->GetID(), pCaller, pRule, 0, pRule->LocalN("oa_caption"), 0, 1);

	var start = page * pagesize;
	var end   = Min(start + pagesize, n);
	var i;
	for (i = start; i < end; i++)
	{
		var tgt = sorted[i];
		if (!tgt) continue;
		var szInfo = pRule->Call(infoFn, tgt);
		if (!szInfo) szInfo = tgt->GetName();
		var szCmd = Format("Object(%d)->OA_Fire(Object(%d),\"%s\",Object(%d))",
		                   pRule->ObjectNumber(), tgt->ObjectNumber(), cmd, pCaller->ObjectNumber());
		pCaller->AddMenuItem(tgt->GetName(), szCmd, tgt->GetID(), pCaller, 0, tgt, szInfo);
	}

	/* "More..." / "Previous..." pagination rows. */
	if (end < n)
	{
		var szMoreCmd = Format("Object(%d)->OA_NextPage(Object(%d))",
		                       pRule->ObjectNumber(), pCaller->ObjectNumber());
		pCaller->AddMenuItem("$OAMore$", szMoreCmd, pRule->GetID(), pCaller, 0, 0, "$OAMoreDesc$");
	}
	if (page > 0)
	{
		var szPrevCmd = Format("Object(%d)->OA_PrevPage(Object(%d))",
		                       pRule->ObjectNumber(), pCaller->ObjectNumber());
		pCaller->AddMenuItem("$OAPrevious$", szPrevCmd, pRule->GetID(), pCaller, 0, 0, "$OAPreviousDesc$");
	}

	/* "All" batch entry: only if allowed AND n <= OA_AllCap() (spec Edge case #4). */
	if (allowAll && n <= OA_AllCap())
	{
		var szAllCmd = Format("Object(%d)->OA_FireAll(Object(%d))",
		                      pRule->ObjectNumber(), pCaller->ObjectNumber());
		pCaller->AddMenuItem(Format("$OAAll$", n), szAllCmd, pRule->GetID(), pCaller, 0, 0,
		                     Format("$OAAllDesc$", n));
	}

	/* "Cancel" entry - closes the submenu, no command. */
	var szCancelCmd = Format("Object(%d)->OA_Cancel(Object(%d))",
	                         pRule->ObjectNumber(), pCaller->ObjectNumber());
	pCaller->AddMenuItem("$OACancel$", szCancelCmd, pRule->GetID(), pCaller, 0, 0, "$OACancelDesc$");

	if (iOldSel >= 0) SelectMenuItem(iOldSel, pCaller);
	return true;
}

/* Pagination dispatchers. The injected command is
   `Object(pRuleNr)->OA_NextPage(Object(pCallerNr))`, so `this` is the child
   rule (which inherited these functions via #include OAPK) and the stashed
   `oa_*` locals live on `this`. */
public func OA_NextPage(object pCaller)
{
	this->LocalN("oa_page") = this->LocalN("oa_page") + 1;
	return OA_RenderPage(this);
}

public func OA_PrevPage(object pCaller)
{
	if (this->LocalN("oa_page") > 0)
		this->LocalN("oa_page") = this->LocalN("oa_page") - 1;
	return OA_RenderPage(this);
}

/* Sort candidates by ascending ObjectDistance to pCaller (selection sort,
   O(n^2) but n <= ~50 in practice). */
public func OA_SortByDistance(array candidates, object pCaller)
{
	var a = [];
	var i;
	for (i = 0; i < GetLength(candidates); i++) a[i] = candidates[i];
	var j;
	for (i = 0; i < GetLength(a) - 1; i++)
	{
		var best = i;
		for (j = i + 1; j < GetLength(a); j++)
		{
			if (ObjectDistance(a[j], pCaller) < ObjectDistance(a[best], pCaller)) best = j;
		}
		if (best != i) { var tmp = a[i]; a[i] = a[best]; a[best] = tmp; }
	}
	return a;
}

/* Fire SetCommand on a single target. Re-validates target (spec Edge case #5).
   Invoked as `Object(pRuleNr)->OA_Fire(tgt, cmd, caller)`, so `this` is the
   child rule that inherited this function via #include OAPK. */
public func OA_Fire(object pTgt, string cmd, object pCaller)
{
	if (!pTgt || pTgt->GetStatus() != C4OS_NORMAL)
	{
		if (pCaller) pCaller->CloseMenu();
		return false;
	}
	SetCommand(pCaller, cmd, pTgt);
	if (pCaller) pCaller->CloseMenu();
	return true;
}

/* "All" batch: re-queue SetCommand for every stashed candidate, capped at
   OA_AllCap() (which we already checked before showing the row).
   Invoked as `Object(pRuleNr)->OA_FireAll(caller)`, so `this` is the child
   rule and the stashed `oa_*` locals live on `this`. */
public func OA_FireAll(object pCaller)
{
	var sorted = this->LocalN("oa_candidates");
	var cmd    = this->LocalN("oa_cmd");
	var cap    = OA_AllCap();
	var i, fired = 0;
	for (i = 0; i < GetLength(sorted) && fired < cap; i++)
	{
		var tgt = sorted[i];
		if (!tgt || tgt->GetStatus() != C4OS_NORMAL) continue;
		SetCommand(pCaller, cmd, tgt);
		fired++;
	}
	if (pCaller) pCaller->CloseMenu();
	return true;
}

/* Cancel: close the submenu.
   Invoked as `Object(pRuleNr)->OA_Cancel(caller)`. */
public func OA_Cancel(object pCaller)
{
	if (pCaller) pCaller->CloseMenu();
	return true;
}
