/*-- Dammbruch.c4s -- the Breach showcase (cycle 124). -----------------*/
/*                                                                        */
/* Five beats, all physics, no scripted water:                            */
/*   1. Agency      -- the player digs or blasts the Sandstone keystone   */
/*                     course in the dam's base, from the work ledge at   */
/*                     its valley-side foot (a 20px course: the dig      */
/*                     tunnels through at body height).                   */
/*   2. Anticipation-- the first seeps swell as movers mobilize the      */
/*                     reservoir behind the fresh crack (FlowRate ramp;  */
/*                     not scripted delays).                              */
/*   3. Spectacle   -- JetFall water arcs from the breach and the ledge   */
/*                     lip down the cliff into the valley.                */
/*   4. Consequence -- the flood fronts across the village on the valley  */
/*                     floor.                                             */
/*   5. Aftermath   -- the valley lake settles dead-flat; the goal        */
/*                     completes when the surface is calm.                */
/*                                                                        */
/* The camera is a tick-keyed SetViewOffset sweep (council B3): beats are */
/* polled from physics state, never GetTime().                            */

#strict 2

// Apparatus (landscape 1000x600, ground top y=300 on this map class --
// Feuerstaudamm's SetGroundRow(300) precedent).
static const G = 300;

static g_fBreachSeen, g_fFloodSeen, g_fSettleSeen;

protected func Initialize()
{
	// Plateau: the reservoir foundation; the valley floor is the natural
	// ground 80px below the plateau crest.
	DrawMaterialQuad("Rock", 60, 220, 420, 220, 420, G, 60, G);

	// Reservoir tank on the plateau: left wall + static water fill
	// (DrawMaterialQuad paint, construction -- NOT scripted water). The
	// fill runs right up against the dam face so the keystone dig
	// releases the full head; painted water stays inert until the dig
	// disturbs the dam (probe: zero movers over 1050 ticks undisturbed).
	DrawMaterialQuad("Rock", 80, 140, 88, 140, 88, 220, 80, 220);
	DrawMaterialQuad("Water", 89, 150, 403, 150, 403, 219, 89, 219);

	// Dam (Granite crest, Rock shell, and a 20px-tall Sandstone keystone
	// course seated at its base on a one-row Rock sill: the course is
	// body-height for a clonk standing on the sill line, so the dig
	// tunnels straight through it at walking height).
	DrawMaterialQuad("Granite", 404, 140, 420, 140, 420, 150, 404, 150);
	DrawMaterialQuad("Rock", 404, 150, 420, 150, 420, 199, 404, 199);
	DrawMaterialQuad("Rock", 404, 219, 420, 219, 420, 220, 404, 220);
	DrawMaterialQuad("Sandstone", 404, 199, 420, 199, 420, 219, 404, 219);

	// Work ledge under the keystone course on the dam's valley-side
	// face: dry stance for the dig, and the jet-curtain's launch lip.
	// The player spawns here (Position=43,20).
	DrawMaterialQuad("Rock", 420, 219, 452, 219, 452, 232, 420, 232);

	// Valley rim: bounds the settling lake on the right.
	DrawMaterialQuad("Rock", 880, 250, 888, 250, 888, G, 880, G);

	// Village on the valley floor, in the flood path.
	CreateConstruction(HUT1, 500, G - 2, NO_OWNER, 100, true, true);
	CreateConstruction(HUT2, 560, G - 2, NO_OWNER, 100, true, true);
	CreateConstruction(HUT1, 620, G - 2, NO_OWNER, 100, true, true);

	// Scorpions on the floodplain (Desert SCRP, Feuerstaudamm precedent).
	CreateObject(SCRP, 480, G - 6, NO_OWNER);
	CreateObject(SCRP, 530, G - 6, NO_OWNER);
	CreateObject(SCRP, 610, G - 6, NO_OWNER);

	g_fBreachSeen = false;
	g_fFloodSeen = false;
	g_fSettleSeen = false;
	AddEffect("ShowDirector", 0, 1, 35, 0, 0);

	Log("Der Damm haelt. Der Schlussstein wartet. Grab - oder sprenge.");
	return true;
}

/* Deterministic spawn: the engine's site search can seat the crew on
   the reservoir floor, underwater. Snap them onto the dry work ledge
   (playtest: gui12, spawn fix proven necessary). */
protected func InitializePlayer(int iPlr)
{
	var crew = GetCrew(iPlr);
	if (crew) SetPosition(436, 206, crew);
	return true;
}

global func FxShowDirectorStart(target, effect, temp) { return 1; }

global func FxShowDirectorTimer(object target, int effect, int timer)
{
	// Beat polling (physics-state keyed, never wall-clock).
	var valleyWater = CountMatRegion(Material("Water"), 421, 220, 879, G);
	var mm = GetMassMoverCount();

	if (!g_fBreachSeen && valleyWater > 0) { g_fBreachSeen = true; Log("Das Wasser findet seinen Weg."); }
	if (g_fBreachSeen && !g_fFloodSeen && valleyWater > 3000) { g_fFloodSeen = true; Log("Eine Flutwelle rollt ins Tal!"); }
	if (g_fFloodSeen && !g_fSettleSeen && mm <= 50 && LakeFlat())
	{
		g_fSettleSeen = true;
		Log("Der Staub legt sich. Der See ruht.");
		Log("Dammbruch PASS");
		GameOver();
		return -1;
	}

	// Tick-keyed camera sweep (SetViewOffset, C4Script.cpp:6064): beats
	// polled from physics state, never GetTime(). Reservoir framing
	// pre-breach -> dam on first movement -> valley flood -> calm lake.
	if (!g_fBreachSeen) SetViewOffset(0, -60, -40);
	else if (!g_fFloodSeen) SetViewOffset(0, 0, 0);
	else if (!g_fSettleSeen) SetViewOffset(0, 60, 30);
	else SetViewOffset(0, 0, 0);
	return 1;
}

/* Flat-at-rest over the valley-lake interior span. */
global func LakeFlat()
{
	var min = 32767, max = -32767, cols = 0, x, y, top;
	for (x = 460; x <= 840; x++)
	{
		top = -1;
		for (y = 220; y < G; y++)
			if (GetMaterial(x, y) == Material("Water")) { top = y; break; }
		if (top >= 0)
		{
			++cols;
			if (top < min) min = top;
			if (top > max) max = top;
		}
	}
	if (cols == 0) return false;
	return (max - min) <= 3;
}

/* Static material cells in the rectangle [x1,x2] x [y1,y2]. */
global func CountMatRegion(mat, x1, y1, x2, y2)
{
	var count = 0, x, y;
	for (x = x1; x <= x2; x++)
		for (y = y1; y <= y2; y++)
			if (GetMaterial(x, y) == mat)
				++count;
	return count;
}
