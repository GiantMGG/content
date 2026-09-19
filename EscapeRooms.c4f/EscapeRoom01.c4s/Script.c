/*-- EscapeRoom01 "The Siphon" (cycle 147, RB). --*/
/* A cistern hangs on a rock promontory; its sand floor plug caps the  */
/* only way out. Dig the plug (shovel: Sand is DigFree=1) or blast it  */
/* with the FLNT in the side niche, and the water drops through an     */
/* enclosed ROCK chute into a rock-lined float basin embedded in the   */
/* chamber floor. When the basin holds enough water the escape gate    */
/* opens.                                                              */
/*                                                                     */
/* Enclosed-path note (cycle-147 RB physics probe, scratch/147/        */
/* probe-c.txt): water PXS landing on the default Material=Earth map   */
/* get absorbed, so the whole water path must touch only Rock -- the   */
/* chute walls and basin shell are carved inside one rock promontory,  */
/* and the cistern floor is the only opening. EscapeSiphonSmoke (the   */
/* scale model) measured 8400 of a 9396-cell cistern delivered with    */
/* zero loss on this geometry; the room threshold below is frozen with */
/* a >=2x margin against the room-scale probe.                          */
/*                                                                     */
/* Geometry (1000x400 map, ground top y=200):                          */
/*   chamber x[350,720] y[70,242], interior carved to air, entry door   */
/*   left at ground level, exit doorway right (EGAT gate).              */
/*   promontory x[430,600] y[150,242] carries the cistern shell         */
/*   x[440,580] y[80,150]: water fill x[446,540] y[86,144], sand floor   */
/*   plug x[500,546] y[144,150], right shelf x[546,580] y[144,150].     */
/*   chute x[500,546] y[150,205] carved in the promontory; basin         */
/*   x[440,590] y[204,241] carved below the chamber floor, rock floor    */
/*   y[242,252]. Right staircase climbs the promontory to the shelf.     */

#strict 2

/* Basin fill floor: the director opens the gate when the float basin  */
/* holds at least this much static water. Room-scale probe              */
/* (.opencode/scratch/147/rb-room01-probe.txt, cycle-147 RB) dug the    */
/* same plug on the same paint and settled at basin 5460 of a 5509-cell */
/* cistern (cist 49, zero map loss, steady from step 3). 2600 keeps a   */
/* 2.1x margin below the worst green observation (5460). Never relax    */
/* this for a failing run -- fix the geometry.                          */
static const BASIN_WATER_MIN = 2600;

/* Basin ROI: the rock-lined float basin interior (below the floor). */
static const BASIN_X1 = 440;
static const BASIN_Y1 = 204;
static const BASIN_X2 = 589;
static const BASIN_Y2 = 241;

static g_pGate;

