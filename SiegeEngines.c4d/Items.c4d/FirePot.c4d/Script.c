/*-- Feuer-Topf --*/

#strict

protected func Hit() {
	// Route through STGT if we hit a siege target (x3 vs wooden structures)
	var pT;
	// Category filter via FindObjects (plain FindObject has no category
	// parameter; passing the mask there trips "param 8: expected object").
	for (pT in FindObjects(Find_InRect(-4, -4, 8, 8), Find_Category(C4D_Structure() | C4D_StaticBack()), Find_NoContainer()))
		if (pT->~IsSiegeTarget()) {
			pT->~SiegeDamage(20, GetController(), GetID());
			break;
		}
	// Verbatim from FireBomb.c4d/Script.c:14-22
	CastObjects(DFLM, 20, 20);
	Sound("Inflame");
	Sound("Fuse");
	RemoveObject();
	return 1;
}
