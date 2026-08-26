/*-- Kelpfarm --*/

#strict

local stored_fibre;
local grow_timer;

protected func Initialize()
{
  stored_fibre = 0;
  grow_timer = 0;
  SetAction("Growth");
}

/* Depth + material gate */
private func CanGrowHere()
{
  // Must be in Earth or Coral material at the base
  var mat = GetMaterial(0, 5);
  if (mat != Material("Earth") && mat != Material("Coral")) return(false);
  // Must be above the Abyss line (Sunlight or Twilight band only)
  if (GetY() > 900) return(false);
  return(true);
}

/* TimerCall — growth + production */
protected func Growth()
{
  if (!CanGrowHere())
  {
    // Wrong place — wilt
    DoCon(-1);
    if (GetCon() <= 1) RemoveObject();
    return();
  }

  // Grow to full
  if (GetCon() < 100)
  {
    DoCon(+1);
    return();
  }

  // At full growth: produce a KLPF every 60 ticks, capped at 5
  grow_timer++;
  if (grow_timer >= 60 && stored_fibre < 5)
  {
    stored_fibre++;
    grow_timer = 0;
  }
  else if (stored_fibre >= 5)
  {
    grow_timer = 0;
  }
}

/* Clonk harvests the stored fibre */
public func Activate(pClonk)
{
  if (stored_fibre <= 0)
  {
    if (pClonk) pClonk->Message("$TxtNoFibre$");
    return(1);
  }
  while (stored_fibre > 0)
  {
    var fibre = CreateContents(KLPF, pClonk);
    if (!fibre) Enter(pClonk, CreateObject(KLPF, 0, 0, GetOwner()));
    stored_fibre--;
  }
  Sound("Munch1");
  return(1);
}

/* Damage / destruction */
public func IsKelpFarm()           { return(1); }
public func IsDeepSeaResearch()    { return(1); }
public func GetResearchBase()      { return(RSU1); }

protected func Damage()
{
  if (GetDamage() < 150) return();
  Destruction();
}

protected func Destruction()
{
  // Drop all stored fibre
  var i;
  for (i = 0; i < stored_fibre; i++)
    CreateObject(KLPF, GetX(), GetY(), GetOwner());
  // Bubble burst
  for (i = 0; i < 14; i++) Bubble(Random(30), Random(20));
  // Notify any giant squid that claimed this farm as territory
  var s;
  while (s = FindObject(GSQD, 0, 0, 0, 0, 0, 0, 0, NoContainer(), s))
    if (s->LocalN("territory") == this())
      s->~OnTerritoryDestroyed();
}