protected func Initialize()
{
	// --- The chamber shell (rock, sealed, Room-01 pattern) -----------
	DrawMaterialQuad("Rock", 350, 70, 358, 70, 358, 242, 350, 242);   // left wall
	DrawMaterialQuad("Rock", 712, 70, 720, 70, 720, 242, 712, 242);   // right wall
	DrawMaterialQuad("Rock", 350, 70, 720, 70, 720, 74, 350, 74);     // ceiling
	DrawMaterialQuad("Rock", 350, 198, 720, 198, 720, 202, 350, 202); // floor
	FreeRect(358, 74, 354, 124);                                      // interior x[358,711] y[74,197]
	FreeRect(348, 176, 12, 22);                                       // entry door (ground level)
	FreeRect(710, 176, 12, 22);                                       // exit doorway (ground level)

	// --- Promontory + basin shell (one solid rock mass) --------------
	DrawMaterialQuad("Rock", 430, 150, 600, 150, 600, 242, 430, 242); // promontory x[430,599] y[150,241]
	DrawMaterialQuad("Rock", 430, 242, 600, 242, 600, 252, 430, 252); // basin floor y[242,251]

	// --- Cistern on the promontory -----------------------------------
	DrawMaterialQuad("Rock", 440, 80, 446, 80, 446, 150, 440, 150);   // left wall
	DrawMaterialQuad("Rock", 574, 80, 580, 80, 580, 144, 574, 144);   // right lip (above the shelf)
	DrawMaterialQuad("Rock", 440, 80, 580, 80, 580, 86, 440, 86);     // top lid
	DrawMaterialQuad("Rock", 446, 144, 500, 144, 500, 150, 446, 150); // floor left of the plug
	DrawMaterialQuad("Sand", 500, 144, 546, 144, 546, 150, 500, 150); // sand floor plug (the dig target)
	DrawMaterialQuad("Water", 446, 86, 540, 86, 540, 144, 446, 144);  // static water fill
	// Right shelf (x[546,580] y[144,150]) is promontory rock: the clonk
	// stands here and digs left into the plug, or throws the FLNT.

	// --- Enclosed chute + float basin (carved to air) ----------------
	FreeRect(500, 150, 46, 56);                                       // chute x[500,545] y[150,205]
	FreeRect(440, 204, 150, 38);                                      // basin x[440,589] y[204,241]

	// --- Right staircase: chamber floor up to the promontory top -----
	DrawMaterialQuad("Rock", 600, 186, 660, 186, 660, 198, 600, 198); // step 1 (top y=186)
	DrawMaterialQuad("Rock", 600, 174, 650, 174, 650, 186, 600, 186); // step 2 (top y=174)
	DrawMaterialQuad("Rock", 600, 162, 640, 162, 640, 174, 600, 174); // step 3 (top y=162)
	DrawMaterialQuad("Rock", 600, 150, 630, 150, 630, 162, 600, 162); // step 4 (top y=150)

	// --- FLNT niche on the right wall, beside the exit ---------------
	DrawMaterialQuad("Rock", 690, 176, 710, 176, 710, 180, 690, 180); // niche roof
	CreateObject(FLNT, 700, 190, NO_OWNER);

	// --- The exit gate in the doorway: locked until solved -----------
	g_pGate = CreateObject(EGAT, 716, 188, NO_OWNER);
	if (g_pGate) g_pGate->Lock();

	// --- Paint-baseline asserts --------------------------------------
	var cist = CountMatRegion(Material("Water"), 446, 86, 540, 144);
	if (cist < 5000)
		FatalError(Format("EscapeRoom01 FAIL: cistern fill short (%d)", cist));
	if (CountMatRegion(Material("Water"), BASIN_X1, BASIN_Y1, BASIN_X2, BASIN_Y2) != 0)
		FatalError("EscapeRoom01 FAIL: basin not empty at start");

	Log("EscapeRoom01: The Siphon begins.");
	StoryMessage("$MsgIntro$");
	// Repeat-self button (SaltRoad01 pattern).
	SetNextMission("EscapeRooms.c4f\\EscapeRoom01.c4s", "$BtnReplay$", "$BtnReplayDesc$");
	AddEffect("EscapeRoom", 0, 1, 35, 0, 0);
	return true;
}

// global, not private: the director's global func calls it bare-name
// and bare-name calls inside global funcs resolve only through the
// engine-owned global func map (cycle-146 func-map lesson).
// NOTE: CustomMessage signature is (msg, obj, owner, offx, offy, clr,
// idDeco, portrait, flags, hsize) - the color sits at position 6.
// 9-arg form only: a trailing 0 shifts the color into the idDeco slot
// and the message silently drops (cycle-147 sibling defect, Room 03).
global func StoryMessage(string msg)
{
	var plr = 0;
	if (GetPlayerCount() > 0) plr = GetPlayerByIndex(0);
	return CustomMessage(msg, 0, plr, 0, 0, 0xffffff, 0, "Portrait:SCLK::0000ff::1", MSG_Bottom);
}

global func FxEscapeRoomStart(target, effect, temp) { return 1; }

global func FxEscapeRoomTimer(object target, int effect, int timer)
{
	// Solved predicate: the float basin has taken the cistern's water.
	if (g_pGate && g_pGate->IsLocked())
	{
		var iWater = CountMatRegion(Material("Water"),
		                            BASIN_X1, BASIN_Y1, BASIN_X2, BASIN_Y2);
		if (iWater >= BASIN_WATER_MIN)
		{
			g_pGate->Unlock();
			g_pGate->Access();
			Log("EscapeRoom01: the basin is full - the exit gate opens.");
			StoryMessage("$MsgSolved$");
			SetNextMission("EscapeRooms.c4f\\EscapeRoom02.c4s", "$BtnNext$", "$BtnNextDesc$");
			GameOver();
			return -1;
		}
	}
	return 1;
}

/* Static material cells in the rectangle [x1,x2] x [y1,y2]. */
global func CountMatRegion(int mat, int x1, int y1, int x2, int y2)
{
	var count = 0;
	var x, y;
	for (x = x1; x <= x2; x++)
		for (y = y1; y <= y2; y++)
			if (GetMaterial(x, y) == mat)
				count++;
	return count;
}
