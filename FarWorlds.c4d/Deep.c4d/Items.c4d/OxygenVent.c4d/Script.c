/*-- Sauerstoff-Blasenquelle --*/

#strict

protected func Initialize()
{
  SetAction("Pump");
}

/* Pump cycle: StartCall of Pump action — fires every frame the action runs.
   We throttle to once per 30 ticks via an internal counter. */
local pump_counter;

protected func PumpCycle()
{
  if (++pump_counter < 30) return();
  pump_counter = 0;

  // Spawn a cluster of oxygen bubbles from the vent mouth
  var i;
  for (i = 0; i < 6; i++)
  {
    var b = CreateObject(FXU1, RandomX(-6, 6), -25, GetOwner());
    if (b)
    {
      b->SetAction("Bubble");
      b->SetComDir(COMD_Up());
      b->SetXDir(RandomX(-5, 5));
      // Attach a breath-on-contact callback via a global effect on the bubble
      AddEffect("IntOxygenBubble", b, 1, 1);
    }
  }
}

/* Damage / destruction */
public func IsOxygenVent()        { return(1); }
public func IsDeepSeaResearch()   { return(1); }
public func GetResearchBase()     { return(KLPM); }

protected func Damage()
{
  if (GetDamage() < 100) return();
  Destruction();
}

protected func Destruction()
{
  // One large bubble burst, then stops producing (object removed)
  var i;
  for (i = 0; i < 20; i++) Bubble(RandomX(-15, 15), RandomX(-10, 10));
}

/* Breath-on-contact effect for spawned bubbles */
global func FxIntOxygenBubbleTimer(pObj, iNum, iA)
{
  if (!pObj) return(-1);
  for (var prey in FindObjects(
      Find_Distance(8, GetX(pObj), GetY(pObj)),
      Find_OCF(OCF_Alive),
      Find_NoContainer()))
  {
    if (prey == pObj) continue;
    DoBreath(25, prey);
  }
  return(1);
}
