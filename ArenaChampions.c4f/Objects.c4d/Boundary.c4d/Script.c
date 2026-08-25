#strict

protected func Timer() {
  var victims = FindObjects(Find_InRect(GetX() - 50, GetY() - 50, 100, 100), Find_OCF(OCF_CrewMember));
  for (var v in victims)
    DoEnergy(-5, v);
  return 1;
}
