/* Gravity Well attractor - invisible anchor, removed by the effect */

#strict

protected func Initialize() {
	// Invisible; the effect handles visuals and removal
	SetClrModulation(RGBa(0,0,0,255));
}
