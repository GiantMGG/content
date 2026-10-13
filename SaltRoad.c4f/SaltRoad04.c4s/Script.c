/*-- Salt Road, Act IV: The Salt Run (cycle 226). --*/
/* Work the desert's first salt works at the oasis: boil the brine    */
/* in the salt pans, load two camels with SALT, and run the loaded    */
/* caravan east past scripted ambushes to the far market station.     */
/* Win: the salt is delivered; lose: the caravan dies with it.        */

#strict 2

static g_iPhase;      // 0 salt works, 1 on the road, 2 ended
static g_iArrived;    // camels through the market gate
static g_iAmbushIdx;  // next ambush threshold index
static g_iCrewSeen;   // crew-wiped watch latch (see Task 3)

static const SALTROAD4_CARGO    = 6;  // SALT riding camels to depart
static const SALTROAD4_DELIVERY = 6;  // SALT in the market rect to win

protected func Initialize()
{
	g_iPhase = 0;
	g_iArrived = 0;
	g_iAmbushIdx = 0;
	g_iCrewSeen = 0;

	// The caravan-to-be: two camels at the west camp.
	CreateObject(CAML, 140, GroundY(140) - 20, NO_OWNER);
	CreateObject(CAML, 180, GroundY(180) - 20, NO_OWNER);

	// The salt works at the dune-valley floor (see Landscape.txt).
	// The OASS stands on the west bank of the pond, seated 2px deep
	// so its Construction() notch bites into the bank beside it.
	CreateObject(OASS, 610, GroundY(610) - 2, NO_OWNER);
	// Road repair: the map's chunky-rim rendering dithers a single
	// 8px-wide, 8-11px deep notch into the valley floor at x~656
	// (frame-0 profile, scratch/226 t5). Fill it with the local Sand
	// so the caravan road stays flat through the salt works.
	var fy = GroundY(644);
	DrawMaterialQuad("Sand", 656, fy, 664, fy, 664, fy + 12, 656, fy + 12, false);
	// The painted waterline: a flush sheet seated INTO the ground at
	// the valley floor -- the quad's top edge is 1px below the first-
	// solid row, so every painted pixel is at/below the local surface
	// (SaltChainSmoke determinism note: water above the ground line
	// does not stick) and the sheet is laterally bounded by ground at
	// the same rows: it holds without a dug pit, and the caravan
	// wades straight across it (no rim step on the road).
	var wy = GroundY(644);
	DrawMaterialQuad("Water", 624, wy + 1, 672, wy + 1, 672, wy + 6, 624, wy + 6, false);
	Log(Format("SDBG pond fy=%d wy=%d post GY644=%d GY648=%d", fy, wy, GroundY(644), GroundY(648)));  // TEMP diagnostic
	// The pan in the brine pond: GroundY reads through the non-solid
	// painted water to the bed beneath, so the pan settles onto the
	// bed inside the sheet and its flank probes sample the pool.
	// Permanent placement pin: at least one ShoreContact probe offset
	// (SaltPan.c4d) must read the painted Water from tick 0.
	var pan = CreateObject(SLTP, 648, GroundY(648) - 6, NO_OWNER);
	if (pan)  // TEMP diagnostic block
		Log(Format("SDBG pan y=%d probes %d %d %d %d %d water=%d",
			GetY(pan), pan->GetMaterial(0, 10), pan->GetMaterial(-17, 4),
			pan->GetMaterial(17, 4), pan->GetMaterial(-17, -2),
			pan->GetMaterial(17, -2), Material("Water")));  // TEMP diagnostic
	// Permanent placement pin: at least one ShoreContact probe offset
	// (SaltPan.c4d) must read the painted Water from tick 0.
	if (pan)
	{
		var water = Material("Water");
		if (pan->GetMaterial(0, 10) != water
		 && pan->GetMaterial(-17, 4) != water
		 && pan->GetMaterial(17, 4) != water
		 && pan->GetMaterial(-17, -2) != water
		 && pan->GetMaterial(17, -2) != water)
			Log("Salt Road Act IV: salt pan not on the waterline (placement defect)");
	}

	// The quarry on the painted sandstone vein (Act-I verbatim shape):
	// the vein is a slope-following stratum slab (top edge 3px proud
	// of the road -- the caravan steps over it), not a flat quad, so
	// the dune road stays walkable. Placement pin unchanged.
	var q1 = GroundY(720), q2 = GroundY(800);
	DrawMaterialQuad("Sandstone", 720, q1 - 3, 800, q2 - 3, 800, q2 + 13, 720, q1 + 13, false);
	var quarry = CreateObject(QRRY, 760, (q1 + q2) / 2 - 5, NO_OWNER);
	if (quarry)
		if (quarry->GetMaterial(0, 4) != Material("Sandstone"))
			Log("Salt Road Act IV: quarry not on sandstone (placement defect)");

	// Palm groves on the oasis banks (Act-I DATP shape).
	CreateObject(DATP, 588, GroundY(588) - 10, NO_OWNER);
	CreateObject(DATP, 596, GroundY(596) - 10, NO_OWNER);
	CreateObject(DATP, 690, GroundY(690) - 10, NO_OWNER);

	// The far market station.
	CreateObject(MKTS, LandscapeWidth() - 100, GroundY(LandscapeWidth() - 100) - 10, NO_OWNER);

	Log("Salt Road Act IV: the salt run begins.");
	StoryMessage("$MsgIntro$");
	// Repeat-self button (Tutorial01 pattern).
	SetNextMission("SaltRoad.c4f\\SaltRoad04.c4s", "$BtnReplayActIV$", "$BtnReplayActIVDesc$");
	AddEffect("SaltRoadAct", 0, 1, 35, 0);
	TProf0();  // TEMP diagnostic: frame-0 surface profile (removed before commit)
	return true;
}

