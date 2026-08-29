/*-- Pearl (APRL) -- harvestable pearl emitted by the Oyster. --*/

#strict

public func Activate(object pByObject)
{
	[$CtrlEatDesc$]
	return Eat(pByObject);
}

public func Eat(object pByObject)
{
	pByObject->~Feed(10);
	RemoveObject();
	return 1;
}
