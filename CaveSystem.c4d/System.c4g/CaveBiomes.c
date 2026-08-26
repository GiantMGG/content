/*-- Composable biome overlay prefabs for CaveSystem --*/

#strict 2

/*
 * Stamp a band of the landscape with biome materials. Modders call these from
 * a scenario Initialize() to compose a multi-strata cave. Each function scans
 * a sparse grid (every 4 px) across the band and InsertMaterial's the biome
 * material where the existing material is diggable earth/rock (i.e. not open
 * Tunnel/Air and not already the target material). Cap per call: 5000 px.
 *
 * Arguments:
 *   x0, y0   - top-left of the band (landscape px)
 *   wdt, hgt - band dimensions
 */

global func Biome_TopCave(int x0, int y0, int wdt, int hgt)
{
	StampBand(x0, y0, wdt, hgt, "Earth", "Quartz", 12);
}

global func Biome_Lake(int x0, int y0, int wdt, int hgt)
{
	StampBand(x0, y0, wdt, hgt, "Water", "Amethyst", 10);
}

global func Biome_CrystalCavern(int x0, int y0, int wdt, int hgt)
{
	StampBand(x0, y0, wdt, hgt, "Rock", "Amethyst", 8);
}

global func Biome_LavaCavern(int x0, int y0, int wdt, int hgt)
{
	StampBand(x0, y0, wdt, hgt, "DuroLava", "Selenite", 6);
}

/*
 * Internal: scan a sparse grid in the band and InsertMaterial target_mat at
 * eligible pixels (currently Earth/Rock/Sand and not already target_mat),
 * plus seed_mat at a 1-in-seedEvery chance for crystal seams.
 */
private func StampBand(int x0, int y0, int wdt, int hgt, string target_mat, string seed_mat, int seedEvery)
{
	var mat_target = Material(target_mat);
	var mat_seed   = Material(seed_mat);
	if (mat_target == -1 || mat_seed == -1) return;
	var stamped = 0;
	var x, y;
	for (y = y0; y < y0 + hgt; y += 4)
	{
		for (x = x0; x < x0 + wdt; x += 4)
		{
			if (stamped >= 5000) return;
			var here = GetMaterial(x, y);
			if (here == -1) continue;
			// Only stamp into solid earth/rock-class materials (Density > 0)
			if (GetMaterialDensity(here) <= 0) continue;
			if (here == mat_target) continue;
			InsertMaterial(mat_target, x, y);
			stamped++;
			if (!Random(seedEvery))
				InsertMaterial(mat_seed, x, y);
		}
	}
}