// TEMP diagnostic: frame-0 GY dump every 8px (removed before commit)
global func TProf0()
{
	var x, s;
	for (x = 0; x < LandscapeWidth(); x += 8)
	{
		s = Format("%s %d", s, GroundY(x));
		if ((x / 8) % 12 == 11)
		{
			Log(Format("T0 %d%s", x - 88, s));
			s = "";
		}
	}
	if (s) Log(Format("T0 %d%s", x - 96 + 8, s));
	return true;
}

// global, not private: the effect-director global func below calls these
// bare-name, and bare-name calls inside global funcs resolve only through
// the engine-owned global func map -- scenario-local scope is not visible
// there (cycle-146 func-map lesson, FirstLight.c4s precedent).
global func GroundY(int x)
{
	var y = 0;
	while (y < LandscapeHeight() && !GBackSolid(x, y)) y++;
	return y;
}

// TEMP diagnostic: full-map surface profile dump (removed before commit)
global func TProf()
{
	var x;
	for (x = 0; x < LandscapeWidth(); x += 2)
		Log(Format("TP %d %d", x, GroundY(x)));
	return true;
}

global func StoryMessage(string msg)
{
	var plr = 0;
	if (GetPlayerCount() > 0) plr = GetPlayerByIndex(0);
	return CustomMessage(msg, 0, plr, 0, 0, 0xffffff, 0, "Portrait:SCLK::0000ff::1", MSG_Bottom);
}

/* ---- cargo + delivery counters ---- */

// SALT riding the caravan: contained SALT rides its container's
// position every tick (C4Movement.cpp CopyMotion), so a camel's hold
// reads at the camel's x/y.
global func CargoRiding()
{
	var camel, count = 0;
	for (camel in FindObjects(Find_ID(CAML)))
		count += ContentsCount(SALT, camel);
	return count;
}

// SALT inside the market rect at the far east: the band spans the
// MKTS stall and the caravan's arrival waypoint (Act-I DatesSecured
// AtRect shape).
global func SaltDelivered()
{
	var salt, count = 0;
	for (salt in FindObjects(Find_ID(SALT), Find_AtRect(LandscapeWidth() - 200, 0, 200, LandscapeHeight())))
		count++;
	return count;
}

// The lead camel's x (Act-II verbatim).
global func LeadCamelX()
{
	var camel, maxx = 0;
	for (camel in FindObjects(Find_ID(CAML)))
		if (GetX(camel) > maxx) maxx = GetX(camel);
	return maxx;
}

/* ---- gate hook + scene direction ---- */

// Gate hook (round-gate win leg, beats rig): load every camel with
// count SALT; returns the total loaded.
global func SaltRoad04_LoadCargo(int count)
{
	var camel, i, loaded = 0;
	for (camel in FindObjects(Find_ID(CAML)))
		for (i = 0; i < count; i++)
			if (CreateContents(SALT, camel)) loaded++;
	return loaded;
}

// The loaded caravan rides for the far market station (Act-I
// StartCaravan shape). The ComDir drive effect below is REQUIRED in
// addition to the Camel.c4d puppet: the puppet re-issues
// SetCommand("MoveTo") every frame, and the engine spends a fresh
// command's first execution on C4Command::InitEvaluation() only
// (C4Command.cpp:1569) -- MoveTo(), the part that sets the walk
// ComDir, never runs. The scenario drive sets ComDir directly so
// the caravan actually walks, while the puppet keeps doing the
// waypoint logic and fires the real arrival GameCall. The effect
// must be target 0: object-targeted effect callbacks resolve on the
// target's def script only (C4Effect::GetCallbackScript).
global func Depart()
{
	var camel;
	Log(Format("SDBG depart LW=%d target=%d", LandscapeWidth(), LandscapeWidth() - 120));  // TEMP diagnostic
	for (camel in FindObjects(Find_ID(CAML)))
	{
		Log(Format("SDBG camel pre-start x=%d y=%d comd=%d", GetX(camel), GetY(camel), GetComDir(camel)));  // TEMP diagnostic
		SaltRoad_StartCaravan(camel, [LandscapeWidth() - 120]);
	}
	if (!GetEffect("SaltRoadDrive", 0)) AddEffect("SaltRoadDrive", 0, 1, 2, 0);
	Log("Salt Road Act IV: the caravan rides east.");
	StoryMessage("$MsgDeparture$");
	return true;
}

