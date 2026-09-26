/*-- Belagerungs-Felsbrocken --*/

#strict

protected func Hit() {
	// Route through STGT if we hit a siege target
	// SBLD ignores 50% of MaxSiegeHP (handled in STGT::SiegeDamage)
	var pT;
	// Category filter via FindObjects (plain FindObject has no category
	// parameter; passing the mask there trips "param 8: expected object").
	for (pT in FindObjects(Find_InRect(-6, -6, 12, 12), Find_Category(C4D_Structure() | C4D_StaticBack()), Find_NoContainer()))
		if (pT->~IsSiegeTarget()) {
			pT->~SiegeDamage(120, GetController(), GetID());
			break;
		}
	// Heavy debris
	CastObjects(ROCK, 8, 25);
	Sound("RockHit*");
	RemoveObject();
}
