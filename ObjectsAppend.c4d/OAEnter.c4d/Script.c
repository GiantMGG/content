/*-- OAEnter: Enter-able target context item (OAEN) --*/

#strict
#include OAPK

/* Filter: enterable, in reach, no container, same layer, exclude rules/staticback. */
private func OA_Candidates(object pCaller)
{
	return FindObjects(
		pCaller->Find_Distance(OA_Reach("Enter")),
		Find_Exclude(Find_Category(C4D_Rule)),
		Find_Exclude(Find_Category(C4D_StaticBack)),
		Find_NoContainer(),
		Find_Layer(pCaller->GetObjectLayer()),
		Find_OCF(OCF_Container),
		Sort_Distance()
	);
}

/* Per-candidate info caption. */
public func OAEnter_Info(object pTgt)
{
	return Format("%s, %d%% HP", pTgt->GetName(), pTgt->GetEnergy());
}

/* Child-rule contract: returns one context-menu tuple or []. */
public func OA_GetItems(object pTarget, object pCaller)
{
	if (!pTarget || !pCaller) return [];
	var cands = OA_Candidates(pCaller);
	var n = GetLength(cands);
	if (n == 0) return [];

	if (n == 1)
	{
		var tgt = cands[0];
		var szCmd = Format("SetCommand(Object(%d),\"Enter\",Object(%d))",
		                   pCaller->ObjectNumber(), tgt->ObjectNumber());
		return [[tgt->GetName(), szCmd, GetID(), OAEnter_Info(tgt)]];
	}

	/* n >= 2: open the picker submenu. */
	var szCmd = Format("Object(%d)->OA_OpenPickerPicker(Object(%d))",
	                   this->ObjectNumber(), pCaller->ObjectNumber());
	return [["$CtxEnter$", szCmd, GetID(), "$CtxEnterDesc$"]];
}

/* Wrapper called from the injected item: open the picker on pCaller. */
public func OA_OpenPickerPicker(object pCaller)
{
	var cands = OA_Candidates(pCaller);
	var opts = { AllowAll = true };
	return OA_OpenPicker(pCaller, "$CtxEnter$", "Enter", cands, "OAEnter_Info", opts, this);
}

protected func Activate(int iByPlayer)
{
	MessageWindow(GetDesc(), iByPlayer);
	return true;
}
