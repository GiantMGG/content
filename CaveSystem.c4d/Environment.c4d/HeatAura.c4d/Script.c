/*-- Heat Aura: invisible overlay draining energy in the lava stratum --*/

#strict

// Tunable: damage per tick applied to non-immune, non-Water-immersed Clonks
// in the lava stratum. Spec: tune to -3 per tick.
static const HEATAURA_DamagePerTick = 3;
static const HEATAURA_Stratum3_Y   = 300;  // lava stratum top boundary (px)

protected func Initialize()
{
	// Invisible, no interaction
	SetClrModulation(RGBa(0, 0, 0, 0));
}

/* TimerCall: drain energy from eligible Clonks in the lava stratum */
protected func HeatTick()
{
	// Find all living crew in the lava stratum
	for (var clonk in FindObjects(Find_Category(C4D_Living), Find_OCF(OCF_Alive)))
	{
		// Only the lava stratum
		if (GetY(clonk) < HEATAURA_Stratum3_Y) continue;
		// Immune? (e.g. MagmaCrab exposes IsHeatImmune())
		if (clonk->~IsHeatImmune()) continue;
		// In Water? (stepping into Water halts the drain)
		if (GetMaterial(clonk->GetX() - GetX(), clonk->GetY() - GetY()) == Material("Water")) continue;
		// Apply heat damage
		DoEnergy(-HEATAURA_DamagePerTick, clonk);
		// Optional: visual heat shimmer particle on the victim
		if (!Random(8))
			CastParticles("Spark", 1, 10, GetX(clonk) - GetX(), GetY(clonk) - GetY(), 20, 20,
				RGB(255, 120, 40), RGB(200, 60, 20));
	}
}
