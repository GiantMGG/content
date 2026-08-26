/*-- Handelsschiff --*/

#strict 2

local hasGrantedKnowledge;
local iTimeout;

/* Initialisierung */

protected func Initialize()
{
    hasGrantedKnowledge = false;
    iTimeout = 0;

    // Sail toward the dock (beach area at 40% width, 50% height)
    var iDockX = LandscapeWidth() * 40 / 100;
    var iDockY = LandscapeHeight() * 50 / 100;
    SetCommand(this, "MoveTo", 0, iDockX, iDockY);

    // Add timer effect to check arrival (runs every 10 frames)
    AddEffect("SailToDock", this, 1, 10, this);

    return true;
}

/* SailToDock effect - checks if ship arrived at dock */

func FxSailToDockTimer(object target, int effect, int timer)
{
    if (hasGrantedKnowledge) return -1; // remove effect

    var iDockX = LandscapeWidth() * 40 / 100;
    var iDockY = LandscapeHeight() * 50 / 100;

    // Check if ship is near dock
    if (Distance(GetX(), GetY(), iDockX, iDockY) < 50)
    {
        GrantChemKnowledge();
        return -1;
    }

    // Timeout after 30 seconds (30 * 10 = 300 frames at 10-frame interval)
    iTimeout++;
    if (iTimeout > 30)
    {
        GrantChemKnowledge();
        return -1;
    }

    return 1;
}

/* Grant CHEM knowledge to all players */

func GrantChemKnowledge()
{
    if (hasGrantedKnowledge) return;
    hasGrantedKnowledge = true;

    for (var i = 0; i < GetPlayerCount(); i++)
    {
        SetPlrKnowledge(GetPlayerByIndex(i), CHEM);
    }

    // Announce arrival
    Message("$MsgShipArrived$");
}
