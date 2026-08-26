/*-- Feuer-Topf --*/

#strict

protected func Hit() {
	// Route through STGT if we hit a siege target (x3 vs wooden structures)
	var pT;
	while (pT = FindObject(0, -4, -4, 8, 8, 0, 0, C4D_Structure() | C4D_StaticBack(), NoContainer(), pT))
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
