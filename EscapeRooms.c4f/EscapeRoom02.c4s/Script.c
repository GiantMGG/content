/*-- EscapeRoom02 "The Furnace" (cycle 147, RC). --*/
/* A wall of snow seals the exit door; the furnace lever opens the  */
/* lava furnace under the plug. The lever is NOT wired to the gate  */
/* - the room director senses the throw (GetPhase()), drops the     */
/* furnace lid and blasts the plug base (the "stoke"), then opens   */
/* the gate when the doorway is "mostly gone". The doorway-clear    */
/* predicate is the UNIFIED WIN: the furnace melt feeds it, and so  */
/* does the advertised pick chisel (Snow is DigFree=1 in Snow.c4m)  */
/* - whoever clears the doorway opens the gate.                     */
/*                                                                  */
/* Mechanism note (cycle-147 RC, plan R1 escalation): the temp-     */
/* scan melt (Ice AboveTempConvert=10) converted only ~4 px per     */
/* column per sweep (t3-furnace-smoke.txt: ice 880->836, stalled),  */
/* so no fat doorway plug could melt in play time. The plug is now  */
/* SNOW and the furnace is a LAVA CHARGE: Snow.c4m [Reaction]       */
/* Type=Convert TargetSpec=Incindiary converts snow contacting      */
/* lava to water (SnowMeltSmoke-proven headless). The probe series  */
/* (.opencode/scratch/147/rc-probe-*, cycle-147 RC) showed:         */
/*   - a static snow plug never falls through air, so a simple lid  */
/*     Free leaves a stall (lid gap of any height = no contact);    */
/*   - the winning trigger = FreeRect the lid AND BlastFree the     */
/*     plug base: the blast's CheckInstabilityRange (BlastFreePix   */
/*     runs it at every cell, C4Landscape.cpp:1146) turns the       */
/*     instable snow base into mass movers; the column avalanches   */
/*     into the pool, and the Convert chain melts it bottom-up      */
/*     (snowDoor 750 -> ~13, lava 380 -> 0 in ~2 steps). Even with  */
/*     a furnace that has ashed out pre-lever, the avalanche alone  */
/*     clears the doorway (probe G: snowDoor 10) - no stuck door.   */
/*                                                                  */
/* Climate note (plan deviation, in-code): [Weather] Climate=100    */
/* maps internally to -50 (C4Weather.cpp:47; Arctic.c4s precedent)  */
/* - a cold chamber that stays below the snow melt bound (5) even   */
/* while the seasonal target drifts. The init pins + per-tick       */
/* re-pins lock the cold side until the furnace is stoked. Lava's   */
/* BelowTempConvert=100 slowly ashess the sealed charge (probe G:   */
/* leakage from ~tick 105 on the 500px map, later on the 1000px     */
/* map), which is tolerated: the door still clears by collapse.     */

#strict 2

static const PLUG_ROI_X1   = 512;    // snow-plug sensor ROI (rect around the plug)
static const PLUG_ROI_Y1   = 145;
static const PLUG_ROI_X2   = 532;
static const PLUG_ROI_Y2   = 192;
static const PLUG_MELTED_MAX = 40;   // CAL-FREEZE: doorway-snow end-state after
                                     // either solution. The chisel path clears the
                                     // plug outright (0); EscapeFurnaceSmoke (the
                                     // scale model) measured 6..13 of 750 in the
                                     // doorway band -> room equivalent ~3..7 of
                                     // 407; 40 keeps >=5x margin over the worst
                                     // end-state and 10x below the pre-win plug
                                     // (~407). The unified win predicate reads
                                     // this ROI regardless of how it was cleared.
static const FURNACE_LID_X1 = 517;   // lava-furnace lid (the seal between the snow
static const FURNACE_LID_Y1 = 187;   // plug and the charge); freed by the lever.
static const FURNACE_LID_W  = 11;
static const FURNACE_LID_H  = 5;
static const PLUG_BLAST_X   = 522;   // the "stoke": blast at the plug base, rad 8
static const PLUG_BLAST_Y   = 190;   // (covers the plug base rows + the pool top;
static const FURNACE_BLAST_RAD = 8;  // clears nothing - rock ISN'T BlastFree - but
                                     // fires CheckInstabilityRange -> avalanche).

static g_bFurnace;
static g_pLever;
static g_pGate;

