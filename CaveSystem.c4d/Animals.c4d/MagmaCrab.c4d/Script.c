/*-- Magma Crab --*/

#strict
#include ANIM

public func IsAnimal()    { return 1; }
public func IsHeatImmune(){ return 1; }  // immune to HeatAura (spec edge case)

private func ReproductionAreaSize() { return 600; }
private func ReproductionRate()     { return 3000; }
private func MaxAnimalCount()       { return 6; }

protected func Initialize()
{
	SetAction("Walk");
	Birth();
}

/* ANIM TimerCall override: seek DuroLava, attack on contact, take damage in Water */
protected func Activity()
{
	if (GetEffect("PossessionSpell", this())) return;
	if (Contained()) return;

	// Immersed in Water: take damage + steam
	if (GBackLiquid(0, 0) && GetMaterial(0, 0) == Material("Water"))
	{
		DoEnergy(-2);
		CastParticles("Steam", 3, 20, 0, 0, 20, 30, RGB(200, 200, 220), RGB(160, 160, 180));
		if (!GetAlive()) return;
	}

	// Seek DuroLava downward via material scan when not in combat
	if (!GetCommand() && GetAction() eq "Walk")
	{
		if (GetMaterial(0, 4) == Material("DuroLava"))
		{
			// Standing on/near lava: hold position
			SetComDir(COMD_Stop);
		}
		else if (!Random(20))
		{
			// Scan a small grid for the nearest DuroLava pixel
			var best_x = 0, best_y = 0, best_dist = 9999;
			var dx, dy;
			for (dy = -40; dy <= 40; dy += 10)
				for (dx = -40; dx <= 40; dx += 10)
				{
					if (GetMaterial(dx, dy) == Material("DuroLava"))
					{
						var d = dx*dx + dy*dy;
						if (d < best_dist) { best_dist = d; best_x = dx; best_y = dy; }
					}
				}
			if (best_dist < 9999)
				SetComDir(best_x < 0 ? COMD_Left : COMD_Right);
		}
	}

	// Predator: Punch any living thing in contact range
	var prey = FindObject(0, -20, -20, 40, 40, OCF_Alive, 0, 0, NoContainer());
	if (prey && prey != this() && GetCategory(prey) & C4D_Living)
	{
		SetAction("Attack");
		Punch(prey, 8);
	}
}

protected func HitCheck()
{
	if (GetEffect("PossessionSpell", this())) return;
}

protected func Death()
{
	ChangeDef(DBAT);  // generic corpse (no dedicated MGCR corpse in scope)
	return 1;
}
