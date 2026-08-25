/*-- Weizenbuendel --*/

#strict

/* Flag consumed by the Windmill's ProductionAcquireMaterial loop. */
public func IsMillable() { return(1); }

/* Allow stack-splitting menu interaction (mirrors WOOD pattern). */
protected func Hit() { Sound("WoodHit*"); }

/* Feed a tiny amount if eaten raw (last-resort). */
public func Activate(object pClonk)
{
	[$TxtFeed$]
	pClonk->~Feed(5);
	RemoveObject();
	return(1);
}
