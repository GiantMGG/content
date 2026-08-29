/*-- WLFA -- shared wildlife include. --*/
/* Composable behaviour effects + DropLoot helper.                */
/* Mirrors the ANIM include pattern: creatures do                 */
/*   #include ANIM                                                */
/*   #include WLFA                                                */
/* and register behaviours via WLFA_AddBehavior(this, name, opts).*/

#strict 3

/* ===== Marker ===== */

public func IsWildlife() { return true; }

/* ===== Behaviour registration ===== */

// Register a behaviour effect on a creature.
//   name  -- behaviour suffix, e.g. "HuntPrey"  -> effect "WLF_HuntPrey"
//   opts  -- proplist of behaviour options (Range, PackID, Interval, ...)
// Returns the effect number, or 0 on failure.
public func WLFA_AddBehavior(object creature, string name, proplist opts)
{
	if (!creature) return 0;
	var interval = 35;
	if (opts && opts.Interval) interval = opts.Interval;
	var fx = AddEffect(Format("WLF_%s", name), creature, 1, interval, creature);
	if (fx && opts)
	{
		EffectVar(0, creature, fx) = opts;
	}
	return fx;
}

// Synchronous "step every behaviour once" entry point used by smoke tests.
// The engine fires FxWLF_*Timer callbacks automatically during the normal
// game loop (each AddEffect has its own interval). But the smoke harness
// runs assertions synchronously at tick 0 before any timer fires, so we
// expose this to force one logical AI step deterministically.
//
// We iterate the known behaviour names, check if each effect exists on the
// creature, and invoke its *Timer callback via creature->Call(cb, ...).
// Since the creature #includes WLFA, the Fx*Timer functions are in the
// creature's script scope and Call resolves them.
public func WLFA_StepBehaviours(object creature)
{
	if (!creature) return;
	var names = ["WLF_HuntPrey", "WLF_PackFlank", "WLF_Territorial",
	             "WLF_WebTrap", "WLF_NestSpawner", "WLF_RetreatLeash"];
	var n;
	for (var n in names)
	{
		var fx = GetEffect(n, creature);
		if (!fx) continue;
		var cb = Format("Fx%sTimer", n);
		creature->Call(cb, creature, fx, 0);
	}
}

/* ===== Loot table ===== */

// Attach a loot table to a creature. table = { items: [id, chance01, ...] }.
public func WLFA_SetLootTable(object creature, proplist table)
{
	if (!creature) return;
	var fx = AddEffect("WLFA_Loot", creature, 1, 0, creature);
	if (fx) EffectVar(0, creature, fx) = table;
}

public func WLFA_GetLootTable(object creature)
{
	if (!creature) return 0;
	var fx = GetEffect("WLFA_Loot", creature);
	if (!fx) return 0;
	return EffectVar(0, creature, fx);
}

// Roll the loot table on a corpse: for each (id, chance) pair, if
// Random(100) < chance, CreateContents(id, corpse). Called from Death()
// BEFORE ChangeDef(DeadID).
public func WLFA_DropLoot(object corpse, proplist table)
{
	if (!corpse || !table || !table.items) return;
	var items = table.items;
	var i = 0;
	while (i < GetLength(items))
	{
		var idDrop = items[i];
		var chance = items[i + 1];
		if (!chance) chance = 100;
		if (Random(100) < chance)
			CreateContents(idDrop, corpse);
		i += 2;
	}
}

/* ===== Shared predator primitives (reused from Shark/Bear patterns) ===== */

// Find nearest live prey within range (Shark.c4d:87-102 pattern).
public func WLFA_FindPrey(object creature, int range)
{
	if (!creature) return 0;
	if (!range) range = 500;
	var prey = FindObject2(Find_OCF(OCF_Prey), Find_OCF(OCF_Alive),
	                       Find_Distance(range), Find_NoContainer(),
	                       Find_Layer(GetObjectLayer()), Find_Exclude(creature));
	return prey;
}

// Bite a prey adjacent to the creature. Returns true if an attack landed.
// IntAttackDelay (70 ticks) gates re-attack (Shark.c4d:161-176 pattern).
public func WLFA_Attack(object creature, object prey, int damage)
{
	if (!creature || !prey) return false;
	if (GetEffect("IntAttackDelay", creature)) return false;
	if (!damage) damage = 8;
	Punch(prey, damage);
	Fling(prey, -1 + 2 * GetDir(creature), -2);
	Sound("Munch1");
	AddEffect("IntAttackDelay", creature, 1, 70, creature);
	return true;
}

// Auto-expire the attack delay after one interval (70 ticks).
func FxIntAttackDelayTimer() { return -1; }

/* ===== Behaviour: WLF_HuntPrey ===== */
/* opts: { PreyOCF, Range, Interval } */
/* Each tick: find prey, SetCommand MoveTo it; if adjacent, Attack.       */

