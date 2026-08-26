/*-- Riesentintenfisch --*/

#strict
#include ANIM

public func IsPossessible()    { return(1); }
public func IsDeepSeaCreature(){ return(1); }

local territory;          // claimed KLPM object
local patrol_radius;      // 250 default
local rage_timer;         // 30-second rage state after territory destroyed
local ink_used;           // has ink-retreat fired this damage cycle?

protected func Initialize()
{
  territory = 0;
  patrol_radius = 250;
  rage_timer = 0;
  ink_used = 0;
  Birth();
  // Defer territory claim until KLPM instances have finished Initialize
  ScheduleCall(this(), "ClaimTerritory", 5);
}

/* Find the nearest unclaimed KLPM and store it as territory */
private func ClaimTerritory()
{
  if (territory) return();
  var best = 0, best_d = 999999;
  var farm;
  while (farm = FindObject(KLPM, 0, 0, 0, 0, 0, 0, 0, NoContainer(), farm))
  {
    // Skip farms already claimed by another squid
    var claimed = 0;
    var other;
    while (other = FindObject(GSQD, 0, 0, 0, 0, 0, 0, 0, NoContainer(), other))
    {
      if (other == this()) continue;
      if (other->LocalN("territory") == farm) { claimed = 1; break; }
    }
    if (claimed) continue;
    var d = Abs(GetX(farm) - GetX()) + Abs(GetY(farm) - GetY());
    if (d < best_d) { best_d = d; best = farm; }
  }
  territory = best;
}

/* TimerCall — territorial patrol / intruder response */
protected func Activity()
{
  if (GetEffect("PossessionSpell", this())) return();

  // Rage timer expiry
  if (rage_timer > 0)
  {
    rage_timer--;
    if (rage_timer == 0) patrol_radius = 250;
  }

  // If our claimed farm is gone, re-claim
  if (territory && !ObjectDistance(territory)) territory = 0;
  if (!territory) ClaimTerritory();

  // Patrol centre = territory farm, else current position
  var cx = territory ? GetX(territory) : GetX();
  var cy = territory ? GetY(territory) : GetY();

  // Intruder detection: any crew member or prey inside patrol radius
  var intruder = FindObject(0, 0, 0, 0, 0, OCF_CrewMember | OCF_Prey, 0, 0, NoContainer());
  var closest = 0, closest_d = patrol_radius + 1;
  while (intruder = FindObject(0, 0, 0, 0, 0, OCF_CrewMember | OCF_Prey, 0, 0, NoContainer(), intruder))
  {
    if (!GetAlive(intruder)) continue;
    var d = ObjectDistance(intruder);
    if (d <= patrol_radius && d < closest_d) { closest_d = d; closest = intruder; }
  }

  if (closest)
  {
    // Intruder state: chase within radius only
    SetCommand(this(), "MoveTo", 0, GetX(closest), GetY(closest));
    if (closest_d < 30) Attack(closest);
    return();
  }

  // Patrol state: alternate direction with occasional drift
  if (!Random(3))
  {
    if (Random(2)) SetComDir(COMD_Left);  else SetComDir(COMD_Right);
  }
  if (!Random(5))
  {
    if (Random(2)) SetComDir(COMD_Up);    else SetComDir(COMD_Down);
  }

  // Stay within radius of patrol centre
  if (Abs(GetX() - cx) > patrol_radius)
  {
    if (GetX() > cx) SetComDir(COMD_Left); else SetComDir(COMD_Right);
  }
  if (Abs(GetY() - cy) > patrol_radius)
  {
    if (GetY() > cy) SetComDir(COMD_Up); else SetComDir(COMD_Down);
  }
}

private func Attack(pTarget)
{
  if (GetEffect("IntAttackDelay", this())) return();
  var dmg = 8;
  if (rage_timer > 0) dmg = 12;   // +50% during rage
  Punch(pTarget, dmg);
  SetAction("Jaw");
  Sound("Munch1");
  AddEffect("IntAttackDelay", this(), 1, 70);
  return(1);
}

/* Ink cloud retreat at 50% HP */
public func InkRetreat()
{
  // Emit ink-cloud bubble burst
  var i;
  for (i = 0; i < 14; i++) Bubble(RandomX(-20, 20), RandomX(-10, 10));
  // Flee upward for 2 seconds
  SetComDir(COMD_Up);
  ScheduleCall(this(), "ResumePatrol", 70);
}

private func ResumePatrol()
{
  if (GetAction() S= "Swim") SetComDir(COMD_Left);
}

protected func Damage(int iChange, int iPlr)
{
  if (!ink_used && GetEnergy() <= GetPhysical("Energy", 0) / 2)
  {
    ink_used = 1;
    InkRetreat();
  }
  // Death handled by Death() once energy hits 0
}

protected func Death()
{
  SetDir(0);
  ChangeDef(DSQD);
  SetAction("Dead");
  // Drop the squid beak into the corpse
  CreateContents(SQBK);
  return(1);
}

protected func Birth()
{
  SetAction("Swim");
  SetComDir(COMD_Left);
  if (Random(2)) SetComDir(COMD_Right);
}

/* Contact: turn around at walls */
protected func ContactLeft()  { if (GetAction() S= "Swim") TurnRight(); }
protected func ContactRight() { if (GetAction() S= "Swim") TurnLeft(); }

private func TurnRight()
{
  if (Stuck() || (GetAction() ne "Walk" && GetAction() ne "Swim")) return();
  if (GetXDir() < 0) SetXDir(0);
  SetDir(DIR_Right);
  SetComDir(COMD_Right);
  return(1);
}

private func TurnLeft()
{
  if (Stuck() || (GetAction() ne "Walk" && GetAction() ne "Swim")) return();
  if (GetXDir() > 0) SetXDir(0);
  SetDir(DIR_Left);
  SetComDir(COMD_Left);
  return(1);
}

/* Called by the KLPM Destruction() when our claimed farm is destroyed */
public func OnTerritoryDestroyed()
{
  territory = 0;
  rage_timer = 1800;   // 30 seconds at 60fps
  patrol_radius = 500; // double radius during rage
}
