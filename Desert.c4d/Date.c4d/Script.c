/*-- Date (DATE) -- desert food item. --*/

#strict

public func Activate(object pClonk)
{
	[$TxtEat$]
	return Eat(pClonk);
}

public func Eat(object pByObject)
{
	pByObject->~Feed(40);
	RemoveObject();
	return 1;
}

protected func Hit() { Sound("WoodHit*"); }
