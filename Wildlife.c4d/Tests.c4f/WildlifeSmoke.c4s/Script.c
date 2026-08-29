/*-- WildlifeSmoke.c4s -- headless content integration test. --*/
/* Synchronous RunSmokeSteps() pattern (cf. EventSmoke.c4s).      */

#strict 3

/* Pull in the WLFA framework helpers (WLFA_StepBehaviours,              */
/* WLFA_GetLootTable) so the scenario script can drive them synchronously. */
#include WLFA

protected func Initialize()
{
	RunSmokeSteps();
	return true;
}

func RunSmokeSteps()
{
	/* Step 0: spawn the three creatures + a bear den. Assert counts. */
	var den = CreateObject(BDEN, 200, 100, NO_OWNER);
	if (!den) FatalError("WildlifeSmoke FAIL step 0: could not spawn BDEN");

	var wolf1 = CreateObject(WOLF, 100, 100, NO_OWNER);
	var wolf2 = CreateObject(WOLF, 110, 100, NO_OWNER);
	var wolf3 = CreateObject(WOLF, 120, 100, NO_OWNER);
	if (!wolf1 || !wolf2 || !wolf3)
		FatalError("WildlifeSmoke FAIL step 0: could not spawn wolf pack");
	if (ObjectCount(WOLF) < 3)
		FatalError(Format("WildlifeSmoke FAIL step 0: expected >=3 wolves, got %d", ObjectCount(WOLF)));

	var bear = CreateObject(WBRS, 210, 100, NO_OWNER);
	if (!bear) FatalError("WildlifeSmoke FAIL step 0: could not spawn WBRS");
	if (ObjectCount(BDEN) != 1)
		FatalError(Format("WildlifeSmoke FAIL step 0: expected 1 BDEN, got %d", ObjectCount(BDEN)));

	var spider = CreateObject(SPDR, 300, 100, NO_OWNER);
	if (!spider) FatalError("WildlifeSmoke FAIL step 0: could not spawn SPDR");

	/* Step 1: assert each wolf has the WLF_HuntPrey + WLF_PackFlank effects
	   and the loot table attached (WLFA ran in Initialize via CreateObject). */
	if (!GetEffect("WLF_HuntPrey", wolf1))
		FatalError("WildlifeSmoke FAIL step 1: wolf missing WLF_HuntPrey effect");
	if (!GetEffect("WLF_PackFlank", wolf1))
		FatalError("WildlifeSmoke FAIL step 1: wolf missing WLF_PackFlank effect");
	if (!WLFA_GetLootTable(wolf1))
		FatalError("WildlifeSmoke FAIL step 1: wolf missing loot table");
	// Marker: wolves are wildlife.
	if (!wolf1->~IsWildlife())
		FatalError("WildlifeSmoke FAIL step 1: wolf IsWildlife() returned false");

	/* Step 2: assert the bear has the WLF_Territorial effect. */
	if (!GetEffect("WLF_Territorial", bear))
		FatalError("WildlifeSmoke FAIL step 2: bear missing WLF_Territorial effect");

	/* Step 2a: bear territorial -- a Clonk intruder INSIDE the territory
	   radius triggers aggression (bear issues MoveTo toward the intruder).
	   This exercises F1's Find_Distance fix: the search must be centred on
	   the den, not offset by the bear's position. */
	DoEnergy(1000, bear); // ensure bear is above fleeHP so it aggros
	var intruder = CreateObject(CLNK, 205, 100, NO_OWNER);
	if (!intruder) FatalError("WildlifeSmoke FAIL step 2a: could not spawn intruder");
	SetCommand(bear, "None");
	WLFA_StepBehaviours(bear);
	if (!SEqual(GetCommand(bear), "MoveTo"))
		FatalError(Format("WildlifeSmoke FAIL step 2a: bear did not aggro intruder (cmd=%s)", GetCommand(bear)));
	RemoveObject(intruder);

	/* Step 2b: bear territorial -- a Clonk FAR outside the territory radius
	   does NOT trigger aggression. The bear is already within radius of the
	   den, so the else-branch issues no command and the bear stays idle. */
	var farClonk = CreateObject(CLNK, 900, 100, NO_OWNER);
	if (!farClonk) FatalError("WildlifeSmoke FAIL step 2b: could not spawn far clonk");
	SetCommand(bear, "None");
	WLFA_StepBehaviours(bear);
	if (SEqual(GetCommand(bear), "MoveTo"))
		FatalError(Format("WildlifeSmoke FAIL step 2b: bear aggroed far clonk (cmd=%s)", GetCommand(bear)));
	RemoveObject(farClonk);

	/* Step 3: assert the spider has the WLF_WebTrap effect. */
	if (!GetEffect("WLF_WebTrap", spider))
		FatalError("WildlifeSmoke FAIL step 3: spider missing WLF_WebTrap effect");

	/* Step 4: drive one AI step on a wolf, assert it issued a command. */
	WLFA_StepBehaviours(wolf1);
	// After stepping, the wolf should have a MoveTo command toward prey (or
	// none if no prey in range -- we accept either, but the call must not crash).
	// Place a WIPF prey so the hunt has a target.
	var prey = CreateObject(WIPF, 150, 100, NO_OWNER);
	if (!prey) FatalError("WildlifeSmoke FAIL step 4: could not spawn WIPF prey");
	WLFA_StepBehaviours(wolf1);
	if (SEqual(GetCommand(wolf1), "None"))
		FatalError("WildlifeSmoke FAIL step 4: wolf did not issue a hunt command");

	/* Step 5: spider lays a web via the behaviour step; assert WEBB exists.
	   Stepping multiple times must NOT stack webs on the same tile -- this
	   exercises F3's Find_AtPoint(0,0) fix (the "don't stack webs" guard). */
	WLFA_StepBehaviours(spider);
	// The web is only laid if prey is near; spawn prey near the spider.
	var prey2 = CreateObject(WIPF, 300, 100, NO_OWNER);
	if (!prey2) FatalError("WildlifeSmoke FAIL step 5: could not spawn WIPF prey");
	WLFA_StepBehaviours(spider);
	if (ObjectCount(WEBB) < 1)
		FatalError(Format("WildlifeSmoke FAIL step 5: expected >=1 WEBB, got %d", ObjectCount(WEBB)));
	// Step two more times; the "don't stack webs" guard must prevent extras.
	WLFA_StepBehaviours(spider);
	WLFA_StepBehaviours(spider);
	if (ObjectCount(WEBB) != 1)
		FatalError(Format("WildlifeSmoke FAIL step 5: expected exactly 1 WEBB (no stacking), got %d", ObjectCount(WEBB)));

	/* Step 6: a Clonk contacting a web gains the Webbed slow. */
	var clonk = CreateObject(CLNK, 300, 100, NO_OWNER);
	if (!clonk) FatalError("WildlifeSmoke FAIL step 6: could not spawn CLNK");
	// Move the clonk onto a web and fire the web's WebTick timer.
	var web = FindObject2(Find_ID(WEBB));
	if (!web) FatalError("WildlifeSmoke FAIL step 6: no WEBB to test");
	SetPosition(GetX(web), GetY(web), clonk);
	web->WebTick();
	if (!GetEffect("Webbed", clonk))
		FatalError("WildlifeSmoke FAIL step 6: clonk did not gain Webbed effect");

	/* Step 7: kill the alpha wolf; assert survivors scatter. */
	// First, force alpha election by stepping pack flank on all wolves.
	WLFA_StepBehaviours(wolf1);
	WLFA_StepBehaviours(wolf2);
	WLFA_StepBehaviours(wolf3);
	// Identify the elected alpha (the wolf PackFlank marked with WLFA_IsAlpha)
	// and kill it. HP-based identification would be ambiguous when HP are tied
	// and FindObjects returns members in an order that doesn't match creation
	// order, so use the marker effect instead.
	var alpha = 0;
	if (GetEffect("WLFA_IsAlpha", wolf1)) alpha = wolf1;
	else if (GetEffect("WLFA_IsAlpha", wolf2)) alpha = wolf2;
	else if (GetEffect("WLFA_IsAlpha", wolf3)) alpha = wolf3;
	if (!alpha) FatalError("WildlifeSmoke FAIL step 7: no alpha elected");
	Kill(alpha);
	// Scatter is triggered from the alpha's Destruction() -> WLFA_ScatterPack.
	// Assert at least one survivor has the WLF_Scatter effect.
	var survivors = [wolf1, wolf2, wolf3];
	var scattered = false;
	var s;
	for (var s in survivors)
	{
		if (s && s != alpha && GetEffect("WLF_Scatter", s))
		{
			scattered = true;
			break;
		}
	}
	if (!scattered)
		FatalError("WildlifeSmoke FAIL step 7: no surviving wolf gained WLF_Scatter");

	/* Step 8: loot drop -- kill a wolf and assert its corpse contains WPLT. */
	var wolfLoot = CreateObject(WOLF, 400, 100, NO_OWNER);
	if (!wolfLoot) FatalError("WildlifeSmoke FAIL step 8: could not spawn loot-test wolf");
	Kill(wolfLoot);
	// After Death(), the wolf ChangeDef'd to DWLF. ChangeDef preserves the
	// object reference, so wolfLoot is now the DWLF corpse. (We can't rely on
	// Find_AtPoint at the spawn coords -- the corpse shifts when its action
	// changes to "Dead" on death.)
	if (GetID(wolfLoot) != DWLF)
		FatalError("WildlifeSmoke FAIL step 8: wolf did not ChangeDef to DWLF");
	if (!FindObject2(Find_ID(WPLT), Find_Container(wolfLoot)))
		FatalError("WildlifeSmoke FAIL step 8: corpse missing WPLT loot");

	Log("WildlifeSmoke PASS");
	GameOver();
	return true;
}
