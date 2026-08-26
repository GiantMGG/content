/*-- Quartz (shallow crystal, CRYS-tier) --*/

#strict

protected func Hit() { Sound("Crystal*"); return 1; }

/* Feeds 1 magic energy per Apply cycle (spec: Quartz feeds 1) */
public func Apply(object obj)
{
	var used = false;
	var maxtimes = (GetCon() - 50) / 5;
	for (var i = 0; i < maxtimes; ++i)
	{
		if (DoMagicEnergy(1, obj))
		{
			used = true;
			if (GetCon() > 55) DoCon(-5); else RemoveObject();
		}
	}
	return used;
}

protected func CalcValue() { return (GetCon() - 50) / 10 + 2; }  // low tier

func IsAlchemContainer() { return true; }
func AlchemProcessTime() { return 150; }