protected func Initialize()
{
	// Cold chamber pin: scenario Climate=100 -> internal -50, seasonal
	// target stays <= -20 (C4Weather.cpp:99-104), so the snow plug never
	// melts on ambient drift (Snow.c4m AboveTempConvert=5). Re-pinned by
	// the director until the furnace is stoked.
	SetTemperature(0);
	SetClimate(-50);

	// --- The chamber (rock shell on the default 1000x400 earth map,
	//     ground top y=200, Room-01 pattern) --------------------------
	// Door gap in the left wall (y[172,216]) aligned with the exterior
	// surface; interior carved to air; floor at y[220,222].
	DrawMaterialQuad("Rock", 350, 120, 352, 120, 352, 171, 350, 171);  // left wall, above the door
	DrawMaterialQuad("Rock", 350, 217, 352, 217, 352, 222, 350, 222);  // left wall, door sill
	DrawMaterialQuad("Rock", 718, 120, 720, 120, 720, 222, 718, 222);  // right wall
	DrawMaterialQuad("Rock", 350, 120, 720, 120, 720, 122, 350, 122);  // ceiling
	DrawMaterialQuad("Rock", 350, 220, 720, 220, 720, 222, 350, 222);  // floor
	FreeRect(352, 122, 366, 98);                                       // carve the interior to air
	// Drain sump at the chamber base: the meltwater collects here
	// ("pre-carve a drain notch", plan 3.1).
	FreeRect(358, 222, 44, 14);

	// --- The exit: divider wall with a snow-plug-sealed opening -------
	DrawMaterialQuad("Rock", 520, 122, 522, 122, 522, 149, 520, 149);  // divider above the opening
	// The doorway plug: 11 x 37 snow column sealing the opening (was
	// Ice before cycle-147 RC; Snow keeps the DigFree=1 chisel path).
	DrawMaterialQuad("Snow", 517, 150, 527, 150, 527, 186, 517, 186);
	// The furnace: a rock pedestal inside the doorway column below the
	// plug, with a carved trough holding the lava charge, capped by the
	// lid (the pedestal's top slab y[187,191]).
	DrawMaterialQuad("Rock", 517, 187, 527, 187, 527, 236, 517, 236);  // pedestal (= lid cap + walls + floor)
	FreeRect(519, 192, 7, 44);                                         // carve the furnace cavity x[519,525] y[192,235]
	DrawMaterialQuad("Lava", 520, 193, 524, 193, 524, 234, 520, 234);  // the charge (5 x 42)
	// The exit gate behind the plug: the director unlocks + opens it
	// when the doorway clears (never wired to the lever directly).
	g_pGate = CreateObject(EGAT, 560, 209, NO_OWNER);
	if (g_pGate) g_pGate->Lock();
	// The furnace lever, mounted on the chamber's left wall.
	g_pLever = CreateObject(ERLV, 362, 184, NO_OWNER);

	Log("EscapeRoom02: The Furnace begins.");
	StoryMessage("$MsgIntro$");
	// Repeat-self button (SaltRoad01 pattern).
	SetNextMission("EscapeRooms.c4f\\EscapeRoom02.c4s", "$BtnReplay$", "$BtnReplayDesc$");

	g_bFurnace = false;
	AddEffect("EscapeRoom", 0, 1, 35, 0, 0);
	return true;
}

// global, not private: the director's global func calls it bare-name
// and bare-name calls inside global funcs resolve only through the
// engine-owned global func map (cycle-146 func-map lesson).
// NOTE: CustomMessage signature is (msg, obj, owner, offx, offy, clr,
// idDeco, portrait, flags, hsize) - the color sits at position 6
// (Western ExtraLog.c4d precedent), NOT before idDeco.
global func StoryMessage(string msg)
{
	var plr = 0;
	if (GetPlayerCount() > 0) plr = GetPlayerByIndex(0);
	return CustomMessage(msg, 0, plr, 0, 0, 0xffffff, 0, "Portrait:SCLK::0000ff::1", MSG_Bottom);
}

global func FxEscapeRoomStart(target, effect, temp) { return 1; }

global func FxEscapeRoomTimer(object target, int effect, int timer)
{
	// Cold pin until the furnace is stoked (drift guard).
	if (!g_bFurnace) SetTemperature(0);

	// Rising-edge latch: the first sight of a thrown lever stokes the
	// furnace - drop the lid and blast the plug base into the charge.
	if (!g_bFurnace && g_pLever && g_pLever->GetPhase() != 0)
	{
		g_bFurnace = true;
		FreeRect(FURNACE_LID_X1, FURNACE_LID_Y1, FURNACE_LID_W, FURNACE_LID_H);
		BlastFree(PLUG_BLAST_X, PLUG_BLAST_Y, FURNACE_BLAST_RAD);
		StoryMessage("$MsgFurnace$");
	}
	// Note: after the throw there is no re-pin - the blast is a single
	// deterministic kick (EscapeFurnaceSmoke-proven); re-blasting every
	// tick would re-churn the settled debris.

	// Solved predicate: the doorway ROI is "mostly gone" - the unified
	// win. Both paths feed it: the furnace melt (progressive; threshold
	// shared with EscapeFurnaceSmoke) and the pick chisel (DigFreeRect
	// clears the plug outright); whichever clears the doorway opens
	// the gate, so the intro's chisel alternative is a true win.
	// The lever->lava mechanism above is untouched by this check.
	if (g_pGate && g_pGate->IsLocked())
	{
		var iSnow = CountMatRegion(Material("Snow"),
		                          PLUG_ROI_X1, PLUG_ROI_Y1, PLUG_ROI_X2, PLUG_ROI_Y2);
		if (iSnow <= PLUG_MELTED_MAX)
		{
			g_pGate->Unlock();
			g_pGate->Access();
			Log("EscapeRoom02: the snow plug is gone - the exit gate opens.");
			StoryMessage("$MsgSolved$");
			SetNextMission("EscapeRooms.c4f\\EscapeRoom03.c4s", "$BtnNext$", "$BtnNextDesc$");
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