// The caravan drive (see Depart): keep every camel whose Camel.c4d
// caravan puppet is still active walking east; dissolve once no
// caravan is active any more (all camels arrived or lost).
global func FxSaltRoadDriveTimer(target, effect, time)
{
	var camel, active;
	for (camel in FindObjects(Find_ID(CAML)))
		if (SaltRoad_IsCaravanActive(camel))
		{
			active = true;
			SetComDir(COMD_Right, camel);
		}
	if (!active) return FX_Execute_Kill;
	return FX_OK;
}

// Road ambushes fire when the lead camel crosses the map quarters
// (Act-II AmbushCheck shape).
global func AmbushCheck()
{
	var thresholds = [LandscapeWidth() / 4, LandscapeWidth() / 2, 3 * LandscapeWidth() / 4];
	if (g_iAmbushIdx < GetLength(thresholds) && LeadCamelX() >= thresholds[g_iAmbushIdx])
	{
		var x = thresholds[g_iAmbushIdx];
		Log(Format("SDBG ambush %d fires at x=%d gy=%d", g_iAmbushIdx, x, GroundY(x)));  // TEMP diagnostic
		SaltRoad_SpawnRaid(x, GroundY(x) - 40, 2);
		g_iAmbushIdx++;
		StoryMessage("$MsgAmbush$");
	}
	return true;
}

/* ---- the director ---- */

// GLOBAL func: effects on target 0 need GLOBAL callbacks (the
// SaltChainSmoke lesson; cycle-146 func-map note carried by Acts
// I-III).
global func FxSaltRoadActTimer(target, effect, time)
{
	{  // TEMP diagnostic block: per-tick caravan state dump
		var c, s = "";
		for (c in FindObjects(Find_ID(CAML)))
			s = Format("%s[%d,%d,%d]", s, GetX(c), GetY(c), GetComDir(c));
		Log(Format("SDBG tick ph=%d caml=%d leadx=%d arr=%d amb=%d pos=%s",
			g_iPhase, ObjectCount(CAML), LeadCamelX(), g_iArrived, g_iAmbushIdx, s));
	}
	// Win re-check (Act-I review-F1 shape): a camel killed in transit
	// after the first arrival never fires another callback, so poll
	// completion here every director tick: some camel through the
	// market gate AND the delivered SALT at the stall.
	if (g_iPhase < 2 && g_iArrived > 0 && SaltDelivered() >= SALTROAD4_DELIVERY)
	{
		g_iPhase = 2;
		return ActComplete();
	}

	// Defeat watch: no camels left means no caravan, no Salt Road.
	if (g_iPhase < 2 && ObjectCount(CAML) == 0)
	{
		g_iPhase = 2;
		StoryMessage("$MsgDefeat$");
		Log("Salt Road Act IV: the caravan is lost.");
		GameOver();
		return FX_OK;
	}

	// Crew-wiped watch: defeat also fires when the crew dies off. The
	// g_iCrewSeen latch keeps playerless headless runs from
	// false-defeating before any crew exists.
	if (!g_iCrewSeen && ObjectCount(CLNK) > 0) g_iCrewSeen = 1;
	if (g_iPhase < 2 && g_iCrewSeen && ObjectCount(CLNK) == 0)
	{
		g_iPhase = 2;
		StoryMessage("$MsgDefeat$");
		Log("Salt Road Act IV: the caravan is lost.");
		GameOver();
		return FX_OK;
	}

	if (g_iPhase == 0)
	{
		// Departure check: the caravan rides when the salt is loaded.
		if (CargoRiding() >= SALTROAD4_CARGO)
		{
			g_iPhase = 1;
			Depart();
		}
	}
	else if (g_iPhase == 1)
	{
		AmbushCheck();
	}
	return FX_OK;
}

// Arrival callback (GameCall from the caravan puppet, Camel.c4d).
// GameCall receivers must be public (script-local): global funcs are
// engine-owned and invisible to Game.Script's own-table lookup.
public func SaltRoadCaravanArrived(object camel)
{
	Log(Format("SDBG arrived x=%d", GetX(camel)));  // TEMP diagnostic
	g_iArrived++;
	// Halt the camel at the market (the drive effect skips it from
	// now on -- its caravan puppet dissolves this same tick).
	SetComDir(COMD_Stop, camel);
	return true;
}

// The single Act-IV victory: message, delivered-salt count and game
// over. NO next-act button (Act V is a later tranche; the repeat-self
// button is already set in Initialize).
global func ActComplete()
{
	g_iPhase = 2;
	StoryMessage("$MsgActComplete$");
	Log(Format("Salt Road Act IV: %d SALT delivered at the market.", SaltDelivered()));
	GameOver();
	return true;
}
