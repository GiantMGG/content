/*-- Obsidian (quenched-lava crafting material) --*/

#strict

protected func Hit() { Sound("Rock*"); return 1; }

/* High sell value (spec: high) */
protected func CalcValue() { return (GetCon() - 50) / 6 + 18; }

/* Used in advanced construction (future); alchem-container for future alchemy */
func IsAlchemContainer() { return true; }
func AlchemProcessTime() { return 200; }
