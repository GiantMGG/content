/*-- Rammbock --*/

#strict

local iLastSwing;

protected func Initialize() {
	SetAction("Idle");
	SetDir(Random(2));
	iLastSwing = 0;
}

/* Momentum-damage: every frame the ram head overlaps a siege target,
   deal Mass*|XDir|/1000 damage (clamped [0,60]). A stationary ram does 0. */
private func RamContact(int iDirSign) {
	// No movement -> no damage
	if (GetXDir(this(), 100) == 0) return;
	// Ram head is the leading edge: scan a small box ahead
	var iHeadX = 18 * iDirSign;
	var pTarget;
	while (pTarget = FindObject(0, iHeadX - 2, -8, 4, 16, OCF_Fullcon(), 0, C4D_Structure() | C4D_StaticBack(), NoContainer(), pTarget))
		if (pTarget->~IsSiegeTarget()) {
			var iMomentum = GetMass() * Abs(GetXDir(this(), 100)) / 1000;
			iMomentum = BoundBy(iMomentum, 0, 60);
			if (iMomentum <= 0) return;
			// Rate-limit the swing sound/debris to every 6 frames
			if (FrameCounter() - iLastSwing >= 6) {
				Sound("RamHit");
				CastObjects(ROCK, 2, 10, iHeadX, 0);
				iLastSwing = FrameCounter();
			}
			pTarget->~SiegeDamage(iMomentum, GetController(), GetID());
			return;
		}
}

/* Contact callbacks fire on left/right contact. */
public func ContactLeft() { RamContact(-1); return 0; }
public func ContactRight() { RamContact(+1); return 0; }

/* Pferde-Anspannung (template: Mortar.c4d) */
public func IsHorseWagon() { return 1; }

private func FindPullingHorse(pHorse) {
	return FindObject2(Find_ActionTarget(this()), Find_Or(Find_Action("Pull"), Find_Action("Pull2"), Find_Action("Pull3"), Find_Action("Turn")));
}

public func Connect(pHorse) {
	CreateObject(CHBM, 0, 0, GetOwner())->CHBM::Connect(pHorse, this());
	Sound("Connect");
	return 1;
}

public func Disconnect(pFromHorse) {
	var pBeam = FindObject(CHBM, 0, 0, 0, 0, 0, "Connect", pFromHorse);
	if (pBeam) RemoveObject(pBeam);
	Sound("Connect");
	return 1;
}

public func PullCheck() {
	if (FindPullingHorse()) return;
	if (GetXDir() != 0)
		if (FindObject(0, 0, 0, 0, 0, OCF_CrewMember(), "Push", this(), NoContainer()))
			SetXDir(BoundBy(GetXDir(), -2, 2));
}

/* Forschung */
public func GetResearchBase() { return SIEE; }
