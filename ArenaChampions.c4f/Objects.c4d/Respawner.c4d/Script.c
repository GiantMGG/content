#strict

protected func Initialize() {
  Local(0) = 3;
  Local(1) = 2;
  // F4: one board message announcing the shared charge pool + threshold
  Log("$MsgReady$");
  return 1;
}

// TimerCall=Timer (DefCore), every Timer frames:
// the shared Local(0) charge pool serves the WHOLE team, not just the
// structure owner - any team member below Local(1) crew gets a clonk
// (explicitly made crew so it counts toward the threshold and is playable).
// Local(2) = depleted-latch: the "charges exhausted" board message fires
// exactly once per object life (new round -> new object -> fresh latch).
protected func Timer() {
  var team = GetOwner();
  if (team == NO_OWNER) return 1;
  if (Local(0) <= 0) return 1;
  var i;
  for (i = 0; i < GetPlayerCount(); i++) {
    var plr = GetPlayerByIndex(i);
    if (GetPlayerTeam(plr) == GetPlayerTeam(team) && GetCrewCount(plr) < Local(1)) {
      // CreateObject offsets are RELATIVE to the calling object
      // (C4Script.cpp:1994-1997) - the old GetX()/GetY() form spawned the
      // clonk at DOUBLE the respawner position (cycle 235 finding: a
      // respawner away from the origin spawned its crew into the void, and
      // the DustrunnerCrossing retry loop burned all charges on spawn-kills)
      var clonk = CreateObject(CLNK, 0, 0, plr);
      if (clonk) MakeCrewMember(clonk, plr);
      Local(0)--;
      if (SoundExists("RespawnChime")) Sound("RespawnChime");
      if (Local(0) <= 0) {
        // F4: the LAST charge was spent on this spawn
        if (!Local(2)) {
          Local(2) = 1;
          Log("$MsgDepleted$");
        }
        break;
      }
    }
  }
  return 1;
}
