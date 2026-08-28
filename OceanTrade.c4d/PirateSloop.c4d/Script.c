/*-- PirateSloop (PSLP) -- hostile ship. --*/
/* Patrols waypoints; on spotting a CGSH cargo ship within sensor range, */
/* breaks patrol and grapples. Never targets own team's Cog              */
/* (spec edge case #5: pirates are NO_OWNER, so Find_ID(CGSH) plus       */
/*  Find_Exclude(this) only ever acquires player Cogs).                   */

#strict

local patrolFx;

public func IsPirateShip() { return 1; }

protected func Initialize() {
	SetAction("Float");
	patrolFx = 0;
}

/* ---- Patrol API (called from scenario / PirateDirector) ---- */
public func StartPatrol(array aWaypoints) {
	patrolFx = AddEffect("FxPatrolLane", this, 1, 10, this);
	EffectVar(0, this, patrolFx) = aWaypoints;
	EffectVar(1, this, patrolFx) = 0;  // current waypoint index
	return 1;
}

func FxPatrolLaneStart(object pTarget, int fx, bool temp) { return 1; }

func FxPatrolLaneTimer(object pTarget, int fx) {
	var aWP = EffectVar(0, pTarget, fx);
	var iWP = EffectVar(1, pTarget, fx);
	if (GetLength(aWP) == 0) return 1;
	var iTargetX = aWP[iWP];
	if (Abs(GetX(pTarget) - iTargetX) < 20) {
		EffectVar(1, pTarget, fx) = (iWP + 1) % GetLength(aWP);
		return 1;
	}
	if (iTargetX > GetX(pTarget)) SetComDir(COMD_Right, pTarget);
	else SetComDir(COMD_Left, pTarget);
	return 1;
}

/* ---- Per-tick: detect cargo ship, break patrol to grapple ---- */
private func PatrolTick() {
	if (!patrolFx) return;  // idle if no patrol started
	var pCog;
	for (pCog in FindObjects(Find_ID(CGSH), Find_Exclude(this))) {
		if (Abs(GetX(pCog) - GetX()) < 150) {
			// Break patrol and grapple the cargo ship.
			RemoveEffect("FxPatrolLane", this);
			patrolFx = 0;
			SetCommand(this, "MoveTo", pCog, GetX(pCog), GetY(pCog));
			return;
		}
	}
}
