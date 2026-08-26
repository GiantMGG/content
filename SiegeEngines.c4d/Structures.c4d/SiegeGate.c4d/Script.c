/*-- Belagerungs-Tor --*/

#strict

#include STGT

protected func Initialize() {
	SetAction("Idle");
	// Gate is a wooden structure: vulnerable to fire (x3 from FPOT)
	return 1;
}

/* STGT overrides */
public func MaxSiegeHP() { return 120; }
public func IsWoodenStructure() { return true; }

/* Open the gate on ControlLeft/ControlRight when a clonk pushes it. */
protected func ControlLeft(object pClonk) {
	[$TxtOpen$|Image=SGAT:0]
	if (GetAction() eq "Open") return 1;
	SetAction("Open");
	Sound("Gate");
	return 1;
}

protected func ControlRight(object pClonk) {
	[$TxtOpen$|Image=SGAT:0]
	return ControlLeft(pClonk);
}

/* Forschung */
public func GetResearchBase() { return SIEE; }
