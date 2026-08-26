/*-- Crystal Cluster (decor with faked bioluminescent glow) --*/

#strict

protected func Initialize()
{
	// Attach a soft amethyst glow overlay (spec: CrystalCluster glows)
	AttachGlowOverlay(this(), RGB(180, 80, 200), 40);
}

/* Mineable: drops an Amethyst chunk when destroyed */
protected func Damage(int iChange, int iPlr)
{
	if (GetDamage() > 30)
	{
		var drop = CreateObject(AMTH, 0, 0, NO_OWNER);
		if (drop) drop->SetOwner(NO_OWNER);
		RemoveObject();
	}
}
