/*-- Light helpers (faked bioluminescence) --*/

#strict 2

/*
 * Attach a glowing overlay sprite to an object. The overlay uses the
 * Anglerfish-lure pattern: CreateObject(GLOW) + SetAction("Attach", host)
 * + SetCategory(C4D_Object | C4D_Background | C4D_Parallax).
 *
 * to     - host object the glow follows
 * rgb    - glow tint, e.g. RGB(100,200,255)
 * radius - visual radius of the glow sprite (stored on the overlay for the renderer)
 * returns the created overlay object on success, 0 on failure.
 */
global func AttachGlowOverlay(object to, int rgb, int radius)
{
	if (!to) return 0;
	var glow = CreateObject(GLOW, 0, 0, GetOwner(to));
	if (!glow) return 0;
	glow->SetAction("Attach", to);
	glow->SetCategory(C4D_Object | C4D_Background | C4D_Parallax);
	// Store tint + radius on the overlay so a renderer hook can read them
	glow.LocalN("glow_rgb") = rgb;
	glow.LocalN("glow_radius") = radius;
	return glow;
}

/*
 * Sample GetMaterialColor in a coarse polar grid around (ox, oy) and return
 * the [x, y, darkness] triple of the darkest sample. Darkness = 765 - (R+G+B).
 * Used by blind CaveFish to flee toward darkness / away from light.
 */
global func SampleDarkestPixel(int ox, int oy)
{
	var best_x = ox, best_y = oy, best_dark = -1;
	var r, a;
	for (r = 10; r <= 60; r += 10)
	{
		for (a = 0; a < 360; a += 30)
		{
			var sx = ox + Cos(a, r);
			var sy = oy + Sin(a, r);
			var mat = GetMaterial(sx - ox, sy - oy);
			if (mat == -1) continue;
			var col = GetMaterialColor(mat, 0, 0);
			if (col == -1) continue;
			var dark = 765 - (GetRGBValue(col, 1) + GetRGBValue(col, 2) + GetRGBValue(col, 3));
			if (dark > best_dark) { best_dark = dark; best_x = sx; best_y = sy; }
		}
	}
	return [best_x, best_y, best_dark];
}

/*
 * Apply the per-stratum SetGamma ramp. stratum is 0..3.
 * Mirrors the DeepAbyss.c4s FxDepthLightTimer pattern but with four fixed bands.
 */
global func ApplyStratumGamma(int stratum)
{
	if (stratum == 0)
		SetGamma(RGB(0,0,0), RGB(128,132,140), RGB(255,255,255));
	else if (stratum == 1)
		SetGamma(RGB(0,0,0), RGB(80,90,120), RGB(180,200,230));
	else if (stratum == 2)
		SetGamma(RGB(0,0,0), RGB(90,70,110), RGB(200,180,220));
	else if (stratum == 3)
		SetGamma(RGB(0,0,0), RGB(160,110,90), RGB(255,255,255));
}
