/*-- CascadeDirector (CDIR) -- shared cascade state machine.      --*/
/*                                                                  */
/* Paints the canonical "Feuerstaudamm" apparatus (PaintCascade)    */
/* and polls the five cascade beats every 35 ticks, narrating each  */
/* exactly once via Log:                                            */
/*                                                                  */
/*   1. breach  - reservoir Lava+Ashes count drops under 95% of     */
/*               the paint baseline (lava cools to Ashes in place,  */
/*               so only real drainage trips the beat)              */
/*   2. FLAM    - a FLAM object appears in the oil-pan ROI          */
/*   3. rock    - rock delta in the sea band >= 33 px               */
/*   4. rain    - water deposits in the sluice gap between the      */
/*                sea surface and the tank wall >= 10 px            */
/*   5. burial  - any walker carries the SandBuried effect          */
/*                                                                  */
/* Both hosts create the director at (0, 0) - the ROI probes are    */
/* object-relative, so any other position shifts them.              */
/*                                                                  */
/* Desert.c4d symbols are resolved at RUNTIME (C4Id("SNDT")) so     */
/* the smoke can load this pack without Desert.c4d: compile-time    */
/* id references throw C4AulParseError "unknown identifier"         */
/* (C4AulParse.cpp:2229).                                           */

#strict 2

local ground_row;      // absolute landscape row of the ground top
local f_launch_storm;  // playable: launch SNDT on breach
local f_seed_sea;      // seed the sea-contact leg (spec's CastPXS fallback)
local f_breached, f_flam, f_rock, f_rain, f_buried;
local i_lava0, i_rock0, i_water0;  // paint-time baselines

protected func Construction()
{
	ground_row = 200;
	f_launch_storm = false;
	f_seed_sea = false;
	f_breached = false;
	f_flam = false;
	f_rock = false;
	f_rain = false;
	f_buried = false;
	i_lava0 = 0;
	i_rock0 = 0;
	i_water0 = 0;
	return true;
}

/* ---- Host API --------------------------------------------------- */

public func SetGroundRow(int g) { ground_row = g; return true; }

public func SetLaunchStorm(bool f) { f_launch_storm = f; return true; }

/* Secondary failure: the dam's right flank gives way - lava sprays
   directly into the sea. Physics finding (cycle 86, 4 geometry
   variants): the pan-overflow route is unreachable - the oil fire
   consumes pan + lava before the mix can top the rim, and the
   residual cools to Ashes. This seeds the sea-contact leg via
   CastPXS (the spec's documented fallback), mirroring the proven
   LavaWaterSmoke pattern: lava PXS onto static water -> Rock
   (LSProduct) + Steam (PXSProduct, Rate=100). */
public func SetSeedSea(bool f) { f_seed_sea = f; return true; }

public func IsBreached() { return f_breached; }
public func HasFlam()    { return f_flam; }
public func HasRock()    { return f_rock; }
public func HasRain()    { return f_rain; }
public func HasBuried()  { return f_buried; }

public func AllBeats()
{
	return f_breached && f_flam && f_rock && f_rain && f_buried;
}

/* ---- Apparatus -------------------------------------------------- */

