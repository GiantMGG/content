/*-- Muehlenfluegel --*/

#strict

protected func Completion()
{
	SetR(Random(360));
	return(1);
}

/* Turn action PhaseCall — keep rotation speed proportional to wind. */
public func Wind2Turn()
{
	SetRDir(GetWind() / 4);
	return(1);
}
