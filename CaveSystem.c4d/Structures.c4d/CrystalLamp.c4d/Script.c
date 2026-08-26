/*-- Crystal Lamp (player-built structure, warm faked glow) --*/

#strict

protected func Initialize()
{
	// Attach a warm white glow overlay (spec: CrystalLamp casts a warm faked glow)
	AttachGlowOverlay(this(), RGB(255, 220, 160), 80);
}

/* Destroyed: drop components for partial refund */
protected func Destruction()
{
	CreateObject(WOOD, 0, 0, NO_OWNER);
	CreateObject(CRYS, 0, 0, NO_OWNER);
}
