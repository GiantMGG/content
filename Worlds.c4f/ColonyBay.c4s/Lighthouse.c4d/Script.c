/*-- Lighthouse --*/

#strict 2

local lenseflare;
local isLit;
local isDormant;

/* Initialisierung */

protected func Initialize()
{
    // Lighthouse starts dormant (not yet lit)
    isLit = false;
    isDormant = true;
    SetAction("Idle");
    return true;
}

/* Aktivierung - clonk enters lighthouse to light beacon */

public func Activate(int byPlayer)
{
    // Must be fully constructed
    if (GetCon() < 100)
    {
        return MessageWindow("$MsgNotComplete$", byPlayer);
    }
    // Must be night
    if (IsDay())
    {
        return MessageWindow("$MsgWaitForNight$", byPlayer);
    }
    // Already lit?
    if (isLit) return false;

    // Light the beacon!
    isLit = true;
    isDormant = false;
    Sound("Trumpet", 1);

    // Attach Lenseflare effect at lighthouse top
    if (!lenseflare)
    {
        lenseflare = CreateObject(LENS, GetX(), GetY() - 40, GetOwner());
    }

    // Schedule ending sequence (trade ship spawn) after 10 seconds (350 frames)
    ScheduleCall(this, "EndingSequence", 350, 1);

    return true;
}

/* Ending sequence - spawn trade ship */

func EndingSequence()
{
    // Spawn TradeShip from off-map (left edge)
    var iShipY = LandscapeHeight() * 30 / 100;
    CreateObject(TSHp, -50, iShipY, NO_OWNER);
    return true;
}

/* Status abfragen */

public func IsLit() { return isLit; }
public func IsDormant() { return isDormant; }

/* Kontext - light beacon */

public func ContextLight(object pClonk)
{
    [$TxtLight$|Image=LGHT|Condition=IsComplete]
    return Activate(GetOwner(pClonk));
}

private func IsComplete() { return GetCon() >= 100; }
