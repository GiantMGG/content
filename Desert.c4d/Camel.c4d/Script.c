/*-- Camel (CAML) -- passive desert herd animal. --*/

#strict 3

#include ANIM
#include WLFA

public func IsPossessible() { return true; }

protected func Initialize()
{
	// No predator behaviour -- the camel idles via ANIM Activity.
	WLFA_SetLootTable(this, [MEAT, 100]);
	return Birth();
}

protected func Activity()
{
	// Driven by ANIM idle wander. Possessed creatures skip AI.
	if (GetEffect("PossessionSpell", this)) return;
}

protected func Death()
{
	WLFA_DropLoot(this, WLFA_GetLootTable(this));
	RemoveObject();
	return true;
}

private func MaxAnimalCount() { return 4; }

/* ---- Salt Road caravan API (cycle 146) -------------------------------- */
/* Shared story machinery compiled into every Desert-loaded context:    */
/* acts and the story smoke orchestrate the same globals (StartSea-    */
/* Caravan precedent, CargoShipCaravan.c4d/Script.c:12). The puppet    */
/* effect re-issues SetCommand("MoveTo") every tick because the        */
/* camel's own ANIM idle-wander competes for the command slot; it      */
/* holds (skips the re-issue, resumes later) while SDRF's SandBuried  */
/* effect is on the camel, and dissolves itself on final arrival.     */

global func SaltRoad_StartCaravan(object camel, array waypoints)
{
	if (!camel) return false;
	if (!waypoints || GetLength(waypoints) < 1) return false;
	if (SaltRoad_IsCaravanActive(camel)) return true;
	var fx = AddEffect("SaltRoadCaravan", camel, 1, 1, camel);
	EffectVar(0, camel, fx) = waypoints;
	EffectVar(1, camel, fx) = 0;  // current waypoint index
	return true;
}

global func SaltRoad_StopCaravan(object camel)
{
	if (!camel) return false;
	if (GetEffect("SaltRoadCaravan", camel))
		RemoveEffect("SaltRoadCaravan", camel);
	return true;
}

global func SaltRoad_IsCaravanActive(object camel)
{
	if (!camel) return false;
	return !!GetEffect("SaltRoadCaravan", camel);
}

global func FxSaltRoadCaravanTimer(object camel, int fx, int time)
{
	// Hold while sand-buried (SDRF): skip the re-issue this tick;
	// the puppet resumes from the current waypoint once freed.
	if (GetEffect("SandBuried", camel)) return FX_OK;

	var waypoints = EffectVar(0, camel, fx);
	var idx = EffectVar(1, camel, fx);
	var target_x = waypoints[idx];

	// Waypoint reached (within 30 px)?
	if (Abs(GetX(camel) - target_x) < 30)
	{
		++idx;
		if (idx >= GetLength(waypoints))
		{
			// Final waypoint: report arrival to the scenario, then
			// dissolve the puppet (FX_Execute_Kill ends the effect).
			GameCall("SaltRoadCaravanArrived", camel);
			return FX_Execute_Kill;
		}
		EffectVar(1, camel, fx) = idx;
		target_x = waypoints[idx];
	}

	// Re-issue every tick: the camel's own ANIM idle-wander competes
	// for the command slot (cycle-146 spec, camel-puppet risk).
	SetCommand(camel, "MoveTo", nil, target_x, GetY(camel));
	return FX_OK;
}
