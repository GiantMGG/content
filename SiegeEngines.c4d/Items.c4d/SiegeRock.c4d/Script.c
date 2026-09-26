/*-- Belagerungs-Stein --*/

#strict

protected func Hit() {
	// Route through STGT if we hit a siege target
	var pT;
	// Category filter via FindObjects (plain FindObject has no category
	// parameter; passing the mask there trips "param 8: expected object").
	for (pT in FindObjects(Find_InRect(-4, -4, 8, 8), Find_Category(C4D_Structure() | C4D_StaticBack()), Find_NoContainer()))
		if (pT->~IsSiegeTarget()) {
			pT->~SiegeDamage(40, GetController(), GetID());
			break;
		}
	// Generic blunt-impact debris
	CastObjects(ROCK, 3, 12);
	Sound("RockHit*");
	RemoveObject();
}
