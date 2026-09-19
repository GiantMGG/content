/*-- EscapeRoom03 "The Undertow" (cycle 147). --*/
/* A column of sand chokes the vertical exit shaft. Beneath the  */
/* column rests a sandstone keystone shelf; below the shelf an    */
/* undercroft with a sump pit. Undermine the keystone -- dig it   */
/* slowly and safely with the shovel, or blast it instantly with  */
/* the flint provided (sand-spray hazard) -- and the column       */
/* cascades into the pit (Sand Instable=1; both dig and blast     */
/* fire CheckInstabilityRange, C4Landscape.cpp:1113/1146). The    */
/* room director polls the choke ROI; when the sand has run out,  */
/* the exit gate opens -- but only after the escape key from the  */
/* undercroft niche has been used (the key moment: ERKY.Activate  */
/* -> gate->Unlock(); the director then completes gate->Access()).*/
/*                                                                  */
/* Geometry (side view, all on the default 1000x400 earth map):    */
/*                                                                  */
/*    ceiling y[120,125]                                            */
/*    x[350,358] left wall (entry door at ground level y[176,200]) */
/*    shaft towers x[558,570] and x[650,662], shaft interior       */
/*    x[570,650]: sand column x[590,630] y[156,236], keystone      */
/*    x[558,662] y[236,244], undercroft cavity + sump pit below    */
/*    chamber floor y[306,312]                                     */
/*                                                                  */
/* The gate stands at the shaft top on a platform capping the      */
/* shaft; when sand clears AND the key has been used, the director */
/* opens it and the room wins (last room: replay button only).     */

#strict 2

/* Choke / cascade thresholds shared with EscapeUndertowSmoke:     */
/* the smoke measured a 12 px drained-choke residual (worst green) */
/* on the same physics; 50 keeps >=4x margin while a hung column   */
/* reads >= 1700 (never move this threshold, fix the geometry).    */
static const UNDERTOW_CLEAR_MAX = 50;

/* Choke ROI: the shaft segment where the column stood.            */
static const CHOKE_X1 = 570;
static const CHOKE_Y1 = 150;
static const CHOKE_X2 = 650;
static const CHOKE_Y2 = 250;

static g_pGate;
static g_pKey;
static g_bHinted;

