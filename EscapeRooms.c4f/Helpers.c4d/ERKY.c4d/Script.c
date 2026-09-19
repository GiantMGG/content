/*-- Escape Key (ERKY) — collectible; Activate unlocks the     */
/* bound EGAT gate (SevenKeys KeyBase shape).                  */

#strict 2

local gate;

public func Set(object g) { gate = g; }

public func Activate(object clonk)
{
	if (gate)
	{
		gate->Unlock();
		Sound("Keypad");
		return RemoveObject();
	}
	Sound("CommandFailure1");
	return false;
}
