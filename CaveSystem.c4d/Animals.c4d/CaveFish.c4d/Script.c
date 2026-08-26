/*-- Cave Fish --*/

#strict
#include FISH

local lure;  // attached bioluminescent glow overlay

public func IsPossessible() { return 1; }

protected func Initialize()
{
	Birth();
	// Spawn the attached bioluminescent glow overlay (Anglerfish-lure pattern)
	lure = AttachGlowOverlay(this(), RGB(80, 140, 220), 30);
}

/* FISH TimerCall override: schooling + flee-from-light */
protected func Activity()
{
	if (GetEffect("PossessionSpell", this())) return;
	if (Contained()) return;

	// Schooling: match ComDir of nearby CaveFish within 60 px
	var schoolmate = FindObject(CVFS, -60, -60, 120, 120, OCF_Alive, 0, 0, NoContainer());
	if (schoolmate && schoolmate != this() && !Random(10))
		SetComDir(GetComDir(schoolmate));

	// Flee from light: sample darkest pixel within 60 px, swim toward it
	if (!Random(15) && InLiquid())
	{
		var sample = SampleDarkestPixel(GetX(), GetY());
		if (sample[2] > 600)  // dark enough to be worth fleeing toward
			SetComDir(sample[0] < GetX() ? COMD_Left : COMD_Right);
	}
}

protected func Death()
{
	if (lure) RemoveObject(lure);
	ChangeDef(DBAT);  // reuse generic corpse pattern; no dedicated CVFS corpse in scope
	return 1;
}
