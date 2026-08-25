#strict

protected func Initialize() {
  Local(0) = 3;
  Local(1) = 2;
  return 1;
}

protected func Timer() {
  var team = GetOwner();
  if (team == NO_OWNER) return 1;
  if (Local(0) > 0 && GetCrewCount(team) < Local(1)) {
    CreateObject(CLNK, GetX(), GetY(), team);
    Local(0)--;
  }
  return 1;
}
