#strict

protected func Activate(byPlr) {
  if (SoundExists("ChestOpen")) Sound("ChestOpen");
  return 1;
}

// F5 (cycle-172 critic fix-now): the chest accepts items whose owner is on
// the SAME TEAM as the chest's owner - teammates feed the shared store
// (same-player still passes: a player is always on their own team). Owners
// that are NO_OWNER (<= 0) or on no team (GetPlayerTeam <= 0) keep being
// rejected, as before.
protected func RejectCollect(id def, object item) {
  var chestTeam = GetPlayerTeam(GetOwner());
  if (chestTeam > 0 && GetPlayerTeam(GetOwner(item)) == chestTeam) return 0;
  return 1;
}
