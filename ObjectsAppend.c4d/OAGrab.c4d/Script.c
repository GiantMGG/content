/*-- OAGrab: grabbable target context item (OAGR) --*/

#strict
#include OAPK

private func OA_Candidates(object pCaller)
{
	return FindObjects(
		Find_Distance(OA_Reach("Grab")),
		Find_Exclude(Find_Category(C4D_Rule)),
		Find_Exclude(Find_Category(C4D_StaticBack)),
		Find_NoContainer(),
		Find_Layer(pCaller->GetObjectLayer()),
		Find_Func("IsGrabable"),
		Sort_Distance()
	);
}

public func OAGrab_Info(object pTgt)
{
	return Format("%s, %d%% HP", pTgt->GetName(), pTgt->GetEnergy());
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
		var szCmd = Format("SetCommand(Object(%d),\"Grab\",Object(%d))",
		                   pCaller->ObjectNumber(), tgt->ObjectNumber());
		return [[tgt->GetName(), szCmd, GetID(), OAGrab_Info(tgt)]];
	}

	var szCmd = Format("Object(%d)->OA_OpenPickerPicker(Object(%d))",
	                   this->ObjectNumber(), pCaller->ObjectNumber());
	return [["$CtxGrab$", szCmd, GetID(), "$CtxGrabDesc$"]];
}

public func OA_OpenPickerPicker(object pCaller)
{
	var cands = OA_Candidates(pCaller);
	var opts = { AllowAll = true };
	return OA_OpenPicker(pCaller, "$CtxGrab$", "Grab", cands, "OAGrab_Info", opts, this);
}

protected func Activate(int iByPlayer)
{
	MessageWindow(GetDesc(), iByPlayer);
	return true;
}
