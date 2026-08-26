/*-- Bombe --*/

#strict

public func Initialize() {
	// Timed fuse: the FxIntFuseTimer fires once after 70 frames and detonates.
	AddEffect("IntFuse", this(), 1, 70, this());
	return 1;
}

protected func Hit() {
	// Bombs are timed-fuse; impact just plays a sound. The fuse effect
	// triggers Explode(30) at expiry.
	Sound("RockHit*");
	return 1;
}

public func FxIntFuseTimer(object pTarget, int iEffectNumber, int iEffectTime) {
	// Detonate at fuse expiry (70 frames after creation).
	Explode(30);
	return -1; // remove the effect
}
