/*-- OABuild: construction-site context item (OABD) --*/

#strict
#include OAPK

private func OA_Candidates(object pCaller)
{
	return FindObjects(
		pCaller->Find_Distance(OA_Reach("Build")),
		Find_Exclude(Find_Category(C4D_Rule)),
		Find_Exclude(Find_Category(C4D_StaticBack)),
		Find_NoContainer(),
		Find_Layer(pCaller->GetObjectLayer()),
		Find_OCF(OCF_Construct),
		Sort_Distance()
	);
}

public func OABuild_Info(object pTgt)
{
	/* Completion % (spec "Picker UX"). */
	return Format("%s, %d%%", pTgt->GetName(), pTgt->GetCon());
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
		var szCmd = Format("SetCommand(Object(%d),\"Construct\",Object(%d))",
		                   pCaller->ObjectNumber(), tgt->ObjectNumber());
		return [[tgt->GetName(), szCmd, GetID(), OABuild_Info(tgt)]];
	}

	var szCmd = Format("Object(%d)->OA_OpenPickerPicker(Object(%d))",
	                   this->ObjectNumber(), pCaller->ObjectNumber());
	return [["$CtxBuild$", szCmd, GetID(), "$CtxBuildDesc$"]];
}

public func OA_OpenPickerPicker(object pCaller)
{
	var cands = OA_Candidates(pCaller);
	var opts = { AllowAll = true };
	return OA_OpenPicker(pCaller, "$CtxBuild$", "Construct", cands, "OABuild_Info", opts, this);
}

protected func Activate(int iByPlayer)
{
	MessageWindow(GetDesc(), iByPlayer);
	return true;
}
