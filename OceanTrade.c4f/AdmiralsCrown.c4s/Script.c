/*-- AdmiralsCrown.c4s -- 2-player co-op convoy dash on the storm lane. --*/
/* P1 (helm) rides the Cog and steers by walking aboard: the director     */
/* mirrors the helm's ComDir into a per-frame Drive effect that holds the */
/* ship's XDir (a floating Cog never sails on its own -- its action's     */
/* StartCall fires once; only in-liquid XDir force moves it, verified at  */
/* cycle 213). P2 (boatswain) does damage control aboard: plug breaches   */
/* (consumes a WOOD bale from the hold), bail (pump action cuts iFlood),  */
/* jettison cargo (heaves a SILK bale overboard, raising freeboard).      */
/* Win = 3 SILK bales delivered at the east port; lose = the Cog          */
/* founders. The flood machine itself (fill/settle/founder) is verified   */
/* frame-by-frame by FloodedCrownSmoke.c4s; this script only wires the    */
/* playable leg and the director schedule.                                */
/*                                                                        */
/* Director = target-0 global effect (gotcha #4) with a plain name        */
/* (gotcha #10); every player-derived lookup is null-guarded so a         */
/* playerless headless boot (playtest_sweep / boot probe) stays           */
/* error-free and the schedule still logs.                                */

#strict 2

static pWestPost, pEastPost, pCog;
static iDelivered;      // bales delivered at the east port
static iLastFlood;      // last flood level while the Cog was CGSH (LOSE telemetry)
static iStormIn;        // frame of the currently scheduled storm launch
static iBreachIn;       // frame of the next breach opening
static iCrewActIn;      // cooldown frame for boatswain crew actions

/* ---- Calibration-owned schedule (nothing in the spec pins frame ----
   numbers; the flood constants live in Cog.c4d/Script.c:15-20). */
static const STORM_START    = 600;   // first STRM launch frame
static const STORM_DUR      = 200;   // STRM duration (StormLaneDirector shape)
static const STORM_CADENCE  = 3600;  // storms repeat while the Cog lives
static const BREACH_1       = 3600;  // first hull-breach frame
static const BREACH_CADENCE = 2200;  // later breaches while the Cog lives
static const MAX_BREACHES   = 4;     // director never opens more slots
static const DOCK_RANGE     = 150;   // px: east-port delivery proximity
static const GOAL_DELIVERIES = 3;    // bales docked -> win
static const BAIL_AMOUNT    = 25;    // iFlood cut per pump action
static const CREW_ACT_INTERVAL = 35; // frames between boatswain actions
static const SAIL_SPEED     = 8;     // XDir units while under helm way

/* First liquid row at column iX (all-water sea; the water surface). */
global func WaterSurfaceY(int iX) {
	var iY = 0;
	while (iY < LandscapeHeight() - 1 && !GBackLiquid(iX, iY)) iY++;
	return iY;
}

