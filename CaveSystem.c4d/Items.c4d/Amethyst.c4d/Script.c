/*-- Amethyst (mid-depth crystal, CRYS-tier) --*/

#strict

protected func Hit() { Sound("Crystal*"); return 1; }

/* Feeds 2 magic energy per Apply cycle (spec: Amethyst feeds 2) */
public func Apply(object obj)
{
	var used = false;
	var maxtimes = (GetCon() - 50) / 5;
	for (var i = 0; i < maxtimes; ++i)
	{
		if (DoMagicEnergy(2, obj))
		{
			used = true;
			if (GetCon() > 55) DoCon(-5); else RemoveObject();
		}
	}
	return used;
}

protected func CalcValue() { return (GetCon() - 50) / 8 + 8; }  // mid tier

func IsAlchemContainer() { return true; }
func AlchemProcessTime() { return 150; }
