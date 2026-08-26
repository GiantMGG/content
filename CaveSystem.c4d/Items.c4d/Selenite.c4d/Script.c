/*-- Selenite (deep crystal, CRYS-tier, highest value) --*/

#strict

protected func Hit() { Sound("Crystal*"); return 1; }

/* Feeds 3 magic energy per Apply cycle (spec: Selenite feeds 3) */
public func Apply(object obj)
{
	var used = false;
	var maxtimes = (GetCon() - 50) / 5;
	for (var i = 0; i < maxtimes; ++i)
	{
		if (DoMagicEnergy(3, obj))
		{
			used = true;
			if (GetCon() > 55) DoCon(-5); else RemoveObject();
		}
	}
	return used;
}

protected func CalcValue() { return (GetCon() - 50) / 6 + 25; }  // high tier

func IsAlchemContainer() { return true; }
func AlchemProcessTime() { return 150; }
