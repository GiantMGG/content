/*-- Belagerungs-Katapult --*/

#strict

local iPhase, iFiringPlayer;

protected func Initialize() {
	SetAction("Ready");
	SetDir(Random(2));
	iFiringPlayer = NO_OWNER;
}

private func HaltPushers() {
	var pClonk;
	while (pClonk = FindObject(0,0,0,0,0,0,"Push",this(),0,pClonk))
		SetComDir(COMD_Stop(), pClonk);
}

private func FireSound() { Sound("Catapult"); }

private func Charging() {
	if (GetPhase() < iPhase) return 0;
	SetAction("Ready");
	SetPhase(iPhase);
}

public func Fire(bool fAuto) {
	iFiringPlayer = GetController();
	SetAction("Fire");
	SetPhase(7 - iPhase);
	if (!fAuto)
		if (GetPlrViewMode(GetController()) != 2)
			SetActionTargets(0, GetCursor(GetController()));
	return 1;
}

private func Projectile() {
	var pProjectile = Contents();
	var iX = (GetDir() * 2 - 1) * 12;
	var iY = -10;
	var iR = Random(360);
	var iXDir = 8 * (GetDir() * 2 - 1) * iPhase / 6;
	var iYDir = -12 * iPhase / 6;
	var iRDir = 30;
	if (pProjectile) {
		if (GetOCF(pProjectile) & OCF_Alive()) pProjectile->SetKiller(iFiringPlayer);
		pProjectile->SetController(iFiringPlayer);
		Exit(pProjectile, iX, iY, iR, iXDir, iYDir, iRDir);
	}
	iFiringPlayer = NO_OWNER;
	var iDeviation = RandomX(-iPhase * 10 - 20, iPhase * 10 + 20);
	SetXDir(iXDir * 100 + iDeviation, pProjectile, 100);
	SetYDir(iYDir * 100 + iDeviation, pProjectile, 100);
	if (GetActionTarget(1))
		SetPlrView(GetOwner(GetActionTarget(1)), pProjectile);
	// Reload cooldown: 35 frames
	AddEffect("IntReload", this(), 1, 35, this());
}

private func FireAt(int iTX) {
	if (GetAction() ne "Ready" || !GetPhase()) return 0;
	if (iTX > GetX()) SetDir(DIR_Right());
	if (iTX < GetX()) SetDir(DIR_Left());
	iPhase = BoundBy(Sqrt(Abs(GetX() - iTX) / 22), 1, 6);
	SetPhase(iPhase);
	return Fire();
}

/* Laden */
protected func Collection() { Sound("Connect"); }

/* Nur Belagerungsmunition laden */
public func RejectCollect(id idObj, object pObj) {
	if (idObj == SROK || idObj == FPOT || idObj == SBLD || idObj == BOMB) return 0;
	if (idObj == FLNT || idObj == FBMB) return 0;
	return 1;
}

/* Steuerung */
public func ControlCommand(string szCommand, object pTarget, int iX, int iY) {
	if (szCommand eq "MoveTo")
		if (Distance(GetX(), GetY(), iX, iY) > 50) {
			if (!ContentsCount()) { Sound("Click"); return 1; }
			HaltPushers();
			return FireAt(iX, iY);
		}
}

public func ControlThrow() {
	[$TxtFire$|Image=SCAT:1]
	if (GetAction() ne "Ready") return 1;
	if (GetEffect("IntReload", this())) { Sound("Click"); return 1; }
	if (!GetPhase()) return 0;
	if (!ContentsCount()) return 0;
	return Fire();
}

public func ControlConf(int conf) {
	if (AimStdConf(conf)) {
		iPhase = GetPhase();
		Sound("CatapultSet");
	}
}

public func ControlDig(object clonk) {
	[$TxtAim1$|Method=Classic|Image=SCAT:0]
	AimDown(clonk, 8, "ControlConf");
}

public func ControlDown(object clonk) {
	[$TxtAim1$|Method=JumpAndRun|Image=SCAT:0]
}

public func ControlUp(object clonk) {
	[$TxtAim2$|Image=SCAT:2]
	AimUp(clonk, 8, "ControlConf");
}

public func ControlUpdate(object clonk, int comdir) {
	AimUpdate(clonk, comdir, 8, "ControlConf");
}

public func ControlDownSingle() {
	[$TxtResetview$|Method=None]
}

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
