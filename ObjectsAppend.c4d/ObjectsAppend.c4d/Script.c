/*-- ObjectsAppend master rule (OAPP) --*/

#strict 3

/* Hardcoded child-rule ID order. Deterministic menu order without depending
   on section link order (spec "Module discovery contract").
   OALN (line-connect) is deferred to v2: the engine has no line-connect
   SetCommand (line connection is driven by the Linekit's Activate via
   CreateLine/ConnectLine, not a clonk command), so the OALine family is a
   v2 placeholder and is not aggregated here. */

/* Singleton guard: if another OAPP already exists, remove ourselves.
   Spec Edge case #7. */
protected func Construction()
{
	var other = FindObject(OAPP, 0, 0, 0, 0, 0, nil, nil, NoContainer(), this());
	if (other) { RemoveObject(); return; }
	return true;
}

/* Engine callback (rule-injection hook at C4ObjectMenu.cpp:772-808).
   Walks the child IDs, calls each child's OA_GetItems, concatenates. */
public func GetContextMenuItems(object pTarget, object pCaller)
{
	if (!pTarget || !pCaller) return [];

	var children = [OAEN, OAGR, OACH, OABD];
	var out = [];
	var i;
	for (i = 0; i < GetLength(children); i++)
	{
		var idChild = children[i];
		var pChild = FindObject(idChild);
		if (!pChild) continue;                          /* cherry-pick: missing child -> skip */
		var items = pChild->~OA_GetItems(pTarget, pCaller);
		if (items)
		{
			var j;
			for (j = 0; j < GetLength(items); j++)
				out[GetLength(out)] = items[j];
		}
	}
	return out;
}

protected func Activate(int iByPlayer)
{
	MessageWindow(GetDesc(), iByPlayer);
	return true;
}
