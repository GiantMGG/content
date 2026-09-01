/*-- ScorpionStagingRepro.c4s — cycle-90 symptom-5 staging probe —--*/
/* (spec §4.5): the WLFA hunt/attack/venom chain, driven            */
/* synchronously (DesertSmoke pattern). Expected v366: GREEN.       */
#strict 3

protected func Initialize()
{
	RunSmokeSteps();
	return true;
}

func RunSmokeSteps()
{
	/* Step 0: staging — 2 SCRP + 1 CLNK victim on flat ground. */
	var scorp = CreateObject(SCRP, 500, 100, NO_OWNER);
	var scorp2 = CreateObject(SCRP, 520, 100, NO_OWNER);
	var victim = CreateObject(CLNK, 515, 100, NO_OWNER);
	if (!scorp || !scorp2 || !victim)
		FatalError("ScorpionStagingRepro FAIL step 0: spawn failed");
	var hfx = GetEffect("WLF_HuntPrey", scorp);
	if (!hfx)
		FatalError("ScorpionStagingRepro FAIL step 0: SCRP missing WLF_HuntPrey");

	/* Step 1: hunt — one manual behaviour tick must issue MoveTo. */
	scorp->FxWLF_HuntPreyTimer(scorp, hfx, 0);
	if (!SEqual(GetCommand(scorp), "MoveTo"))
		FatalError("ScorpionStagingRepro FAIL step 1: hunt issued no MoveTo");

	/* Step 2: adjacent attack — the sting applies ScorpionVenom and
	   cuts the victim's Walk physical. */
	var walk0 = GetPhysical("Walk", 0, victim);
	scorp->WLFA_Attack(victim, 8);
	var vfx = GetEffect("ScorpionVenom", victim);
	if (!vfx)
		FatalError("ScorpionStagingRepro FAIL step 2: sting applied no venom");
	if (GetPhysical("Walk", 0, victim) >= walk0)
		FatalError("ScorpionStagingRepro FAIL step 2: venom did not cut Walk");

	/* Step 3: wear-off — drive the venom timer to expiry (returns
	   FX_Execute_Kill), then the stop callback restores Walk. */
	if (scorp->FxScorpionVenomTimer(victim, vfx, 175) != -1)
		FatalError("ScorpionStagingRepro FAIL step 3: venom timer did not expire");
	scorp->FxScorpionVenomStop(victim, vfx, 0, false);
	if (GetPhysical("Walk", 0, victim) != walk0)
		FatalError("ScorpionStagingRepro FAIL step 3: Walk not restored after venom wear-off");

	Log("ScorpionStagingRepro PASS");
	GameOver();
	return true;
}
