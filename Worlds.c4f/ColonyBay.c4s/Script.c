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
    // Create the goal object at center of map
    CreateObject(GLHT, LandscapeWidth() / 2, LandscapeHeight() / 2, NO_OWNER);

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

    // Start the wealth-check effect (runs every 30 frames)
    AddEffect("WealthCheck", this, 1, 30, this);

    // Show opening tutorial message
    TutorialMessage("$MsgIntro$");

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
func FxWealthCheckTimer(object target, int effect, int timer)
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
