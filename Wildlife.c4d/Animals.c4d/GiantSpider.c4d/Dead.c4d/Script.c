/*-- Dead Spider --*/

#strict 3

protected func Initialize() { SetAction("Dead"); return true; }

public func Embowel() { Split2Components(this()); return true; }

private func Decaying() { DoCon(-4); }

public func Eat(object pByObject) { ObjectCall(pByObject, "Feed", 100); RemoveObject(); return true; }
