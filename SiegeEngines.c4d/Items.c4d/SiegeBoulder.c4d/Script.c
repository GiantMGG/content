/*-- Belagerungs-Felsbrocken --*/

#strict

protected func Hit() {
	// Route through STGT if we hit a siege target
	// SBLD ignores 50% of MaxSiegeHP (handled in STGT::SiegeDamage)
	var pT;
	while (pT = FindObject(0, -6, -6, 12, 12, 0, 0, C4D_Structure() | C4D_StaticBack(), NoContainer(), pT))
		if (pT->~IsSiegeTarget()) {
			pT->~SiegeDamage(120, GetController(), GetID());
			break;
		}
	// Heavy debris
	CastObjects(ROCK, 8, 25);
	Sound("RockHit*");
	RemoveObject();
}
