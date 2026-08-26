/*-- Extended Glint sparkle table for CaveSystem crystals --*/

#strict 2

#appendto EGLN

/*
 * Extend the base SetMatAccessTable so Quartz/Amethyst/Selenite/Obsidian
 * sparkle alongside the base Gold/Crystal/Ice. Slot numbers 4..7 must not
 * collide with base slots 1..3; the Twinkle() switch is extended below.
 */
private func SetMatAccessTable()
{
	inherited();
	Local(Material("Quartz"))   = 4;
	Local(Material("Amethyst")) = 5;
	Local(Material("Selenite")) = 6;
	Local(Material("Obsidian")) = 7;
}

/*
 * Extend the base Twinkle() sparkle colour switch. We re-implement only the
 * new cases by hooking via the base Local(m) lookup; base cases (1=Gold,
 * 2=Crystal, 3=Ice) are untouched.
 */
private func Twinkle()
{
	inherited();
	// Scan for the new cave materials (slots 4-7) and sparkle them.
	var x, y, m;
	for (var i = 0; i < sparkle_amount; i++)
		if (Local(m = GetMaterial(x = Random(LandscapeWidth()), y = Random(LandscapeHeight()))))
			if (Local(m) >= 4)
				SparkleNew(Local(m), x, y);
}

private func SparkleNew(int m, int x, int y)
{
	if (m == 4)      Sparkle(x, y, 220, 220, 240, 1);        // Quartz  - cool white
	else if (m == 5) Sparkle(x, y, 180,  80, 200, 1);        // Amethyst - purple
	else if (m == 6) Sparkle(x, y, 255, 240, 180, 1);        // Selenite - warm gold
	else if (m == 7) Sparkle(x, y,  40,  20,  30, 1);        // Obsidian - dark glass
}
