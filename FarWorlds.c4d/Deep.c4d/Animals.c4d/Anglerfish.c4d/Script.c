/*-- Anglerfisch --*/

#strict
#include ANIM

public func IsPossessible()    { return(0); }   // not possessible by default
public func IsDeepSeaCreature(){ return(1); }

local lure;                // attached bioluminescent lure overlay object
local flee_timer;          // 10s despawn window after Flee

protected func Initialize()
{
  Birth();
  // Spawn the attached lure-glow overlay sprite (faked bioluminescence)
  lure = CreateObject(AGLL, 0, -10, GetOwner());
  if (lure)
  {
    lure->SetAction("Attach", this());
    lure->SetCategory(C4D_Object | C4D_Background | C4D_Parallax);
  }
}

/* TimerCall — ambush AI */
protected func Activity()
{
  if (GetEffect("PossessionSpell", this())) return();

  // Flee state: dive deeper, despawn after 10s
  if (flee_timer > 0)
  {
    flee_timer--;
    if (flee_timer == 1)
    {
      if (lure) RemoveObject(lure);
      RemoveObject();
    }
    return();
  }

  // Stay stationary unless prey is in the detection rect (60x40 in front)
  var detect_x = 30 * (GetDir() == DIR_Right ? 1 : -1);
  var prey = FindObject(0, detect_x - 30, -20, 60, 40, OCF_Prey, 0, 0, NoContainer());
  if (prey && GetAlive(prey))
  {
    if (!GetEffect("IntAttackDelay", this()))
    {
      SetAction("Lunge");
      Punch(prey, 10);
      AddEffect("IntAttackDelay", this(), 1, 70, this());
    }
    return();
  }

  // Default: drift slowly with occasional direction change
  if (!Random(20))
  {
    if (Random(2)) SetComDir(COMD_Left); else SetComDir(COMD_Right);
  }
}

/* Helper: sample GetMaterialColor in a radius, return darkest pixel coords */
private func Find_FurthestFromLight()
{
  var best_x = GetX(), best_y = GetY(), best_dark = -1;
  var r;
  for (r = 10; r <= 60; r += 10)
  {
    var a;
    for (a = 0; a < 360; a += 30)
    {
      var sx = GetX() + Cos(a, r);
      var sy = GetY() + Sin(a, r);
      var col = GetMaterialColor(GetMaterial(sx - GetX(), sy - GetY()), 0, 0);
      if (col == -1) continue;
      // Darkness = 765 - (R+G+B)
      var dark = 765 - (GetRGBValue(col, 1) + GetRGBValue(col, 2) + GetRGBValue(col, 3));
      if (dark > best_dark) { best_dark = dark; best_x = sx; best_y = sy; }
    }
  }
  return([best_x, best_y]);
}

/* Flee state below 30% HP */
protected func Damage(int iChange, int iPlr)
{
  if (GetEnergy() < GetPhysical("Energy", 0) * 3 / 10)
  {
    SetComDir(COMD_Down);
    flee_timer = 600;   // 10 seconds at 60fps
  }
}

protected func Death()
{
  SetDir(0);
  ChangeDef(DAGLF);
  SetAction("Dead");
  CreateContents(AGLL);
  if (lure) RemoveObject(lure);
  return(1);
}

protected func Birth()
{
  SetAction("Swim");
  SetComDir(COMD_Stop);
}

protected func ContactLeft()  { if (GetAction() S= "Swim") TurnRight(); }
protected func ContactRight() { if (GetAction() S= "Swim") TurnLeft(); }

private func TurnRight()
{
  if (Stuck()) return();
  SetDir(DIR_Right);
  SetComDir(COMD_Right);
}

private func TurnLeft()
{
  if (Stuck()) return();
  SetDir(DIR_Left);
  SetComDir(COMD_Left);
}

/* Called by DeepAbyss.c4s script on cleanup */
public func GetLure() { return(lure); }
