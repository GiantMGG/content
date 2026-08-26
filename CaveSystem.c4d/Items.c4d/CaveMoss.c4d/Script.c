/*-- Cave Moss (decor with faked bioluminescent blue glow) --*/

#strict

protected func Initialize()
{
	// Attach a faint blue glow overlay (spec: CaveMoss clings to submerged walls, glows blue)
	AttachGlowOverlay(this(), RGB(80, 140, 220), 30);
}

/* Static decor — no Damage handler, no drops in scope */
