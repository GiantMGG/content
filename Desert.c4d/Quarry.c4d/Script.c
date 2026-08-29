/*-- Quarry (QRRY) -- Sandstone extraction structure. --*/

#strict

local vein_exhausted_logged;

public func Construction()
{
	vein_exhausted_logged = false;
	return 1;
}

/* TimerCall: probe 4px below centre; if Sandstone, extract and
   produce a SNDS block. Returns 1 on production, 0 otherwise. */
public func Quarry()
{
	var sandstone = Material("Sandstone");
	if (GetMaterial(0, 4) != sandstone)
	{
		if (!vein_exhausted_logged)
		{
			Log("The quarry's sandstone vein is exhausted.");
			vein_exhausted_logged = true;
		}
		return 0;
	}
	var extracted = ExtractMaterialAmount(0, 4, sandstone, 2);
	if (extracted <= 0) return 0;
	var block = CreateContents(SNDS);
	if (!block) return 0;
	return 1;
}