func FxWLF_HuntPreyTimer(object target, int effect, int time)
{
	var opts = EffectVar(0, target, effect);
	if (!opts) return;
	var range = opts.Range;
	if (!range) range = 500;
	var prey = WLFA_FindPrey(target, range);
	if (!prey) return;
	var iFromSide = +1; if (GetX(prey) < GetX(target)) iFromSide = -1;
	SetCommand(target, "MoveTo", 0, GetX(prey) + 15 * iFromSide, GetY(prey));
	// Adjacent attack
	if (ObjectDistance(prey, target) <= 12)
		WLFA_Attack(target, prey, 8);
}

/* ===== Behaviour: WLF_PackFlank ===== */
/* opts: { PackID, AlphaHP, Interval } */
/* Elects the highest-HP member as alpha (marks it with WLFA_IsAlpha effect).*/
/* Non-alphas fan out to GetX(prey)+/-21. On alpha death, survivors gain     */
/* AddEffect("WLF_Scatter", ...) and flee to a random map edge.             */

func FxWLF_PackFlankStart(object target, int effect, bool temp)
{
	if (temp) return;
	// Defer alpha election to the first timer tick so the pack exists.
}

func FxWLF_PackFlankTimer(object target, int effect, int time)
{
	var opts = EffectVar(0, target, effect);
	if (!opts) return;
	var packID = opts.PackID;
	if (!packID) return;

	// Elect alpha: highest-HP live member of the pack.
	var members = FindObjects(Find_ID(packID), Find_NoContainer());
	var alpha = 0;
	var alphaHP = -1;
	var m;
	for (var m in members)
	{
		if (!GetAlive(m)) continue;
		var hp = GetEnergy(m);
		if (hp > alphaHP) { alphaHP = hp; alpha = m; }
	}
	if (!alpha) return;

	// Mark alpha with an effect (idempotent).
	if (!GetEffect("WLFA_IsAlpha", alpha))
		AddEffect("WLFA_IsAlpha", alpha, 1, 0, alpha);

	// Non-alphas flank toward prey.
	if (target != alpha)
	{
		var prey = WLFA_FindPrey(target, 500);
		if (prey)
		{
			var iFromSide = +1; if (GetX(prey) < GetX(target)) iFromSide = -1;
			SetCommand(target, "MoveTo", 0, GetX(prey) + 21 * iFromSide, GetY(prey));
		}
	}
}

// Called when the alpha dies: scatter survivors. Exposed for smoke tests.
public func WLFA_ScatterPack(id packID)
{
	var members = FindObjects(Find_ID(packID), Find_NoContainer());
	// Elect alpha (highest HP) so we don't scatter the new leader.
	var alpha = 0;
	var alphaHP = -1;
	var m;
	for (var m in members)
	{
		if (!GetAlive(m)) continue;
		var hp = GetEnergy(m);
		if (hp > alphaHP) { alphaHP = hp; alpha = m; }
	}
	if (!alpha) return;
	// Scatter all live non-alpha members.
	for (var m in members)
	{
		if (!GetAlive(m)) continue;
		if (m == alpha) continue;
		if (!GetEffect("WLF_Scatter", m))
			AddEffect("WLF_Scatter", m, 1, 70, m);
	}
}

func FxWLF_ScatterTimer(object target, int effect, int time)
{
	SetCommand(target, "None");
	// Flee to a random map edge.
	var edgeX = 0;
	if (Random(2)) edgeX = 20;
	else edgeX = LandscapeWidth() - 20;
	SetCommand(target, "MoveTo", 0, edgeX, GetY(target));
	// After 350 ticks (5s), remove if still scattered.
	if (time >= 350)
	{
		RemoveObject(target);
		return -1;
	}
}

/* ===== Behaviour: WLF_Territorial ===== */
/* opts: { DenID, TerritoryRadius, FleeHP, HealHP, Interval } */
/* Each tick: if a CrewMember is within TerritoryRadius of the den, growl  */
/* then attack. If GetEnergy < FleeHP, flee to den and heal.               */

func FxWLF_TerritorialTimer(object target, int effect, int time)
{
	var opts = EffectVar(0, target, effect);
	if (!opts) return;
	var denID = opts.DenID;
	var radius = opts.TerritoryRadius;
	if (!radius) radius = 350;
	var fleeHP = opts.FleeHP;
	if (!fleeHP) fleeHP = 30;
	var healHP = opts.HealHP;
	if (!healHP) healHP = 60;

	var den = FindObject2(Find_ID(denID));
	if (!den) return; // phantom leash: bear keeps last coords (simplification: go neutral)

	// Flee + heal when low.
	if (GetEnergy(target) < fleeHP)
	{
		SetCommand(target, "MoveTo", 0, GetX(den), GetY(den));
		if (!GetEffect("WLF_Healing", target))
			AddEffect("WLF_Healing", target, 1, 35, target);
		return;
	}
	// Stop healing once above healHP.
	if (GetEnergy(target) >= healHP)
		RemoveEffect("WLF_Healing", target);

	// Aggression: crew member within territory radius of the den.
	var intruder = FindObject2(Find_OCF(OCF_CrewMember),
		Find_AtRect(GetX(den) - radius, GetY(den) - radius, radius * 2, radius * 2));
	if (intruder)
	{
		SetAction(target, "Run");
		SetCommand(target, "MoveTo", intruder);
		if (ObjectDistance(intruder, target) <= 12)
			WLFA_Attack(target, intruder, 12);
	}
	else
	{
		// Idle near den.
		if (ObjectDistance(den, target) > radius)
			SetCommand(target, "MoveTo", 0, GetX(den), GetY(den));
	}
}

