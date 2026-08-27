/*-- OALine: line-connect target context item (OALN) -- v2 placeholder.
   The engine has no line-connect SetCommand (line connection is driven by
   the Linekit's Activate via CreateLine/ConnectLine, not a clonk command),
   so this family is not aggregated by the OAPP master rule in v1. A v2
   implementation would mirror the Linekit's ConnectLine/CreateLine flow
   via a custom dispatcher instead of SetCommand. --*/

#strict
#include OAPK

private func OA_Candidates(object pCaller)
{
	return FindObjects(
		pCaller->Find_Distance(OA_Reach("Line")),
		Find_Exclude(Find_Category(C4D_Rule)),
		Find_Exclude(Find_Category(C4D_StaticBack)),
		Find_NoContainer(),
		Find_Layer(pCaller->GetObjectLayer()),
		Find_OCF(OCF_LineConstruct),
		Sort_Distance()
	);
}

public func OALine_Info(object pTgt)
{
	/* Line type (spec "Picker UX"). */
	return Format("%s, $OALineConnects$", pTgt->GetName());
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
		var szCmd = Format("SetCommand(Object(%d),\"LineConnect\",Object(%d))",
		                   pCaller->ObjectNumber(), tgt->ObjectNumber());
		return [[tgt->GetName(), szCmd, GetID(), OALine_Info(tgt)]];
	}

	var szCmd = Format("Object(%d)->OA_OpenPickerPicker(Object(%d))",
	                   this->ObjectNumber(), pCaller->ObjectNumber());
	return [["$CtxLine$", szCmd, GetID(), "$CtxLineDesc$"]];
}

public func OA_OpenPickerPicker(object pCaller)
{
	var cands = OA_Candidates(pCaller);
	var opts = { AllowAll = false };  /* Lines are mutually exclusive; no batch */
	return OA_OpenPicker(pCaller, "$CtxLine$", "LineConnect", cands, "OALine_Info", opts, this);
}

protected func Activate(int iByPlayer)
{
	MessageWindow(GetDesc(), iByPlayer);
	return true;
}
