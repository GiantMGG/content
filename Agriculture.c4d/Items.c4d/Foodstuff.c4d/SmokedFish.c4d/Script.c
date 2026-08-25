/*-- Raeucherfisch --*/

#strict

public func Activate(object pClonk)
{
	[$CtrlEatDesc$]
	return(Eat(pClonk));
}

public func Eat(object pByObject)
{
	pByObject->~Feed(120);
	RemoveObject();
	return(1);
}

protected func Hit() { Sound("WoodHit*"); }
