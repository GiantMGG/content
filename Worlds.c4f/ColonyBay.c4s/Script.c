/*-- Colony Bay --*/

#strict 2

// Settlement value threshold for unlocking the lighthouse recipe
static const SETTLEMENT_VALUE_THRESHOLD = 300;

static g_fInitializedPlayers;
static g_iLighthouseUnlocked;

// Simple TutorialMessage wrapper for scenarios without the Tutorial system group.
// The full version is in Tutorial.c4f/System.c4g/Tutorial.c
global func TutorialMessage(string strMessage)
{
    var iPlr = 0;
    if (GetPlayerCount() > 0) iPlr = GetPlayerByIndex(0);
    return CustomMessage(strMessage, 0, iPlr, 0, 0, 0xffffff, DECO, "Portrait:SCLK::0000ff::1", MSG_Bottom, 300);
}

protected func Initialize()
{
    // The GLHT goal object is created by the engine from [Game] Goals=GLHT=1
    // (C4Game::InitGoals runs before this Initialize() callback). Do not
    // create a duplicate here — see review H-1.

    // Place ruined HUT2 on headland (40% completion)
    var iHeadlandX = LandscapeWidth() * 70 / 100;
    var iHeadlandY = LandscapeHeight() * 35 / 100;
    CreateConstruction(HUT2, iHeadlandX, iHeadlandY, -1, 40, 1);

    // Place lighthouse stump on headland (10% completion = foundation stage)
    var iLighthouseX = LandscapeWidth() * 75 / 100;
    var iLighthouseY = LandscapeHeight() * 30 / 100;
    CreateConstruction(LGHT, iLighthouseX, iLighthouseY, -1, 10, 1);

    // Scatter rubble props (WOOD/ROCK) around the ruin
    for (var i = 0; i < 8; i++)
    {
        var iX = iHeadlandX + Random(200) - 100;
        var iY = iHeadlandY + Random(100) - 50;
        CreateObject(WOOD, iX, iY, NO_OWNER);
    }
    for (var i = 0; i < 5; i++)
    {
        var iX = iHeadlandX + Random(200) - 100;
        var iY = iHeadlandY + Random(100) - 50;
        CreateObject(ROCK, iX, iY, NO_OWNER);
    }

    // Coastal fold (cycle 192): dock the western flats so the enrolled
    // tide/fog events have a harbor to play against. Additive dressing
    // only -- the GLHT/LGHT quest is untouched.
    PlaceFlatsDock();

    // Start the wealth-check effect (runs every 30 frames)
    AddEffect("WealthCheck", this, 1, 30, this);

    // Show opening tutorial message
    TutorialMessage("$MsgIntro$");

    return true;
}

// Place a Dock (DKST) construction on the western tidal flats: scan
// down the flats column from mid-height for the first solid ground;
// skip placement entirely if the column never touches solid (no crash,
// the fold stays cosmetic).
func PlaceFlatsDock()
{
    var iFlatsX = LandscapeWidth() * 20 / 100;
    var iDockY = 0;
    var iY;
    for (iY = LandscapeHeight() / 2; iY < LandscapeHeight() - 5; iY++)
    {
        if (GBackSolid(iFlatsX, iY))
        {
            iDockY = iY;
            break;
        }
    }
    if (iDockY > 0)
    {
        CreateConstruction(DKST, iFlatsX, iDockY, -1, 100, 1);
    }
    return true;
}

func InitializePlayer(iPlr)
{
    // Guard against double-init (mirrors Frontier.c4s:17-24)
    if (g_fInitializedPlayers) return;
    g_fInitializedPlayers = 1;

    // Give the first clonk the salvaged ship's stores (WOOD=10, METL=5)
    var pFirstCrew = GetCrew(iPlr, 0);
    if (pFirstCrew)
    {
        for (var j = 0; j < 10; j++)
            pFirstCrew->CreateContents(WOOD);
        for (var j = 0; j < 5; j++)
            pFirstCrew->CreateContents(METL);
    }

    // Plant FLAG at the ruin
    var iRuinX = LandscapeWidth() * 70 / 100;
    var iRuinY = LandscapeHeight() * 35 / 100;
    CreateObject(FLAG, iRuinX, iRuinY, iPlr);

    return true;
}

// Wealth check effect - unlocks lighthouse recipe when settlement value crosses threshold
global func FxWealthCheckTimer(object target, int effect, int timer)
{
    for (var i = 0; i < GetPlayerCount(); i++)
    {
        var iPlr = GetPlayerByIndex(i);
        if (GetPlrValue(iPlr) >= SETTLEMENT_VALUE_THRESHOLD && !g_iLighthouseUnlocked)
        {
            // Grant LGHT knowledge to all players
            for (var j = 0; j < GetPlayerCount(); j++)
            {
                SetPlrKnowledge(GetPlayerByIndex(j), LGHT);
            }
            g_iLighthouseUnlocked = 1;
            TutorialMessage("$MsgLighthouseUnlocked$");
            return 1;
        }
    }
}
