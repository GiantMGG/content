#strict

static pBase1, pBase2;
static waterLevel;

func Initialize() {
  SetWind(0);
  pBase1 = CreateConstruction(HUT3, 200, 270, NO_OWNER, 100, 1);
  pBase2 = CreateConstruction(HUT3, LandscapeWidth()-200, 270, NO_OWNER, 100, 1);
  CreateObject(TCHS, 200, 270, NO_OWNER);
  CreateObject(TCHS, LandscapeWidth()-200, 270, NO_OWNER);
  CreateObject(RSPN, 200, 270, NO_OWNER);
  CreateObject(RSPN, LandscapeWidth()-200, 270, NO_OWNER);
  CreateObject(BNDR, 0, 0, NO_OWNER);
  CreateObject(BNDR, LandscapeWidth(), 0, NO_OWNER);
  CreateObject(BNDR, 0, LandscapeHeight(), NO_OWNER);
  CreateObject(BNDR, LandscapeWidth(), LandscapeHeight(), NO_OWNER);
  waterLevel = 0;
  ScriptGo(1);
  // Rising tide: one raise every 10 seconds. The scenario-script counter
  // (C4ScriptHost) calls each ScriptN exactly once, so the periodic cadence
  // is driven by a 350-frame effect; Script10 below covers the first call.
  AddEffect("Tide", 0, 1, 350);
  return 1;
}

/* Rising-water hook: raise water level every 10 seconds. */
global func FxTideTimer(target, effect, time) {
  RaiseWater();
  return 1;
}

/* First raise (the scenario-script counter calls Script10 once). */
func Script10() {
  RaiseWater();
  ScriptGo(1);
  return 1;
}

/* Paints a 5 px tall band of Water-Smooth across the full map width at the
   level that just rose above the previous one (starts at the bottom).
   Global: the Tide effect timer runs in the engine context and cannot see
   scenario-private functions. */
global func RaiseWater() {
  var y = LandscapeHeight() - waterLevel;
  DrawMaterialQuad("Water-Smooth", 0, y, 0, y + 5, LandscapeWidth(), y + 5, LandscapeWidth(), y);
  waterLevel += 5;
  if (SoundExists("TideAlert")) Sound("TideAlert");
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
  } else for (var i; i < GetCrewCount(iPlr); i++) SetPosition(200, 270, GetCrew(iPlr, i));
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
  } else for (var i; i < GetCrewCount(iPlr); i++) SetPosition(LandscapeWidth()-200, 270, GetCrew(iPlr, i));
  Log("$TeamRightJoin$", GetPlayerName(iPlr), Format("$TeamRight$"));
  return 1;
}
