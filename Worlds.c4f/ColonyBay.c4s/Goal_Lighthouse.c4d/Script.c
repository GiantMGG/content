/*-- Spielziel: Leuchtturm --*/

#strict 2

#include GOAL

local missionPassword = "ColonyBay";

/* Initialisierung */

protected func Initialize()
{
    SetMissionAccess("ColonyBay");
    SetAction("Idle");
    return _inherited();
}

/* Ist das Ziel erfullt? */

public func IsFulfilled()
{
    // Find the lighthouse (LGHT)
    var pLighthouse = FindObject(LGHT);
    if (!pLighthouse) return false;

    // Lighthouse must be lit
    if (!pLighthouse->~IsLit()) return false;

    // CHEM workshop must exist (fully built)
    if (ObjectCount(CHEM, 0, 0, 0, 0, OCF_Fullcon()) == 0) return false;

    return true;
}

/* Aktivierung - show goal status menu */

protected func Activate(int iPlr)
{
    if (IsFulfilled())
    {
        return MessageWindow("$MsgGoalFulfilled$", iPlr);
    }
    return MessageWindow("$MsgGoalDesc$", iPlr);
}
