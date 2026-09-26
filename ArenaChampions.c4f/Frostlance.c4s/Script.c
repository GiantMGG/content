#strict

static pBase1, pBase2;

func Initialize() {
  SetWind(0);
  AddEffect("ArenaTeams", 0, 1, 35); // F3: auto-team + place teamless joins
  var cx = LandscapeWidth()/2;
  var cy = LandscapeHeight()/2;
  pBase1 = CreateConstruction(HUT3, cx - 300, cy - 200, NO_OWNER, 100, 1);
  pBase2 = CreateConstruction(HUT3, cx + 300, cy + 200, NO_OWNER, 100, 1);
  CreateConstruction(CST3, cx, cy, NO_OWNER, 100, 1);
  CreateObject(TCHS, cx - 300, cy - 200, NO_OWNER);
  CreateObject(TCHS, cx + 300, cy + 200, NO_OWNER);
  CreateObject(RSPN, cx - 300, cy - 200, NO_OWNER);
  CreateObject(RSPN, cx + 300, cy + 200, NO_OWNER);
  CreateObject(BNDR, 0, 0, NO_OWNER);
  CreateObject(BNDR, LandscapeWidth(), 0, NO_OWNER);
  CreateObject(BNDR, 0, LandscapeHeight(), NO_OWNER);
  CreateObject(BNDR, LandscapeWidth(), LandscapeHeight(), NO_OWNER);
  return 1;
}

protected func InitializePlayer(iPlr, x, y, bas, team) {
  // F3 (cycle-172 critic fix-now): with TeamDistribution=Free a join reaches
  // this callback teamless (team=0) BEFORE the team list is compiled - do
  // nothing here; the ArenaTeams poll below assigns a team and re-enters
  // this callback (via InitScenarioPlayer's ~InitializePlayer broadcast)
  // with the chosen team on the first 35-frame tick.
  if (team == 1) PlacePlayer1(iPlr);
  else if (team == 2) PlacePlayer2(iPlr);
  return 1;
}

// F3 (cycle-172 critic fix-now): 4 headless/console joins on
// TeamDistribution=Free never resolve their team selection (the engine's
// GetForcedTeamSelection needs exactly ONE joinable team - with two it stays
// 0), so nobody was placed and MELE/KILT never saw two teams: the round
// stalled. This global poll (gotcha #4 form) assigns the SMALLER team
// (tie -> team 1) to every teamless player through the engine's own
// InitScenarioPlayer(plr, team) -> C4Player::ScenarioAndTeamInit -> the
// regular ~InitializePlayer broadcast, so placement + the join logs run in
// the scenario context. Waits for GetTeamCount() > 0: the Teams.txt list is
// compiled only after the first joins landed.
global func FxArenaTeamsTimer(target, effect, time)
{
  if (GetTeamCount() <= 0) return 1;
  var i, done = 0;
  for (i = 0; i < GetPlayerCount(); i++)
  {
    var plr = GetPlayerByIndex(i);
    if (GetPlayerTeam(plr) <= 0)
    {
      InitScenarioPlayer(plr, ArenaSmallerTeam());
      done++;
    }
  }
  if (!done) return -1;   // every joined player has a team: stop polling
  return 1;
}

global func ArenaSmallerTeam()
{
  var c1 = 0, c2 = 0, i;
  for (i = 0; i < GetPlayerCount(); i++)
  {
    var pt = GetPlayerTeam(GetPlayerByIndex(i));
    if (pt == 1) c1++;
    else if (pt == 2) c2++;
  }
  if (c2 < c1) return 2;
  return 1;
}


private func PlacePlayer1(int iPlr) {
  var objs = FindObjects(Find_Category(C4D_Structure), Find_InRect(0, 0, LandscapeWidth()/2, LandscapeHeight()));
  for (var i = GetLength(objs); i > 0; i--)
    if (GetOwner(objs[i-1]) == -1) SetOwner(iPlr, objs[i-1]);
  if (pBase1) {
    for (var i; i < GetCrewCount(iPlr); i++) Enter(pBase1, GetCrew(iPlr, i));
    Enter(pBase1, CreateObject(FLAG, 0, 0, iPlr));
  } else for (var i; i < GetCrewCount(iPlr); i++) SetPosition(LandscapeWidth()/2 - 300, LandscapeHeight()/2 - 200, GetCrew(iPlr, i));
  Log("$TeamLeftJoin$", GetPlayerName(iPlr), Format("$TeamLeft$"));
  return 1;
}

private func PlacePlayer2(int iPlr) {
  var objs = FindObjects(Find_Category(C4D_Structure), Find_InRect(LandscapeWidth()/2, 0, LandscapeWidth(), LandscapeHeight()));
  for (var i = GetLength(objs); i > 0; i--)
    if (GetOwner(objs[i-1]) == -1) SetOwner(iPlr, objs[i-1]);
  if (pBase2) {
    for (var i; i < GetCrewCount(iPlr); i++) Enter(pBase2, GetCrew(iPlr, i));
    Enter(pBase2, CreateObject(FLAG, 0, 0, iPlr));
  } else for (var i; i < GetCrewCount(iPlr); i++) SetPosition(LandscapeWidth()/2 + 300, LandscapeHeight()/2 + 200, GetCrew(iPlr, i));
  Log("$TeamRightJoin$", GetPlayerName(iPlr), Format("$TeamRight$"));
  return 1;
}
