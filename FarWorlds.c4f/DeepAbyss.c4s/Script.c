#strict

static const DEEP_SUNLIGHT_Y = 400;   // Twilight line (px from surface)
static const DEEP_ABYSS_Y    = 900;   // Abyss line

static iStep;

func Initialize()
{
  // Legacy FxWater gamma wave — preserved from Deep.c4s/Script.c
  AddEffect("Water", 0, 32, 1);
  // Depth-based breath/energy drain
  AddEffect("Pressure", 0, 32, 5);
  // Depth-tinted SetGamma
  AddEffect("DepthLight", 0, 32, 30);
  SetSkyParallax(0, 16, 12);
  // Stamp new content into the three depth bands
  InitializeLandscape();
}

func InitializePlayer(iPlr)
{
  // Find the player's Triton shell base
  var pBase = FindObject(TRTS, GetX(GetCrew(iPlr, 0)), GetY(GetCrew(iPlr, 0)), -1, -1);
  if (!pBase) return();
  // Seed construction kits and links
  var i;
  for (i = 0; i < 5; ++i) pBase->CreateContents(CNKT);
  for (i = 0; i < 3; ++i) pBase->CreateContents(LNKT);
  // Extra crew head straight for the shell
  for (i = 0; i < GetCrewCount(); ++i)
    if (i > 0)
      SetCommand(GetCrew(iPlr, i), "Enter", pBase);
}

/* Procedural trench generator — stamps content into the three depth bands */
func InitializeLandscape()
{
  var w = LandscapeWidth();
  var h = LandscapeHeight();
  var i;

  // Stamp KLPM kelp farms in the Sunlight band (0..400 px)
  for (i = 0; i < 8; i++)
  {
    var x = Random(w - 40) + 20;
    var y = Random(300) + 50;   // somewhere in Sunlight band
    CreateConstruction(KLPM, x, y, -1, 100, 1);
  }

  // Stamp OBUB oxygen vents in the Twilight band (400..900 px)
  for (i = 0; i < 4; i++)
  {
    var x = Random(w - 40) + 20;
    var y = Random(400) + 450;
    CreateConstruction(OBUB, x, y, -1, 100, 1);
  }

  // Stamp AGLF anglerfish at the Twilight/Abyss boundary
  for (i = 0; i < 6; i++)
  {
    var x = Random(w - 60) + 30;
    var y = Random(150) + 800;
    CreateObject(AGLF, x, y, -1);
  }

  // Stamp GSQD giant squid — territorial guardians; spawn near a KLPM farm
  for (i = 0; i < 2; i++)
  {
    var farm = FindObject(KLPM, 0, 0, 0, 0, 0, 0, 0, NoContainer());
    if (farm)
      CreateObject(GSQD, GetX(farm) + RandomX(-100, 100), GetY(farm) + 50, -1);
    else
      CreateObject(GSQD, Random(w), 200, -1);
  }
}

/* Legacy FxWater gamma wave — verbatim from Deep.c4s/Script.c */
global func FxWaterTimer(pObj, iNum, iA)
{
  iA *= 2;
  SetGamma(
    RGB(0,   0,   SinX(iA, 20, 100)),
    RGB(100, 100, CosX(iA, 140, 160)),
    RGB(200, 200, CosX(iA, 220, 255))
  );
}

global func SinX(iA, iMin, iMax) { return(Sin(iA, (iMax - iMin) / 2) + (iMax + iMin) / 2); }
global func CosX(iA, iMin, iMax) { return(Cos(iA, (iMax - iMin) / 2) + (iMax + iMin) / 2); }

/* NEW: depth-based pressure drain — augments the engine's standard breathing model */
global func FxPressureTimer(pObj, iNum, iA)
{
  // Iterate all living non-contained objects; apply depth-based drain
  var o;
  for (o in FindObjects(Find_Category(C4D_Living), Find_NoContainer(), Find_OCF(OCF_Alive)))
  {
    // Skip pressurized vehicles (PDSU) and deep-sea creatures (GSQD, AGLF)
    if (o->~IsPressurized()) continue;
    if (o->~IsDeepSeaCreature()) continue;

    var y = GetY(o);
    if (y > DEEP_ABYSS_Y)
    {
      DoBreath(-4, o);
      DoEnergy(-1, false, C4FxCall_EngAsphyxiation, GetOwner(o), o);
    }
    else if (y > DEEP_SUNLIGHT_Y)
    {
      DoBreath(-2, o);
    }
  }
}

/* NEW: depth-tinted SetGamma — picks the deepest crew member as reference */
global func FxDepthLightTimer(pObj, iNum, iA)
{
  var deepest = 0, dy = 0;
  var c;
  for (c in FindObjects(Find_Category(C4D_Living), Find_OCF(OCF_Alive)))
  {
    if (GetY(c) > dy)
    {
      dy = GetY(c);
      deepest = c;
    }
  }
  if (!deepest) return();

  var y = GetY(deepest);
  var darkness = BoundBy((y - DEEP_SUNLIGHT_Y) * 255 / (DEEP_ABYSS_Y - DEEP_SUNLIGHT_Y), 0, 255);
  SetGamma(
    RGB(0, 0, 0),
    RGB(128 - darkness / 2, 128 - darkness / 2, 128 - darkness / 4),
    RGB(255 - darkness, 255 - darkness, 255 - darkness / 2)
  );
}
