/*-- Escape Gate (EGAT) — exit gate; SWCH Access() contract. --*/
/* Lock()/Unlock()/IsLocked() state; Access() drops the        */
/* SolidMask and flips to the open action. Rooms sense the     */
/* solved predicate and call Unlock()+Access().                */

#strict 2

local locked;

public func Lock()     { locked = true;  return true; }
public func Unlock()   { locked = false; return true; }
public func IsLocked() { return locked; }

public func Access()
{
	if (IsLocked())
	{
		Sound("CommandFailure1");
		return false;
	}
	SetAction("GateOpen");
	SetSolidMask(0, 0, 0, 0, 0, 0);
	Sound("Arrow");
	return true;
}

protected func Construction()
{
	SetAction("Gate");
	return true;
}
