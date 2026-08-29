/*-- Wolf Pelt --*/

#strict 3

protected func Initialize() { SetAction("Idle"); return true; }

protected func Hit() { Sound("WoodHit*"); }
