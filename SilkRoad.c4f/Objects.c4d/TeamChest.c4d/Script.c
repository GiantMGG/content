#strict

protected func Activate(byPlr) {
  return 1;
}

protected func RejectCollect(id def, object item) {
  if (GetOwner(item) == GetOwner()) return 0;
  return 1;
}