func FxWLF_HealingTimer(object target, int effect, int time)
{
	if (GetEnergy(target) >= 100) return -1;
	DoEnergy(1, target);
}

/* ===== Behaviour: WLF_WebTrap ===== */
/* opts: { WebID, Interval } */
/* Each tick (slow cadence): if near a wall/floor and no web already placed  */
/* at this spot, CreateObject(WebID) at the creature's position.             */

func FxWLF_WebTrapTimer(object target, int effect, int time)
{
	var opts = EffectVar(0, target, effect);
	if (!opts) return;
	var webID = opts.WebID;
	if (!webID) return;
	// Only lay a web every 5 ticks of the behaviour (throttle).
	if (time % 5 != 0) return;
	// Don't stack webs on the same tile.
	if (FindObject2(Find_ID(webID), Find_AtPoint(GetX(target), GetY(target)))) return;
	// Prefer webbed prey; fall back to FindPrey.
	var prey = FindObject2(Find_OCF(OCF_Prey), Find_Func("IsWebbed"), Find_Distance(200));
	if (!prey) prey = WLFA_FindPrey(target, 300);
	if (prey)
	{
		var web = CreateObject(webID, GetX(target), GetY(target), NO_OWNER);
		if (web) web->SetLocal(0, target); // local 0 = owner spider
	}
}

/* ===== Behaviour: WLF_NestSpawner ===== */
/* opts: { SpawnID, Cap, Interval } */
/* Each tick: if the nest has stored webbed prey and ObjectCount(SpawnID)   */
/* < Cap, CreateObject(SpawnID) as a spiderling.                            */

func FxWLF_NestSpawnerTimer(object target, int effect, int time)
{
	var opts = EffectVar(0, target, effect);
	if (!opts) return;
	var spawnID = opts.SpawnID;
	var cap = opts.Cap;
	if (!spawnID) return;
	if (!cap) cap = 6;
	// Count stored webbed prey near the nest.
	var stored = ObjectCount2(Find_OCF(OCF_Prey), Find_Func("IsWebbed"),
	                          Find_Distance(150));
	if (stored <= 0) return;
	if (ObjectCount(spawnID) >= cap) return;
	var spiderling = CreateObject(spawnID, GetX(target), GetY(target) - 10, NO_OWNER);
	if (spiderling) spiderling->~Birth();
}

/* ===== Behaviour: WLF_RetreatLeash ===== */
/* opts: { EdgeFailsafe, Interval } */
/* Each tick: walk to the nearest map edge; RemoveObject once within 20px   */
/* of the edge or after EdgeFailsafe ticks (default 700).                   */

func FxWLF_RetreatLeashTimer(object target, int effect, int time)
{
	var opts = EffectVar(0, target, effect);
	var failsafe = 700;
	if (opts && opts.EdgeFailsafe) failsafe = opts.EdgeFailsafe;
	var edgeX = 0;
	if (GetX(target) < LandscapeWidth() / 2) edgeX = 20;
	else edgeX = LandscapeWidth() - 20;
	SetCommand(target, "MoveTo", 0, edgeX, GetY(target));
	if (Abs(GetX(target) - edgeX) <= 20 || time >= failsafe)
	{
		RemoveObject(target);
		return -1;
	}
}

/* ===== Webbed effect (applied by WEBB via its WebTick timer) ===== */
/* FxWebbedStart lowers Walk physical to ~20% (11000).              */
/* FxWebbedStop restores the prior physical.                        */
/* IsWebbed() is the marker predators check (must be global so      */
/* Find_Func("IsWebbed") works on any object, including Clonks).    */

global func IsWebbed() { return GetEffect("Webbed", this) != 0; }

// Apply the Webbed slow to a clonk/prey. The host (5th AddEffect arg)
// must be an object whose script includes WLFA (so FxWebbedStart/Stop
// resolve). The web object (which #includes WLFA) is the natural host.
public func WLFA_ApplyWebbed(object clonk, object host)
{
	if (!clonk) return;
	if (GetEffect("Webbed", clonk)) return; // idempotent
	if (!host) host = clonk;
	AddEffect("Webbed", clonk, 1, 1, host);
}

func FxWebbedStart(object target, int effect, bool temp)
{
	if (temp) return;
	SetPhysical("Walk", 11000, 2, target); // ~20% of normal 55000
}

func FxWebbedStop(object target, int effect, int reason, bool temp)
{
	if (temp) return;
	SetPhysical("Walk", 55000, 2, target); // restore
}