protected func Initialize()
{
	// --- The chamber shell (rock, sealed, Room-01 pattern) --------
	DrawMaterialQuad("Rock", 350, 120, 358, 120, 358, 312, 350, 312);   // left wall
	DrawMaterialQuad("Rock", 702, 120, 710, 120, 710, 312, 702, 312);   // right wall
	DrawMaterialQuad("Rock", 350, 120, 710, 120, 710, 125, 350, 125);   // ceiling
	DrawMaterialQuad("Rock", 350, 306, 710, 306, 710, 312, 350, 312);   // floor
	// Interior carve to air.
	FreeRect(358, 125, 344, 181);                                       // x[358,702] y[125,306]
	// Entry door in the left wall at exterior ground level (ground top = 200).
	// Room-01 alignment: bottom at the surface (y[176,200]) so the standing
	// clonk cross-section (body y[180,200]) clears the wall lintel y[120,175].
	FreeRect(348, 176, 12, 24);                                         // x[348,360] y[176,200]
	// Stepped descent ledges on the way down.
	DrawMaterialQuad("Rock", 360, 222, 430, 222, 430, 228, 360, 228);   // entry step
	DrawMaterialQuad("Rock", 360, 262, 460, 262, 460, 268, 360, 268);   // mid ledge

	// --- The exit shaft (right side): tower walls + gate platform --
	DrawMaterialQuad("Rock", 558, 150, 570, 150, 570, 306, 558, 306);   // shaft left tower
	DrawMaterialQuad("Rock", 650, 150, 662, 150, 662, 306, 650, 306);   // shaft right tower
	// Gate platform capping the shaft top; the gate stands on it.
	DrawMaterialQuad("Rock", 570, 150, 650, 150, 650, 156, 570, 156);
	// Shaft climbing grips (playtest nicety, harmless if unused).
	DrawMaterialQuad("Rock", 650, 178, 654, 178, 654, 182, 650, 182);
	DrawMaterialQuad("Rock", 558, 208, 562, 208, 562, 212, 558, 212);
	// The exit gate at the shaft top, locked at start.
	g_pGate = CreateObject(EGAT, 610, 141, NO_OWNER);
	if (g_pGate) g_pGate->Lock();

	// --- The choke: sandstone keystone shelf + static sand column --
	DrawMaterialQuad("Sandstone", 558, 236, 662, 236, 662, 244, 558, 244);
	DrawMaterialQuad("Sand", 590, 156, 630, 156, 630, 236, 590, 236);

	// --- The undercroft: cavity + sump pit below the keystone -----
	// FreeRect (not DigFreeRect): the pit must cut through the rock
	// floor band too, and Rock is not DigFree.
	FreeRect(572, 244, 76, 100);                                        // x[572,648] y[244,344]
	// Low access passage from the chamber floor into the undercroft.
	FreeRect(556, 282, 24, 24);                                         // x[556,580] y[282,306]
	// Undergrove shelves so the keystone underside is reachable with
	// a shovel or an upward flint throw.
	DrawMaterialQuad("Rock", 572, 278, 616, 278, 616, 284, 572, 284);
	DrawMaterialQuad("Rock", 572, 300, 612, 300, 612, 306, 572, 306);
	// Key niche shelf on the left tower inside the undercroft.
	DrawMaterialQuad("Rock", 558, 288, 568, 288, 568, 294, 558, 294);

	// --- Actors ----------------------------------------------------
	// Escape key in the undercroft niche, bound to the exit gate
	// (the key moment: ERKY.Activate -> gate->Unlock()).
	// At y=286 the key's bottom (y+2) rests on the niche shelf top (288).
	g_pKey = CreateObject(ERKY, 563, 286, NO_OWNER);
	if (g_pKey) g_pKey->Set(g_pGate);
	// Blast option: a flint on the chamber floor.
	CreateObject(FLNT, 430, 300, NO_OWNER);

	Log("EscapeRoom03: The Undertow begins.");
	StoryMessage("$MsgIntro$");
	SetNextMission("EscapeRooms.c4f\\EscapeRoom03.c4s", "$BtnReplay$", "$BtnReplayDesc$");

	g_bHinted = false;
	AddEffect("EscapeRoom", 0, 1, 35, 0, 0);
	return true;
}

// global, not private: the director's global func calls it bare-name
// and bare-name calls inside global funcs resolve only through the
// engine-owned global func map (cycle-146 func-map lesson).
global func StoryMessage(string msg)
{
	var plr = 0;
	if (GetPlayerCount() > 0) plr = GetPlayerByIndex(0);
	return CustomMessage(msg, 0, plr, 0, 0, 0xffffff, 0, "Portrait:SCLK::0000ff::1", MSG_Bottom);
}

global func FxEscapeRoomStart(target, effect, temp) { return 1; }

global func FxEscapeRoomTimer(object target, int effect, int timer)
{
	// Solved predicate = the choke ROI is empty of sand (the column
	// has cascaded into the sump). The gate opens only if the escape
	// key was used first (gate unlocked by ERKY.Activate), so the key
	// moment stays real regardless of the action order.
	var iSand = CountMatRegion(Material("Sand"),
	                           CHOKE_X1, CHOKE_Y1, CHOKE_X2, CHOKE_Y2);
	if (iSand <= UNDERTOW_CLEAR_MAX)
	{
		// Key moment: the gate only opens after the key unlocked it.
		if (g_pGate && g_pGate->IsLocked())
		{
			if (!g_bHinted)
			{
				g_bHinted = true;
				StoryMessage("$MsgKeyHint$");
			}
			return 1;
		}
		if (g_pGate)
		{
			g_pGate->Access();
			Log("EscapeRoom03: the sand has run out - the exit gate opens.");
			StoryMessage("$MsgSolved$");
			SetNextMission("EscapeRooms.c4f\\EscapeRoom03.c4s", "$BtnReplay$", "$BtnReplayDesc$");
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
