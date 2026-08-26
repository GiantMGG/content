/*-- Handelsschiff --*/

#strict 2

local hasGrantedKnowledge;
local iTimeout;

/* Initialisierung */

protected func Initialize()
{
    hasGrantedKnowledge = false;
    iTimeout = 0;

    // Add timer effect to sail toward the dock and check arrival
    // (runs every 10 frames). A driverless C4D_Vehicle won't execute a
    // "MoveTo" command, so the timer applies velocity directly toward
    // the dock each tick. (Review M-5.)
    AddEffect("SailToDock", this, 1, 10, this);

    return true;
}

/* SailToDock effect - moves the ship toward the dock and checks arrival */

func FxSailToDockTimer(object target, int effect, int timer)
{
    if (hasGrantedKnowledge) return -1; // remove effect

    var iDockX = LandscapeWidth() * 40 / 100;
    var iDockY = LandscapeHeight() * 50 / 100;

    // Apply velocity toward the dock (roughly 2 px/frame).
    var iDX = iDockX - GetX();
    var iDY = iDockY - GetY();
    var iDist = Distance(iDX, iDY, 0, 0);
    if (iDist > 0)
    {
        var iSpeed = 2;
        SetXDir(iDX * iSpeed / iDist);
        SetYDir(iDY * iSpeed / iDist);
    }

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
