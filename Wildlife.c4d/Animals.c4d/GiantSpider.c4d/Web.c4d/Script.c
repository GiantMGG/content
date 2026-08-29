/*-- Web -- C4D_StaticBack; grants Webbed slow via WebTick timer. --*/

#strict 3

#include WLFA

protected func Initialize() { SetAction("Idle"); return true; }

// TimerCall (every 5 ticks): scan for Clonks/prey in the web's area and
// apply the Webbed slow. The web itself (this) is the AddEffect host so
// FxWebbedStart/Stop (defined in WLFA, which this web #includes) resolve.
// Spec edge case: placing a web on top of a Clonk does NOT instantly web
// them -- they must move into the web's AtRect (the timer catches this).
public func WebTick()
{
	var contactor;
	while (contactor = FindObject2(Find_OCF(OCF_CrewMember), Find_NoContainer(),
	                               Find_AtRect(-10, -10, 20, 20), Find_Exclude(contactor)))
	{
		WLFA_ApplyWebbed(contactor, this);
	}
	return true;
}

// Ally counterplay: attacking the web destroys it.
protected func Damage(int iChange, int iPlr)
{
	RemoveObject();
	return true;
}
