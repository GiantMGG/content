/*-- Cog (CGSH) -- cargo ship. --*/
/* Self-contained sailing vehicle. Holds trade goods in its hold.          */
/* Wind2Sail clamps wind push while IsStormActive() (spec edge case #2).    */
/* On wreck, drops all trade-good contents as floating salvage             */
/* (spec edge case #3).                                                     */
/* NOTE: do NOT redefine GetCargoValue here -- TradeLib.c4d already        */
/* defines it (spec edge case #8).                                          */

#strict

/* ---- Storm helper (global; used by Cog + directors + scenarios) ---- */
global func IsStormActive() {
	return GetActiveWeatherEvent() == STRM;
}

/* ---- Initialization ---- */
protected func Initialize() { SetAction("Sailing"); }

/* ---- Controls: auto-run via SetCommand "MoveTo" ---- */
protected func ControlCommand(string szCommand, object pTarget, int iX, int iY) {
	if (szCommand eq "MoveTo") return Command2Control(iX, iY);
}

private func Command2Control(int iX, int iY) {
	if (iX > GetX()) SetComDir(COMD_Right);
	if (iX < GetX()) SetComDir(COMD_Left);
	return 1;
}

/* ---- Sailing physics with storm clamp ---- */
private func Wind2Sail() {
	var wind_push = BoundBy(GetWind() / 4, -10, +10);
	// Spec edge case #2: clamp wind coupling while a storm rages so the
	// auto-running Cog stays controllable (slower, but on-lane).
	if (IsStormActive()) wind_push = BoundBy(wind_push, -3, +3);
	var xdir = 7 + wind_push / 5;
	if (GetComDir() == COMD_None())  SetXDir(wind_push);
	if (GetComDir() == COMD_Left())  SetXDir(-xdir);
	if (GetComDir() == COMD_Right()) SetXDir(+xdir);
}

/* ---- Sinking: drop cargo as salvage, then remove (edge case #3) ---- */
protected func Damage() {
	if (GetDamage() < 150) return 0;
	// Drop every trade-good content at the Cog's position so it is
	// salvageable / pirateable rather than silently voided.
	var pObj;
	for (pObj in FindObjects(Find_Container(this()), Find_Func("IsTradeGood")))
		pObj->Exit(GetX(), GetY(), 0, 0, 0, 0);
	// Then remove the wrecked Cog.
	RemoveObject();
	return 1;
}

/* ---- Identity ---- */
public func IsCargoShip() { return 1; }
