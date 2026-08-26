/*-- Drucktaucheranzug --*/

#strict

local turn_end_dir;
local lamp;              // attached headlamp glow-overlay sprite

/* Initialisierung */

protected func Completion()
{
  SetAction("Walk");
  SetComDir(COMD_Stop);
  turn_end_dir = -1;
  // Attach headlamp glow-overlay sprite (faked — no AddLight in LegacyClonk)
  lamp = CreateObject(AGLL, 12 * (GetDir() == DIR_Right ? 1 : -1), -8, GetOwner());
  if (lamp)
  {
    lamp->SetAction("Attach", this());
    lamp->SetCategory(C4D_Object | C4D_Background | C4D_Parallax);
  }
}

/* Steuerung — pattern verbatim from Sub.c4d/Script.c:18-64 */

private func TurnEnd()
{
  if(turn_end_dir != -1)
  {
    SetDirection(turn_end_dir);
    turn_end_dir = -1;
  }
}

public func SetDirection(int comdir)
{
  ClearCommand();

  if(GetAction() S= "Turn")
  {
    turn_end_dir = comdir;
    return();
  }

  if(GetAction() ne "Swim") return();

  SetComDir(comdir);

  if(comdir == COMD_Left || comdir == COMD_Right)
    SetYDir(0);
  if(comdir == COMD_Up || comdir == COMD_Down)
    SetXDir(0);

  if(ComDirLike(comdir, COMD_Right))
  {
    if(GetDir() == DIR_Left)
    {
      SetAction("Turn");
      SetDir(DIR_Right);
    }
  }

  if(ComDirLike(comdir, COMD_Left))
  {
    if(GetDir() == DIR_Right)
    {
      SetAction("Turn");
      SetDir(DIR_Left);
    }
  }
}

protected func ContainedUpdate(object clonk, int comdir, bool dig, bool throw)
{
  SetDirection(comdir);
}

protected func ContainedUp(object clonk)
{
  [$TxtComUp$|Image=SSY1:1]
  CloseAirlock();
  if(!GetPlrJumpAndRunControl(clonk->GetController()))
    SetDirection(ComDirTransform(GetComDir(), COMD_Up));
  return(1);
}

protected func ContainedDown(object clonk)
{
  [$TxtComDown$|Image=SSY1:0]
  CloseAirlock();
  if(!GetPlrJumpAndRunControl(clonk->GetController()))
    SetDirection(ComDirTransform(GetComDir(), COMD_Down));
  return(1);
}

protected func ContainedRight(object clonk)
{
  [$TxtComRight$|Image=SSY1:4]
  if(!GetPlrJumpAndRunControl(clonk->GetController()))
    SetDirection(ComDirTransform(GetComDir(), COMD_Right));
  return(1);
}

protected func ContainedLeft(object clonk)
{
  [$TxtComLeft$|Image=SSY1:3]
  if(!GetPlrJumpAndRunControl(clonk->GetController()))
    SetDirection(ComDirTransform(GetComDir(), COMD_Left));
  return(1);
}

protected func ContainedUpDouble(object pByObject)
{
  [$TxtExit$|Image=SSY1:5]
  OpenAirlock();
  Exit(pByObject);
  SetDirection(COMD_Stop);
}

/* ContainedDig: NO combat — differentiates from SUB1. */
protected func ContainedDig(object clonk)
{
  [$TxtNothing$|Image=SSY1:2]
  Sound("Error");
  return(1);
}

/* ContainedThrow: drop a CNKT for abyssal construction */
protected func ContainedThrow(object clonk)
{
  [$TxtDropKit$|Image=SSY1:6]
  var kit = FindContents(CNKT);
  if(!kit)
  {
    Sound("Error");
    return(1);
  }
  SetCommand(kit, "Exit");
  return(1);
}

/* Kontakt */

protected func ContactLeft()
{
  if(GetAction() ne "Swim") return(0);
  if(ComDirLike(GetComDir(), COMD_Left))
    SetDirection(COMD_Stop);
}

protected func ContactRight()
{
  if(GetAction() ne "Swim") return(0);
  if(ComDirLike(GetComDir(), COMD_Right))
    SetDirection(COMD_Stop);
}

/* Funktionen */

private func ClearCommand()
{
  SetCommand(this(), "None");
}

private func SwimProcess()
{
  Bubbling();
}

private func Bubbling()
{
  Bubble(0, Random(4));
  Bubble(0, -5);
  Bubble(0, 5);
}

private func OpenAirlock()
{
  if(GetEntrance()) return(0);
  SetEntrance(1);
  Sound("Airlock1");
  return(1);
}

private func CloseAirlock()
{
  if(!GetEntrance()) return(0);
  SetEntrance(0);
  Sound("Airlock2");
  return(1);
}

private func Damage()
{
  var i = 0;
  if(GetDamage() < 200) return(0);
  // Suit destroyed — eject occupant
  var occ = Contents();
  if(occ) Exit(occ);
  if(lamp) RemoveObject(lamp);
  ChangeDef(SUBB);   // reuse Sub.c4d burnout subdef for the wreck
  SetAction("Wrecked");
  DoCon(-5);
  while(i++ < 14) Bubble(Random(30), Random(20));
}

/* Callbacks */

public func IsPressurized()     { return(1); }
public func IsDeepSeaResearch() { return(1); }
public func GetResearchBase()   { return(KLPM); }