protected func Initialize() {
	SetWind(0); // deterministic sea (StormLane idiom)

	var iW = LandscapeWidth();
	var iGroundY = LandscapeHeight() - 120;

	// Two MKTS ports (StormLane geography): west = producer, east = consumer.
	pWestPost = CreateObject(MKTS, iW / 6, iGroundY, NO_OWNER);
	pEastPost = CreateObject(MKTS, 5 * iW / 6, iGroundY, NO_OWNER);
	if (!pWestPost || !pEastPost)
		FatalError("AdmiralsCrown: MKTS posts not spawned");
	RegisterTradeGood(SILK, pWestPost, 10);
	RegisterTradeGood(SILK, pEastPost, 10);
	// West port restocks SILK (plain-named effect, gotcha #10).
	AddEffect("Produce", pWestPost, 1, 70, pWestPost);

	// The Cog at the west dock (east shore of the west island, x=24% of
	// the map), laden with 5 SILK bales to deliver + 6 WOOD bales for the
	// boatswain's plugs. Spawned below the surface: a floating ship only
	// moves (integrates XDir) while its hull is in liquid; bobbing at the
	// waterline it freezes (engine behavior verified at cycle 213).
	var iDockX = iW * 32 / 100 + 30;
	pCog = CreateObject(CGSH, iDockX, WaterSurfaceY(iDockX) + 40, NO_OWNER);
	if (!pCog)
		FatalError("AdmiralsCrown: CGSH not spawned");
	var i;
	for (i = 0; i < 5; ++i) CreateContents(SILK, pCog);
	for (i = 0; i < 6; ++i) CreateContents(WOOD, pCog);

	// Per-frame sail drive: the director feeds EffectVar(0) the heading
	// (+1 right, -1 left, 0 stop); this interval-1 effect holds the ship's
	// XDir (Wind2Sail's StartCall fires only once per action start, so
	// the Cog never sails on its own).
	var iDrv = AddEffect("Drive", pCog, 1, 1, pCog);
	EffectVar(0, pCog, iDrv) = 0;

	iDelivered = 0;
	iLastFlood = 0;
	iStormIn = STORM_START;
	iBreachIn = BREACH_1;
	iCrewActIn = 0;

	// Director: target-0 global effect (gotcha #4), plain name (#10).
	AddEffect("AdmCrown", 0, 1, 35, 0, 0);

	Log("AdmiralsCrown: director armed (storm %d cad %d; breach %d cad %d; goal %d)",
		STORM_START, STORM_CADENCE, BREACH_1, BREACH_CADENCE, GOAL_DELIVERIES);
	return true;
}

/* ---- Crew placement: both players' crews start at the west dock ---- */
func InitializePlayer(int iPlr, int iX, int iY, object pBase, int iTeam) {
	var iDockX = LandscapeWidth() * 32 / 100 + 30;
	var iDockY = WaterSurfaceY(iDockX) - 45; // on the island top by the dock
	var i;
	for (i = 0; i < GetCrewCount(iPlr); ++i) {
		var pCrew = GetCrew(iPlr, i);
		if (pCrew) SetPosition(iDockX - 40, iDockY - i * 20, pCrew);
	}
	return 1;
}

/* ---- West-port restock (StormLane producer idiom) ---- */
global func FxProduceTimer(object pTarget, int fx) {
	if (ContentsCount(SILK, pTarget) >= 15) return 1;
	CreateContents(SILK, pTarget);
	return 1;
}

/* ---- Per-frame sail drive (interval 1, lives on the Cog) ---- */
/* Holds the ship's XDir toward the helm's heading (EffectVar(0):        */
/* +1 right / -1 left / 0 stop). The Cog's own sailing never starts      */
/* itself (action StartCall fires once), so the director feeds this      */
/* effect the heading the helm mirror derives from player ComDir input.  */
/* Global callback: effects on other objects resolve GLOBAL functions    */
/* (the scenario func is not reachable from the Cog's own script).       */
global func FxDriveTimer(object target, int effect, int timer) {
	var iHdg = EffectVar(0, target, effect);
	if (iHdg)
		SetXDir(iHdg * SAIL_SPEED, target);
	else
		SetXDir(0, target);
	return 1;
}

/* ---- Director: frame-stamped schedule + co-op wiring ---- */
global func FxAdmCrownStart(object target, int effect, int temp) { return 1; }

