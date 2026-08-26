/*-- Belagerungs-Stein --*/

#strict

protected func Hit() {
	// Route through STGT if we hit a siege target
	var pT;
	while (pT = FindObject(0, -4, -4, 8, 8, 0, 0, C4D_Structure() | C4D_StaticBack(), NoContainer(), pT))
		if (pT->~IsSiegeTarget()) {
			pT->~SiegeDamage(40, GetController(), GetID());
			break;
		}
	// Generic blunt-impact debris
	CastObjects(ROCK, 3, 12);
	Sound("RockHit*");
	RemoveObject();
}
