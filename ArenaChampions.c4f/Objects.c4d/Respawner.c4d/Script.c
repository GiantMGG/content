#strict

protected func Initialize() {
  Local(0) = 3;
  Local(1) = 2;
  return 1;
}

// TimerCall=Timer (DefCore), every Timer frames:
// the shared Local(0) charge pool serves the WHOLE team, not just the
// structure owner - any team member below Local(1) crew gets a clonk
// (explicitly made crew so it counts toward the threshold and is playable).
protected func Timer() {
  var team = GetOwner();
  if (team == NO_OWNER) return 1;
  if (Local(0) <= 0) return 1;
  var i;
  for (i = 0; i < GetPlayerCount(); i++) {
    var plr = GetPlayerByIndex(i);
    if (GetPlayerTeam(plr) == GetPlayerTeam(team) && GetCrewCount(plr) < Local(1)) {
      var clonk = CreateObject(CLNK, GetX(), GetY(), plr);
      if (clonk) MakeCrewMember(clonk, plr);
      Local(0)--;
      if (SoundExists("RespawnChime")) Sound("RespawnChime");
      if (Local(0) <= 0) break;
    }
  }
  return 1;
}
