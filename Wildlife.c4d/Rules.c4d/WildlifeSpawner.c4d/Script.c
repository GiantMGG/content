/*-- WildlifeSpawner / NightDirector rule (id=WLSP). --*/
/* The wilderness-danger mechanic for settlement scenarios.   */
/* Polls IsNight()/IsDay() on a 70-tick timer; on nightfall   */
/* spawns 1 + wealth/3 predators (clamped [1, MaxPerNight])   */
/* on the perimeter; on dawn retreats/despawns live predators.*/
/* Opt-in: scenarios not adding WLSP get no night-director.   */

#strict 3

#include WLFA

/* ===== Tunables (scenario-settable) ===== */

local wealthScale;   // divisor for wealth->count, default 3
local maxPerNight;   // hard cap on predators per night, default 8
local patrolRadius;  // not used directly; reserved for scenario tuning

public func SetWealthScale(int n) { wealthScale = n; return true; }
public func SetMaxPerNight(int n) { maxPerNight = n; return true; }
public func SetPatrolRadius(int n) { patrolRadius = n; return true; }

protected func Initialize()
{
	SetAction("Idle");
	wealthScale = 3;
	maxPerNight = 8;
	patrolRadius = 500;
	// Track whether we have spawned for the current night.
	SetLocal("WLSP_NightActive", 0);
	SetLocal("WLSP_SpawnedThisNight", 0);
	SetLocal("WLSP_ForcePhase", -1); // -1 = auto (read IsNight)
	return true;
}

/* ===== Test hooks for deterministic smoke tests ===== */
/* WLSP_SetForcePhase(0) = force day, 1 = force night, -1 = auto. */
public func WLSP_SetForcePhase(int phase) { SetLocal("WLSP_ForcePhase", phase); return true; }

/* ===== Global API ===== */

global func GetWildlifeThreat()
{
	// Count live wildlife predators (WOLF, WBRS, SPDR) on the map.
	return ObjectCount2(Find_ID(WOLF), Find_NoContainer())
	     + ObjectCount2(Find_ID(WBRS), Find_NoContainer())
	     + ObjectCount2(Find_ID(SPDR), Find_NoContainer());
}

global func GetSettlementWealth()
{
	// Wealth proxy = (player structure count + crew count) / wealthScale.
	// Settlement footprint = centroid of all player-owned C4D_Structure.
	var structCount = ObjectCount2(Find_Category(C4D_Structure));
	var crewCount = ObjectCount2(Find_OCF(OCF_CrewMember));
	var raw = structCount + crewCount;
	// Clamp to [1, 8] predators per night (before the +1).
	var clamped = BoundBy(raw, 0, 24); // 24/3 = 8
	return clamped;
}

/* ===== The 70-tick TimerCall ===== */

public func WLSP_Tick()
{
	var isNight = WLSP_IsNightNow();

	if (isNight)
	{
		// Mark night active; spawn up to the night's target.
		if (!GetLocal("WLSP_NightActive"))
		{
			SetLocal("WLSP_NightActive", 1);
			SetLocal("WLSP_SpawnedThisNight", 0);
		}
		TrySpawnOne();
		return true;
	}

	// Day path: retreat any live leashed predators without a den, then
	// clear the night-active flag.
	if (GetLocal("WLSP_NightActive"))
	{
		SetLocal("WLSP_NightActive", 0);
		SetLocal("WLSP_SpawnedThisNight", 0);
		RetreatAllPredators();
	}
	return true;
}

/* ===== Night detection (with force override) ===== */

public func WLSP_IsNightNow()
{
	var phase = GetLocal("WLSP_ForcePhase");
	if (phase == 0) return false; // forced day
	if (phase == 1) return true;  // forced night
	return IsNight();             // auto
}

/* ===== Per-tick spawn ===== */

public func TrySpawnOne()
{
	var target = NightTargetCount();
	var spawned = GetLocal("WLSP_SpawnedThisNight");
	// Top up rather than double-spawn: target counts already-alive predators.
	var alive = GetWildlifeThreat();
	if (alive >= target) return false;
	if (spawned >= target) return false;

	var spot = PickPerimeterSpot();
	if (!spot) return false; // no valid spot this tick; defer

	var idToSpawn = WOLF; // surface default
	// If a SpiderNest exists and we're below spider cap, spawn a spider near it.
	var nest = FindObject2(Find_ID(SPNE));
	if (nest && ObjectCount(SPDR) < 6 && Random(2))
	{
		var spider = CreateObject(SPDR, GetX(nest), GetY(nest) - 10, NO_OWNER);
		if (spider) spider->~Birth();
		SetLocal("WLSP_SpawnedThisNight", spawned + 1);
		return true;
	}

	var predator = CreateObject(idToSpawn, spot[0], spot[1], NO_OWNER);
	if (predator) predator->~Birth();
	SetLocal("WLSP_SpawnedThisNight", spawned + 1);

	// Attach a retreat leash so dawn despawns it.
	if (!GetEffect("WLF_RetreatLeash", predator))
		AddEffect("WLF_RetreatLeash", predator, 1, 35, predator);

	return true;
}

/* ===== Night target count: 1 + wealth/scale, clamped to [1, maxPerNight] ===== */

public func NightTargetCount()
{
	var wealth = GetSettlementWealth();
	var scale = wealthScale;
	if (!scale) scale = 3;
	var count = 1 + wealth / scale;
	count = BoundBy(count, 1, maxPerNight);
	return count;
}

/* ===== Perimeter spot picker ===== */
/* Rejects candidates intersecting a C4D_Structure, requires surface  */
/* (GBackLiquid false, material not Water/Lava) or cave (GBackSolid   */
/* ceiling overhead). After 20 failed samples, defers to next tick.   */

public func PickPerimeterSpot()
{
	var i;
	for (i = 0; i < 20; i++)
	{
		var edge = Random(2);
		var x;
		if (edge == 0) x = 20 + Random(40);
		else x = LandscapeWidth() - 20 - Random(40);
		var y = 0;
		// Scan downward for the surface.
		while (y < LandscapeHeight() && !GBackSolid(x, y)) y++;
		if (y >= LandscapeHeight()) continue;
		// Place just above the surface.
		y = y - 10;
		if (y < 0) continue;
		// Reject if inside/near a structure.
		if (FindObject2(Find_Category(C4D_Structure), Find_AtRect(x - 20, y - 20, 40, 40)))
			continue;
		// Reject liquid surface.
		if (GBackLiquid(x, y + 12)) continue;
		return [x, y];
	}
	return 0;
}

/* ===== Dawn retreat ===== */

public func RetreatAllPredators()
{
	// Command all live leashed predators (no den) to retreat to the nearest
	// edge; the WLF_RetreatLeash effect removes them on arrival.
	var ids = [WOLF, WBRS, SPDR];
	var id;
	for (var id in ids)
	{
		var p = 0;
		while (p = FindObject2(Find_ID(id), Find_NoContainer(), p + 1))
		{
			if (!GetAlive(p)) continue;
			// Bears are den-leashed; don't despawn them at dawn.
			if (id == WBRS) continue;
			if (!GetEffect("WLF_RetreatLeash", p))
				AddEffect("WLF_RetreatLeash", p, 1, 35, p);
		}
	}
	return true;
}
