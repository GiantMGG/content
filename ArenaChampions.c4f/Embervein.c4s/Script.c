#strict

static pBase1, pBase2;

func Initialize() {
  SetWind(0);
  var cx = LandscapeWidth()/2;
  var cy = LandscapeHeight()/2;
  pBase1 = CreateConstruction(HUT3, cx - 300, cy - 200, NO_OWNER, 100, 1);
  pBase2 = CreateConstruction(HUT3, cx + 300, cy + 200, NO_OWNER, 100, 1);
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
  if (team == 1) PlacePlayer1(iPlr);
  if (team == 2) PlacePlayer2(iPlr);
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
