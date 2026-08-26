/*-- CaveExplorer scenario script --*/

#strict

// Strata boundaries (landscape px). Map is 100x75 tiles * zoom = ~95x60 base.
// We use the spec's 0..100 / 100..200 / 200..300 / 300..400 scheme scaled to the
// scenario's actual height. CaveAmbience.c4d owns the canonical CAVE_Stratum*_Top
// constants; the scenario uses CVEX_-prefixed aliases to avoid redefinition.
static const CVEX_Stratum1_Top = 100;
static const CVEX_Stratum2_Top = 200;
static const CVEX_Stratum3_Top = 300;

func Initialize()
{
	// 1. Per-stratum gamma + drip controller
	CreateObject(CVAM, 0, 0, NO_OWNER);

	// 2. HeatAura overlays across stratum 3 (one per ~50px-wide band)
	var x;
	for (x = 0; x < LandscapeWidth(); x += 50)
		CreateObject(HETA, x, CVEX_Stratum3_Top + 50, NO_OWNER);

	// 3. CaveExpedition goal
	CreateObject(CVEX, 0, 0, NO_OWNER);

	// 4. Throttled Obsidian-quench effect (every 60 ticks)
	AddEffect("FxObsidianQuench", 0, 1, 60);

	// 5. Stamp biome decor + creatures + the heart
	SpawnCreatures();
	PlaceDecor();
	PlaceSeleniteHeart();
}

/* Spawn creatures per stratum (spec: BATC in stratum 2 ceiling, CVFS in
   stratum 1 lake, MGCR in stratum 3) */
func SpawnCreatures()
{
	var i, x, y;
	// Bats in stratum 2 ceiling roosts
	for (i = 0; i < 4; ++i)
	{
		x = Random(LandscapeWidth() - 40) + 20;
		y = CVEX_Stratum2_Top + Random(40);
		CreateObject(BATC, x, y, NO_OWNER);
	}
	// CaveFish in stratum 1 lake (underwater)
	for (i = 0; i < 6; ++i)
	{
		x = Random(LandscapeWidth() - 40) + 20;
		y = CVEX_Stratum1_Top + 20 + Random(40);
		CreateObject(CVFS, x, y, NO_OWNER);
	}
	// MagmaCrabs in stratum 3 (on/near DuroLava)
	for (i = 0; i < 3; ++i)
	{
		x = Random(LandscapeWidth() - 40) + 20;
		y = CVEX_Stratum3_Top + 40 + Random(40);
		CreateObject(MGCR, x, y, NO_OWNER);
	}
}

/* Place CrystalCluster + CaveMoss decor at randomized valid (solid-material)
   positions inside each stratum */
func PlaceDecor()
{
	var i, x, y, mat;
	// CrystalCluster in stratum 2
	for (i = 0; i < 8; ++i)
	{
		x = Random(LandscapeWidth() - 40) + 20;
		y = CVEX_Stratum2_Top + Random(80);
		// Only place where the surrounding material is diggable rock
		mat = GetMaterial(x, y);
		if (GetMaterialDensity(mat) > 0)
			CreateObject(CCLT, x, y, NO_OWNER);
	}
	// CaveMoss in stratum 1 (submerged walls)
	for (i = 0; i < 8; ++i)
	{
		x = Random(LandscapeWidth() - 40) + 20;
		y = CVEX_Stratum1_Top + Random(80);
		mat = GetMaterial(x, y);
		if (GetMaterialDensity(mat) > 0)
			CreateObject(CMOS, x, y, NO_OWNER);
	}
}

/* Place the Selenite Heart in the deepest lava pocket (stratum 3, bottom) */
func PlaceSeleniteHeart()
{
	var x = LandscapeWidth() / 2;
	var y = CVEX_Stratum3_Top + 80;
	CreateObject(SLNH, x, y, NO_OWNER);
}

/* Late-joiners: place new crew at the surface starting cave with starter
   supplies (Low.c4s JoinPlayer pattern). Late joiners beyond frame 370 are
   eliminated. */
func InitializePlayer(int iPlr)
{
	if (FrameCounter() > 370)
	{
		EliminatePlayer(iPlr);
		return;
	}
	var crew = GetCrew(iPlr, 0);
	if (!crew) return;
	// Seed starter supplies into the homebase (HUT3) near the crew
	var base = FindObject(HUT3, GetX(crew) - 200, GetY(crew) - 100, 400, 200);
	if (base)
	{
		base->CreateContents(FLNT, 5);
		base->CreateContents(BRED, 3);
		base->CreateContents(CNKT, 2);
	}
}

/* Throttled Obsidian-quench effect (spec edge case "Obsidian quenching").
   Every 60 ticks, scan a sparse grid of pixels for DuroLava directly adjacent
   (4-neighbourhood) to Water; for each match, InsertMaterial(Obsidian) and
   consume one Water pixel. Cap at 200 quenched pixels per scan. */
global func FxObsidianQuenchTimer(object pObj, int iNum, int iA)
{
	var mat_lava  = Material("DuroLava");
	var mat_water = Material("Water");
	var mat_obs   = Material("Obsidian");
	if (mat_lava == -1 || mat_water == -1 || mat_obs == -1) return 1;

	var quenched = 0;
	var x, y, here;
	for (y = 0; y < LandscapeHeight() && quenched < 200; y += 8)
	{
		for (x = 0; x < LandscapeWidth() && quenched < 200; x += 8)
		{
			here = GetMaterial(x, y);
			if (here != mat_lava) continue;
			// Check 4-neighbourhood for Water
			if (GetMaterial(x - 1, y) == mat_water ||
			    GetMaterial(x + 1, y) == mat_water ||
			    GetMaterial(x, y - 1) == mat_water ||
			    GetMaterial(x, y + 1) == mat_water)
			{
				InsertMaterial(mat_obs, x, y);
				quenched++;
			}
		}
	}
	return 1;
}
