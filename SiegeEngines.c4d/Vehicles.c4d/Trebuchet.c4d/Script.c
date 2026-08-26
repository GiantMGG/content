/*-- Trebuchet --*/

#strict

local pCrosshair;
local iReleaseAngle;   // 30..80 degrees
local iFiringPlayer;
local bLaunched;

protected func Initialize() {
	SetAction("Ready");
	SetDir(Random(2));
	SetPhase(0);
	iReleaseAngle = 45;
	iFiringPlayer = NO_OWNER;
	UpdateCrosshair();
}

/* Aiming: ControlUp/ControlDown span the release angle across 30..80. */
public func ControlUp(object pClonk) {
	[$TxtAimup$|Image=TRBT:2]
	iReleaseAngle = BoundBy(iReleaseAngle + 5, 30, 80);
	UpdateCrosshair();
	Sound("CatapultSet");
	return 1;
}

public func ControlDown(object pClonk) {
	[$TxtAimdown$|Image=TRBT:0]
	iReleaseAngle = BoundBy(iReleaseAngle - 5, 30, 80);
	UpdateCrosshair();
	Sound("CatapultSet");
	return 1;
}

public func ControlThrow(object pClonk) {
	[$TxtFire$|Image=TRBT:1]
	// Reload gate
	if (GetEffect("IntReload", this())) { Sound("Click"); return 1; }
	// Need a loaded projectile
	if (!ContentsCount()) { Sound("Click"); return 1; }
	if (GetAction() != "Ready") return 0;
	iFiringPlayer = GetController();
	bLaunched = false;
	SetAction("Swing");
	return 1;
}

/* Per-phase callback during Swing: at the release-point phase, launch. */
public func SwingPhase() {
	if (GetPhase() < 5) return;
	if (bLaunched) return;
	bLaunched = true;
	Launch();
}

private func Launch() {
	var pProj = Contents();
	if (!pProj) { Sound("Click"); return; }
	var iDir = GetDir() * 2 - 1;
	var iAngle = iReleaseAngle;
	var iPower = 160; // fixed launch power
	var iX = Sin(iAngle, 16) * iDir + 3 * (GetDir() * 2 - 1) + 2 * GetDir();
	var iY = -Cos(iAngle, 16) + 2;
	var iXDir = Sin(iAngle, iPower) * iDir;
	var iYDir = -Cos(iAngle, iPower);
	SetOwner(GetController(), pProj);
	pProj->SetController(iFiringPlayer);
	Exit(pProj, iX, iY, Random(360));
	SetXDir(iXDir, pProj, 10);
	SetYDir(iYDir, pProj, 10);
	// Smoke puff at the sling
	for (var i = 0; i < 4; ++i)
		Smoke(iX + RandomX(-5, 5), iY + RandomX(-5, 5), RandomX(5, 12));
	Sound("Catapult");
	// Camera follow
	if (GetPlrViewMode(GetController()) != 2)
		SetPlrView(GetController(), pProj);
	iFiringPlayer = NO_OWNER;
	// Slow reload: 150 frames
	AddEffect("IntReload", this(), 1, 150, this());
}

/* Only the four siege ammunition IDs fit the sling. All four have
   Mass <= 30 (SROK=15, FPOT=10, SBLD=30, BOMB=12), so no explicit
   Mass check is needed. */
public func RejectCollect(id idObj, object pObj) {
	if (idObj == SROK || idObj == FPOT || idObj == SBLD || idObj == BOMB) return 0;
	return 1;
}

protected func Collection() { Sound("Connect"); }

/* Crosshair sub-object (template: Cannon.c4d Script.c:89-103) */
public func UpdateCrosshair(int iDir) {
	if (Contained() || GetAction() != "Ready") {
		if (pCrosshair) RemoveObject(pCrosshair);
		return 0;
	}
	if (!pCrosshair) {
		pCrosshair = CreateObject(GC4V, 0, 0, GetOwner());
		ObjectSetAction(pCrosshair, "Crosshair", this());
	}
	if (!iDir) iDir = GetDir() * 2 - 1;
	// Crosshair sits ahead along the release angle
	var iAngle = iReleaseAngle;
	SetVertexXY(0, (-5 - Sin(iAngle, 40)) * iDir, 3 + Cos(iAngle, 40), pCrosshair);
}

protected func Departure(object pContainer) { UpdateCrosshair(); return 1; }
protected func Entrance(object pContainer) { UpdateCrosshair(); return 1; }
protected func Destruction() { if (pCrosshair) RemoveObject(pCrosshair); }

/* Forschung */
public func GetResearchBase() { return SIEE; }