public func PaintCascade(int g)
{
	var G = g;
	// Left wall (reservoir + pan).
	DrawMaterialQuad("Rock", 100, G-160, 110, G-160, 110, G, 100, G);
	// Reservoir right wall (narrows the lava lake).
	DrawMaterialQuad("Rock", 300, G-160, 310, G-160, 310, G-90, 300, G-90);
	// Diggable sandstone keystone floor under the lava lake.
	DrawMaterialQuad("Sandstone", 110, G-100, 300, G-100, 300, G-90, 110, G-90);
	// Static lava reservoir.
	DrawMaterialQuad("Lava", 120, G-130, 290, G-130, 290, G-100, 120, G-100);
	// Pan rim / sea left wall.
	DrawMaterialQuad("Rock", 450, G-90, 460, G-90, 460, G, 450, G);
	// Sea right wall.
	DrawMaterialQuad("Rock", 890, G-90, 900, G-90, 900, G, 890, G);
	// Static oil pan (top row 5 px below the pan rim crest).
	DrawMaterialQuad("Oil", 120, G-85, 440, G-85, 440, G-1, 120, G-1);
	// Static sea.
	DrawMaterialQuad("Water", 470, G-50, 880, G-50, 880, G-1, 470, G-1);
	// Dune slab (drift source, left of the reservoir).
	DrawMaterialQuad("Sand", 20, G-50, 90, G-50, 90, G-1, 20, G-1);

	// Paint-time baselines (deterministic, ordering-immune).
	i_lava0  = CountRegion(Material("Lava"),  120, G-130, 290, G-100);
	i_rock0  = CountRegion(Material("Rock"),  460, G-50,  890, G-1);
	// Rain ROI (Revision 1 par.4): the pinned wind (Wind=30,0) pushes
	// the condensed steam against the tank wall's inner face, so the
	// rain deposits in the sluice gap x[881,889] between the sea
	// surface and the wall - NOT right of the tank (x[901,990]).
	i_water0 = CountRegion(Material("Water"), 881, G-10, 889, G-1);
	return true;
}

/* ---- TimerCall (DefCore Timer=35) -------------------------------- */

public func Poll()
{
	var G = ground_row;

	// Beat 1: breach - the reservoir drained under 95% of baseline.
	// Count Lava + Ashes: lava cools to Ashes in place (total-preserving),
	// so the beat only fires when material actually LEAVES the reservoir.
	if (!f_breached)
	{
		var i_lava = CountRegion(Material("Lava"), 120, G-130, 290, G-100)
		           + CountRegion(Material("Ashes"), 120, G-130, 290, G-100);
		if (i_lava * 100 < i_lava0 * 95)
		{
			f_breached = true;
			Log("The keystone breaks -- the lake of fire is loose!");
			if (f_launch_storm)
				LaunchWeatherEvent(C4Id("SNDT"), 50, 100);
			if (f_seed_sea)
				CastPXS("Lava", 400, 20, 670, 120);
		}
	}

	// Beat 2: FLAM in the oil-pan ROI (sticky).
	if (!f_flam && FindObject(FLAM, 110, G-100, 340, 110))
	{
		f_flam = true;
		Log("The oil catches fire!");
	}

	// Beat 3: rock crust in the sea band (delta >= 33 px).
	if (!f_rock)
	{
		var i_rock = CountRegion(Material("Rock"), 460, G-50, 890, G-1);
		if (i_rock - i_rock0 >= 33)
		{
			f_rock = true;
			Log("Where fire meets the sea, stone is born.");
		}
	}

	// Beat 4: rain deposits in the sluice gap (delta >= 10 px).
	if (!f_rain)
	{
		var i_water = CountRegion(Material("Water"), 881, G-10, 889, G-1);
		if (i_water - i_water0 >= 10)
		{
			f_rain = true;
			Log("The sea flashes to steam -- rain sweeps the burnt terrace.");
		}
	}

	// Beat 5: any walker caught by the drift director (sticky).
	if (!f_buried)
	{
		var victim;
		for (victim in FindObjects(Find_NoContainer()))
			if (GetEffect("SandBuried", victim))
			{
				f_buried = true;
				Log("The dune swallows the beach walkers.");
				break;
			}
	}
	return true;
}

/* Static material cells in the rectangle [x1,x2] x [y1,y2]. */
private func CountRegion(int mat, int x1, int y1, int x2, int y2)
{
	var count = 0;
	var x, y;
	for (x = x1; x <= x2; x++)
		for (y = y1; y <= y2; y++)
			if (GetMaterial(x, y) == mat)
				count++;
	return count;
}
