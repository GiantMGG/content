/*-- CookedLobster (CLBS) -- cooked lobster food. --*/

#strict

public func Activate(object pByObject)
{
	[$CtrlEatDesc$]
	return Eat(pByObject);
}

public func Eat(object pByObject)
{
	pByObject->~Feed(60);
	RemoveObject();
	return 1;
}
