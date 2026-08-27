/*-- OAChop: choppable target context item (OACH) --*/

#strict
#include OAPK

private func OA_Candidates(object pCaller)
{
	return FindObjects(
		pCaller->Find_Distance(OA_Reach("Chop")),
		Find_Exclude(Find_Category(C4D_Rule)),
		Find_Exclude(Find_Category(C4D_StaticBack)),
		Find_NoContainer(),
		Find_Layer(pCaller->GetObjectLayer()),
		Find_OCF(OCF_Chop),
		Sort_Distance()
	);
}

public func OAChop_Info(object pTgt)
{
	/* Tree type + girth (spec "Picker UX"). */
	var szType = pTgt->GetName();
	var iGirth = pTgt->GetCon();
	return Format("%s, %d%%", szType, iGirth);
}

public func OA_GetItems(object pTarget, object pCaller)
{
	if (!pTarget || !pCaller) return [];
	var cands = OA_Candidates(pCaller);
	var n = GetLength(cands);
	if (n == 0) return [];

	if (n == 1)
	{
		var tgt = cands[0];
		var szCmd = Format("SetCommand(Object(%d),\"Chop\",Object(%d))",
		                   pCaller->ObjectNumber(), tgt->ObjectNumber());
		return [[tgt->GetName(), szCmd, GetID(), OAChop_Info(tgt)]];
	}

	var szCmd = Format("Object(%d)->OA_OpenPickerPicker(Object(%d))",
	                   this->ObjectNumber(), pCaller->ObjectNumber());
	return [["$CtxChop$", szCmd, GetID(), "$CtxChopDesc$"]];
}

public func OA_OpenPickerPicker(object pCaller)
{
	var cands = OA_Candidates(pCaller);
	var opts = { AllowAll = true };
	return OA_OpenPicker(pCaller, "$CtxChop$", "Chop", cands, "OAChop_Info", opts, this);
}

protected func Activate(int iByPlayer)
{
	MessageWindow(GetDesc(), iByPlayer);
	return true;
}
