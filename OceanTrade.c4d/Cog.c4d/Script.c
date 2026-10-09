/*-- Cog (CGSH) -- cargo ship. --*/
/* Self-contained sailing vehicle. Holds trade goods in its hold.          */
/* Wind2Sail clamps wind push while IsStormActive() (spec edge case #2).    */
/* On wreck, drops all trade-good contents as floating salvage             */
/* (spec edge case #3).                                                     */
/* NOTE: do NOT redefine GetCargoValue here -- TradeLib.c4d already        */
/* defines it (spec edge case #8).                                          */

#strict

/* ---- Flood machine constants (cycle 213 flooded-crown; frozen at T2
   calibration -- the FloodedCrownSmoke phase asserts pin these values).
   Duplicated in the smoke's Script.c and in CogWreck.c4d/Script.c for
   SINK_TERMINAL; keep the trio in sync. ---- */
static const BREACH_FILL    = 1; // flood units per open breach per frame
static const AWASH_AT       = 100; // settle-band start
static const FOUNDER_AT     = 190; // founder threshold
static const SETTLE_BIAS    = 2; // bounded SetYDir downward bias while AWASH<=flood<FOUNDER
static const FREEBOARD_GAIN = 25; // iFlood reduction per jettisoned bale
static const STORM_FILL_MULT = 2; // fill multiplier while IsStormActive()

/* Hull-effect EffectVar slots (indices into the effect's variable list). */
static const FLV_BREACHES = 0; // opened breach slots
static const FLV_PLUGGED  = 1; // plugged breach slots
static const FLV_FLOOD    = 2; // bilge units

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

/* ---- Flood machine (cycle 213 flooded-crown; additive, passive-safe) ---- */
/* All flood state lives in EffectVar slots on a per-Cog "Hull" effect       */
/* (plain name, gotchas #10). The effect is created lazily by the first      */
/* OpenBreach() so passive Cogs pay nothing (three pre-existing OceanTrade   */
/* smokes exercise only the passive paths). FxHullTimer (interval 1) drives  */
/* fill -> settle -> founder with integer math only; NO Random() here.       */
/* Stroke semantics: a STRM storm multiplies inflow by STORM_FILL_MULT while */
/* IsStormActive(). Settle applies a bounded downward SetYDir bias (R1: no   */
/* Con). Founder ejects trade-good contents as floating salvage (the         */
/* Damage()-path idiom), swaps to the CWCK wreck def and self-removes.       */

/* +1 breach slot; first call arms the Hull effect (lazy, passive-safe). */
public func OpenBreach() {
	var iFx = GetEffect("Hull", this);
	if (!iFx)
		iFx = AddEffect("Hull", this, 1, 1, this);
	EffectVar(FLV_BREACHES, this, iFx) = EffectVar(FLV_BREACHES, this, iFx) + 1;
	return 1;
}

/* Plug one unplugged slot (boatswain/smoke API). */
public func PlugBreach() {
	var iFx = GetEffect("Hull", this);
	if (!iFx) return 0;
	var iOpen = EffectVar(FLV_BREACHES, this, iFx) - EffectVar(FLV_PLUGGED, this, iFx);
	if (iOpen <= 0) return 0;
	EffectVar(FLV_PLUGGED, this, iFx) = EffectVar(FLV_PLUGGED, this, iFx) + 1;
	return 1;
}

/* Unplugged breach-slot count. */
public func GetOpenBreaches() {
	var iFx = GetEffect("Hull", this);
	if (!iFx) return 0;
	return EffectVar(FLV_BREACHES, this, iFx) - EffectVar(FLV_PLUGGED, this, iFx);
}

/* Current bilge level. */
public func GetFloodLevel() {
	var iFx = GetEffect("Hull", this);
	if (!iFx) return 0;
	return EffectVar(FLV_FLOOD, this, iFx);
}

/* 0 DRY / 1 AWASH / 2 FOUNDERED. */
public func GetFloodPhase() {
	var iFlood = GetFloodLevel();
	if (iFlood >= FOUNDER_AT) return 2;
	if (iFlood >= AWASH_AT) return 1;
	return 0;
}

/* Pump action: reduce bilge, floor 0 (playable). */
public func BailOut(int iAmt) {
	if (iAmt <= 0) return 1;
	var iFx = GetEffect("Hull", this);
	if (!iFx) return 1;
	var iFlood = EffectVar(FLV_FLOOD, this, iFx) - iAmt;
	if (iFlood < 0) iFlood = 0;
	EffectVar(FLV_FLOOD, this, iFx) = iFlood;
	return 1;
}

/* Jettison one trade-good bale; each raises freeboard (iFlood -= FREEBOARD_GAIN). */
public func JettisonCargo() {
	var iFx = GetEffect("Hull", this);
	if (!iFx) return 0;
	var pObj;
	for (pObj in FindObjects(Find_Container(this()), Find_Func("IsTradeGood"))) {
		Exit(pObj, 70, 0, 0, 0, 0, 0); // heave over the side (clears the
		break;                          // Collection box so it stays salvage)
	}
	if (!pObj) return 0;
	var iFlood = EffectVar(FLV_FLOOD, this, iFx) - FREEBOARD_GAIN;
	if (iFlood < 0) iFlood = 0;
	EffectVar(FLV_FLOOD, this, iFx) = iFlood;
	return 1;
}

/* Per-frame flood machine (interval 1; runs only while the Hull effect lives). */
func FxHullTimer(object target, int effect, int timer) {
	var iOpen  = EffectVar(FLV_BREACHES, target, effect) - EffectVar(FLV_PLUGGED, target, effect);
	var iFlood = EffectVar(FLV_FLOOD, target, effect);

	// Fill: storm inflow multiplies (if/else, no ternary - gotchas #2).
	if (IsStormActive())
		iFlood += iOpen * BREACH_FILL * STORM_FILL_MULT;
	else
		iFlood += iOpen * BREACH_FILL;

	// Settle (R1-safe: no Con): bounded downward bias while AWASH <= flood < FOUNDER.
	if (iFlood >= AWASH_AT && iFlood < FOUNDER_AT)
		SetYDir(SETTLE_BIAS);

	// Founder: eject trade-good contents as floating salvage, swap to the
	// wreck def, then self-remove (return -1; clean handoff to CWCK TimerCall).
	if (iFlood >= FOUNDER_AT) {
		EffectVar(FLV_FLOOD, target, effect) = iFlood;
		var pObj;
		for (pObj in FindObjects(Find_Container(target), Find_Func("IsTradeGood")))
			Exit(pObj, 70, 0, 0, 0, 0, 0); // heave over the side (clears the
		//                          Collection box so the bales stay salvage)
		ChangeDef(CWCK);
		return -1;
	}

	EffectVar(FLV_FLOOD, target, effect) = iFlood;
	return 1;
}