global func FxAdmCrownTimer(object target, int effect, int timer) {
	var iF = FrameCounter();
	var bAlive = pCog && GetID(pCog) == CGSH;

	// Win: 3 bales delivered at the east port.
	if (iDelivered >= GOAL_DELIVERIES) {
		Log(Format("AdmiralsCrown WIN: %d bales delivered at the east port", iDelivered));
		GameOver();
		return 1;
	}

	// Lose: the Cog foundered (or was destroyed).
	if (!bAlive) {
		// The CWCK wreck owns no flood API; only read the phase from a
		// still-CGSH Cog (calling it on the wreck aborts the callback and
		// GameOver would never fire).
		var iPhase = -1;
		if (pCog && GetID(pCog) == CGSH) iPhase = pCog->GetFloodPhase();
		Log(Format("AdmiralsCrown LOSE: Cog foundered at f%d (flood %d phase %d)",
			iF, iLastFlood, iPhase));
		GameOver();
		return 1;
	}

	// Keep the founder telemetry current while the Cog is still CGSH.
	iLastFlood = pCog->GetFloodLevel();

	// Delivery poll: the Cog with SILK aboard within dock range of the
	// east port. Each poll unloads one bale; 3 unloads -> win.
	if (bAlive && Abs(GetX(pCog) - GetX(pEastPost)) < DOCK_RANGE) {
		var pBale = FindContents(SILK, pCog);
		if (pBale) {
			RemoveObject(pBale);
			++iDelivered;
			Log(Format("AdmiralsCrown: delivery %d of %d at the east port",
				iDelivered, GOAL_DELIVERIES));
		}
	}

	// Storm schedule: STRM on a cadence while the Cog lives, guarded
	// against overlap (StormLaneDirector Script.c:18-23 shape).
	if (iF >= iStormIn) {
		if (GetActiveWeatherEvent() == 0) {
			LaunchWeatherEvent(STRM, 50, STORM_DUR);
			Log(Format("AdmiralsCrown: STRM storm at f%d", iF));
		}
		iStormIn = iF + STORM_CADENCE;
	}

	// Breach schedule: open a hull slot on a cadence while the Cog lives.
	if (iF >= iBreachIn) {
		if (pCog->GetOpenBreaches() < MAX_BREACHES) {
			pCog->OpenBreach();
			Log(Format("AdmiralsCrown: breach %d opened at f%d",
				pCog->GetOpenBreaches(), iF));
		}
		iBreachIn = iF + BREACH_CADENCE;
	}

	// ---- Crew mirror (hot-seat seam; inert with no players joined) ----
	var iPlr;
	for (iPlr = 0; iPlr < GetPlayerCount(); ++iPlr) {
		var i;
		for (i = 0; i < GetCrewCount(iPlr); ++i) {
			var pCrew = GetCrew(iPlr, i);
			if (!pCrew) continue;
			if (Contained(pCrew) != pCog) continue;

			// P1 (helm): mirror walking direction into the Drive effect's
			// heading (the ship only sails while its hull is in liquid;
			// ComDir input alone never moves it -- engine behavior).
			if (iPlr == 0) {
				var iDrv = GetEffect("Drive", pCog);
				if (iDrv) {
					var iDir = GetComDir(pCrew);
					var iHdg = 0;
					if (iDir == COMD_Left) iHdg = -1;
					if (iDir == COMD_Right) iHdg = 1;
					EffectVar(0, pCog, iDrv) = iHdg;
					if (iHdg > 0) SetComDir(COMD_Right, pCog);
					if (iHdg < 0) SetComDir(COMD_Left, pCog);
					if (!iHdg) SetComDir(COMD_None, pCog);
				}
			}

			// P2 (boatswain): damage control aboard, keyed to ComDir.
			if (iPlr == 1 && iF >= iCrewActIn) {
				var iDir = GetComDir(pCrew);
				if (iDir == COMD_Left) {
					pCog->BailOut(BAIL_AMOUNT);
					iCrewActIn = iF + CREW_ACT_INTERVAL;
				}
				if (iDir == COMD_Right && pCog->JettisonCargo())
					iCrewActIn = iF + CREW_ACT_INTERVAL;
				if (iDir == COMD_Up) {
					var pWood = FindContents(WOOD, pCog);
					if (pWood && pCog->PlugBreach()) {
						RemoveObject(pWood);
						iCrewActIn = iF + CREW_ACT_INTERVAL;
					}
				}
			}
		}
	}
	return 1;
}
