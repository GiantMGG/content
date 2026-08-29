/*-- Lobster (LBST) -- bottom-walking crustacean. --*/

#strict
#include ANIM

local Bait;   // bait object currently being followed

public func IsPossessible() { return 1; }

/* Lobsters willingly enter lobster traps (mirrors Fish allowing IsFishTower).
   Without this, the inherited ANIM RejectEntrance rejects collection whenever
   the COAN "collectible animals" rule is absent -- which is the case in the
   headless smoke scenario. */
public func RejectEntrance(object pContainer)
{
	if (pContainer->~IsLobsterTrap()) return 0;
	return _inherited(pContainer, ...);
}

protected func Initialize()
{
	if (GetAction() S= "Idle") return this->~Birth();
}

/* TimerCall -- KI-Steuerung (mirrors Fish.Activity). */
private func Activity()
{
	if (GetEffect("PossessionSpell", this())) return();

	if (GetAction() eq "Walk") WalkDir();

	// Swimming towards a bait?
	if (Bait)
	{
		if (GetAction(Bait) ne "Bait") Bait = 0;
		else return 1;
	}

	// Forage for a bait signal when fully grown.
	if (GetCon() == 100 && !Random(5))
	{
		var pBait = FindObject(Find_Distance(250), Find_Action("Bait"));
		if (pBait && Random(100) < pBait->~IsBait())
		{
			SetCommand(this(), "Follow", pBait);
			Bait = pBait;
		}
	}

	// Reproduction.
	if (!Random(ReproductionRate())) this->~Reproduction();
	return 1;
}

private func WalkDir()
{
	SetComDir(COMD_Left());
	if (Random(2)) SetComDir(COMD_Right());
	return 1;
}

/* Reproduction overrides (cooperate with REPR like Wolf). */
private func ReproductionAreaSize() { return 600; }
private func ReproductionRate()     { return 6000; }
private func MaxAnimalCount()       { return 4; }

/* State transitions. */
public func Cook()      { ChangeDef(CLBS); SetAlive(0); return 1; }
protected func Death()  { ChangeDef(DLBS); SetAction("Dead"); return 1; }

/* Contact turns (mirror Fish). */
protected func ContactLeft()  { if (GetEffect("PossessionSpell", this())) return(); return TurnRight(); }
protected func ContactRight() { if (GetEffect("PossessionSpell", this())) return(); return TurnLeft(); }

private func TurnRight()
{
	if (Stuck() || (GetAction() ne "Walk" && GetAction() ne "Swim")) return 0;
	if (GetXDir() < 0) SetXDir(0);
	SetDir(DIR_Right());
	SetComDir(COMD_Right());
	return 1;
}

private func TurnLeft()
{
	if (Stuck() || (GetAction() ne "Walk" && GetAction() ne "Swim")) return 0;
	if (GetXDir() > 0) SetXDir(0);
	SetDir(DIR_Left());
	SetComDir(COMD_Left());
	return 1;
}
