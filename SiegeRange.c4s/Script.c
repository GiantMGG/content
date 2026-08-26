/*-- Belagerungs-Schießstand --*/

#strict

protected func Initialize() {
	// Spawn siege engines for the attacker
	CreateObject(SCAT, 100, 100, NO_OWNER);
	CreateObject(BRAM, 140, 100, NO_OWNER);
	CreateObject(TRBT, 180, 100, NO_OWNER);
	// Spawn a horse for towing
	CreateObject(HORS, 80, 100, NO_OWNER);
	// Ammunition piles
	for (var i = 0; i < 5; ++i) CreateObject(SROK, 110 + i * 4, 90, NO_OWNER);
	for (var i = 0; i < 5; ++i) CreateObject(FPOT, 130 + i * 4, 90, NO_OWNER);
	for (var i = 0; i < 3; ++i) CreateObject(SBLD, 150 + i * 4, 90, NO_OWNER);
	// Note: BOMBs have a timed fuse starting on creation (70 frames).
	// Don't pre-spawn them — craft on demand instead.
	// A row of destructible siege gates as targets
	for (var i = 0; i < 3; ++i) CreateObject(SGAT, 400 + i * 50, 100, NO_OWNER);
	return 1;
}

protected func InitializePlayer(int iPlr) {
	// Give the attacker a clonk and basic knowledge
	var pCrew = GetCrew(iPlr, 0);
	if (pCrew) SetPosition(60, 90, pCrew);
	return 1;
}
