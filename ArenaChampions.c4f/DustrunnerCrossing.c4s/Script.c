/*-- DustrunnerCrossing: Dustsprint's arena plus a scripted gold-delivery
  prototype layer (cycle 235, roadmap dustrunner-crossing-prototype, task 1:
  the STATIC layer, task 2: the bot driver).

  Spec: .opencode/design/dustrunner-crossing.md (flash-r3 PASS). Live pieces:

    seeder     8 GOLD per side dropped onto the base BAS4 slab top y=270
               (slab bands x [120,180) left / [1320,1380) right at LW=1500;
               pure script constants - terrain-facts.md "Base platform"),
               walkover-collectible (GOLD Collectible=1), one nugget per
               crossing by design pin;
    grant      200 MBRG spell items into the first-joining player's mage,
               latched once per team (r2 NOTE 4), one GRANT line per side;
    detector   every 3 ticks: crew of team T within 20 px of the ENEMY
               chest while carrying GOLD -> script Enter() each carried
               nugget into the chest (FnEnter passes no reject flag, so the
               TeamChest same-team RejectCollect wall is bypassed), score++,
               DEPOSIT; 5th banked nugget -> WIN + GameOver;
    sweeper    void_line = deepest solid row + 26, derived once by a chunked
               bottom-up GBackSolid scan (step 10 rows / 50 px x, 4 rows per
               poll = 120 calls per script line, well under the ~376/line
               walkprobe constraint, terrain-facts.md:16); every 3 ticks:
               uncontained GOLD below void_line -> VOIDDROP + remove; crew
               below void_line -> CARRIERVOID (latched per crew life via
               the DRCVoidFall effect mark);
    respawner  ONE team-owned RSPN per side at the base (ArenaBotRound.c4s:180
               team-owned pattern; Local(0)=24 charges, Local(1)=3 crew
               threshold), created at the first team join - Dustsprint's
               NO_OWNER pair stays untouched and engine-inert;
    ender      5th banked gold -> WIN + GameOver; tick cap 21000 ->
               TIMEOUT + GameOver (MELE stays as the elimination backstop);
    emitters   every log line DUSTRUNNER-prefixed; ROUNDSTART carries
               seed=199 (the pinned harness seed - GetScenarioVal cannot
               read --parameter Seed back, so it is the DRC_Seed constant),
               the derived voidline, cap and the post-seed world gold
               census (16 expected: 8 per side).

  Dustsprint's flow is kept verbatim (bases/chests/NO_OWNER respawners/
  borders, FxArenaTeams poll, PlacePlayer1/2); the prototype layer is purely
  additive. The bot driver (LOAD -> CROSS -> DELIVER -> RETURN -> RETRY,
  design 2.5) is at the bottom. --*/

#strict

// ---- Dustsprint flow (verbatim) ---------------------------------------------

static pBase1, pBase2;

// ---- prototype layer: statics ------------------------------------------------

static g_pChestL, g_pChestR;  // the two NO_OWNER team chests (delivery targets)
static g_drScore;             // per-team banked gold: g_drScore[team]
static g_drArmed;             // per-team first-join arm latch
static g_drEnded;             // 1 once WIN/TIMEOUT fired
static g_drScanY;             // void scan: next row to probe (bottom-up)
static g_drScanDone;          // 1 once the void scan finished
static g_drVoidLine;          // derived: deepest solid row + DRC_VoidPad
static g_driPlr;              // driver: driven player per team (arm-time latch)
static g_driMage;             // driver: pool-keeper mage per team (stays in the hut)
static g_driCrew;             // driver: tracked carrier object per team
static g_driPhase;            // driver: DRC_PH_* state per team
static g_driAnchor;           // driver: x of the last cast (0 = cast at once)
static g_driLastX;            // driver: previous x (stuck watchdog)
static g_driStuck;            // driver: ticks without x progress
static g_driEntDone;          // driver: CROSSENTRY latch per attempt
static g_driTouchDone;        // driver: CROSSTOUCH latch per attempt
static g_driCarry;            // driver: the nugget this attempt's carrier loaded
static g_driWait;             // driver: multi-tick wait counter

// ---- prototype layer: constants ----------------------------------------------

static const DRC_Seed = 199;          // pinned harness seed (--parameter Seed=)
static const DRC_WinGold = 5;         // banked gold that ends the round
static const DRC_CapTicks = 21000;    // round cap -> TIMEOUT + GameOver
static const DRC_GrantSpells = 200;   // MBRG pool per side (design 2.3)
static const DRC_RspnCharges = 24;    // team respawner Local(0)
static const DRC_RspnThreshold = 3;   // team respawner Local(1)
static const DRC_DeliverDist = 20;    // enemy-chest touch radius (px)
static const DRC_VoidPad = 26;        // void_line = deepest solid + pad
static const DRC_GoldPerSide = 8;     // seeded nuggets per base
static const DRC_SlabLX0 = 120;       // left slab band start x
static const DRC_SeedDropY = 262;     // drop height; nuggets settle to ~268
static const DRC_ScanRowStep = 10;    // void scan row stride (px)
static const DRC_ScanRowsPerPoll = 4; // void scan rows per poll
static const DRC_ScanSampleStep = 50; // void scan x-sample stride (px)
static const DRC_DrvTick = 2;         // driver effect interval (frames)
static const DRC_LaneY = 259;         // crew lane: deck tops land flush at 270
static const DRC_AdvStep = 64;        // px advance per cast (deck far edge >= 67)
static const DRC_StuckLimit = 30;     // driver ticks without x progress -> assist
static const DRC_Regrant = 40;        // retry re-arm pool (design 2.5, r2 NOTE 2)
static const DRC_PastEdge = 182;      // x past the corridor-side slab edge
static const DRC_CastGate = 4;        // px before a goal slab edge casts stop

// driver phases (one state machine per team)
static const DRC_PH_WAIT = 0;         // not armed yet
static const DRC_PH_EXTRACT = 1;      // leave the hut / take the respawn
static const DRC_PH_LOAD = 2;         // walk to a home-slab nugget, pick it up
static const DRC_PH_CROSS = 3;        // cast-walk to the enemy chest
static const DRC_PH_DELIVER = 4;      // at the chest, wait for the 2.1 detector
static const DRC_PH_RETURN = 5;       // cast-walk home; new attempt at LOAD
static const DRC_PH_RETRY = 6;        // carrier lost: wait for the respawner

func Initialize() {
  SetWind(0);
  AddEffect("ArenaTeams", 0, 1, 35); // F3: auto-team + place teamless joins
  pBase1 = CreateConstruction(HUT3, 150, 270, NO_OWNER, 100, 1);
  pBase2 = CreateConstruction(HUT3, LandscapeWidth()-150, 270, NO_OWNER, 100, 1);
  g_pChestL = CreateObject(TCHS, 150, 270, NO_OWNER);
  g_pChestR = CreateObject(TCHS, LandscapeWidth()-150, 270, NO_OWNER);
  CreateObject(BNDR, 0, 0, NO_OWNER);
  CreateObject(BNDR, LandscapeWidth(), 0, NO_OWNER);
  CreateObject(BNDR, 0, LandscapeHeight(), NO_OWNER);
  CreateObject(BNDR, LandscapeWidth(), LandscapeHeight(), NO_OWNER);
  // prototype layer: state + kickoffs (seeded gold, void scan, sweep, driver)
  g_drScore = [0, 0, 0];
  g_drArmed = [0, 0, 0];
  g_drEnded = 0;
  g_drScanDone = 0;
  g_drVoidLine = 0;
  g_drScanY = LandscapeHeight() - DRC_ScanRowStep;
  g_driPlr = [-1, -1, -1];
  g_driMage = [0, 0, 0];
  g_driCrew = [0, 0, 0];
  g_driPhase = [0, 0, 0];
  g_driAnchor = [0, 0, 0];
  g_driLastX = [0, 0, 0];
  g_driStuck = [0, 0, 0];
  g_driEntDone = [0, 0, 0];
  g_driTouchDone = [0, 0, 0];
  g_driCarry = [0, 0, 0];
  g_driWait = [0, 0, 0];
  DRCSeedGold();
  AddEffect("DustrunnerScan", 0, 1, 1);
  AddEffect("DustrunnerSweep", 0, 1, 3);
  AddEffect("DustrunnerDrive", 0, 1, DRC_DrvTick);
  return 1;
}

protected func InitializePlayer(iPlr, x, y, bas, team) {
  // F3 (cycle-172 critic fix-now): with TeamDistribution=Free a join reaches
  // this callback teamless (team=0) BEFORE the team list is compiled - do
  // nothing here; the ArenaTeams poll below assigns a team and re-enters
  // this callback (via InitScenarioPlayer's ~InitializePlayer broadcast)
  // with the chosen team on the first 35-frame tick.
  // Prototype layer: the team-dispatched path also arms the side (team
  // respawner + spell pool + GRANT line), latched once per team.
  if (team == 1) {
    PlacePlayer1(iPlr);
    DRCArmSide(iPlr, 1);
  } else if (team == 2) {
    PlacePlayer2(iPlr);
    DRCArmSide(iPlr, 2);
  }
  return 1;
}

// F3 (cycle-172 critic fix-now): 4 headless/console joins on
// TeamDistribution=Free never resolve their team selection (the engine's
// GetForcedTeamSelection needs exactly ONE joinable team - with two it stays
// 0), so nobody was placed and MELE/KILT never saw two teams: the round
// stalled. This global poll (gotcha #4 form) assigns the SMALLER team
// (tie -> team 1) to every teamless player through the engine's own
// InitScenarioPlayer(plr, team) -> C4Player::ScenarioAndTeamInit -> the
// regular ~InitializePlayer broadcast, so placement + the join logs run in
// the scenario context. Waits for GetTeamCount() > 0: the Teams.txt list is
// compiled only after the first joins landed.
global func FxArenaTeamsTimer(target, effect, time)
{
  if (GetTeamCount() <= 0) return 1;
  var i, done = 0;
  for (i = 0; i < GetPlayerCount(); i++)
  {
    var plr = GetPlayerByIndex(i);
    if (GetPlayerTeam(plr) <= 0)
    {
      InitScenarioPlayer(plr, ArenaSmallerTeam());
      done++;
    }
  }
  if (!done) return -1;   // every joined player has a team: stop polling
  return 1;
}

global func ArenaSmallerTeam()
{
  var c1 = 0, c2 = 0, i;
  for (i = 0; i < GetPlayerCount(); i++)
  {
    var pt = GetPlayerTeam(GetPlayerByIndex(i));
    if (pt == 1) c1++;
    else if (pt == 2) c2++;
  }
  if (c2 < c1) return 2;
  return 1;
}


private func PlacePlayer1(int iPlr) {
  var objs = FindObjects(Find_Category(C4D_Structure), Find_InRect(0, 0, LandscapeWidth()/2, LandscapeHeight()));
  for (var i = GetLength(objs); i > 0; i--)
    if (GetOwner(objs[i-1]) == -1) SetOwner(iPlr, objs[i-1]);
  if (pBase1) {
    for (var i; i < GetCrewCount(iPlr); i++) Enter(pBase1, GetCrew(iPlr, i));
    Enter(pBase1, CreateObject(FLAG, 0, 0, iPlr));
  } else for (var i; i < GetCrewCount(iPlr); i++) SetPosition(150, 270, GetCrew(iPlr, i));
  Log("$TeamLeftJoin$", GetPlayerName(iPlr), Format("$TeamLeft$"));
  return 1;
}

private func PlacePlayer2(int iPlr) {
  var objs = FindObjects(Find_Category(C4D_Structure), Find_InRect(LandscapeWidth()/2, 0, LandscapeWidth(), LandscapeHeight()));
  for (var i = GetLength(objs); i > 0; i--)
    if (GetOwner(objs[i-1]) == -1) SetOwner(iPlr, objs[i-1]);
  if (pBase2) {
    for (var i; i < GetCrewCount(iPlr); i++) Enter(pBase2, GetCrew(iPlr, i));
    Enter(pBase2, CreateObject(FLAG, 0, 0, iPlr));
  } else for (var i; i < GetCrewCount(iPlr); i++) SetPosition(LandscapeWidth()-150, 270, GetCrew(iPlr, i));
  Log("$TeamRightJoin$", GetPlayerName(iPlr), Format("$TeamRight$"));
  return 1;
}

// ---- prototype layer: gold seeder (design 2.2) --------------------------------
//
// 8 nuggets per side at deterministic offsets inside the slab bands; each
// drop starts 8 px above the slab top y=270 so the nuggets settle ON the
// BAS4 slab (terrain-facts.md "Base platform": slab band [270,278), x
// [120,180) left / [1320,1380) right at LW=1500 - script constants,
// seed-independent). Supply 8 vs. win-5 keeps the 3-nugget void-attrition
// buffer; InEarth is inert in this landscape class (census evidence,
// design 2.2).

global func DRCSeedGold()
{
  var offs = [4, 11, 18, 25, 32, 39, 46, 53];
  var i;
  for (i = 0; i < DRC_GoldPerSide; i++)
  {
    var xl = DRC_SlabLX0 + offs[i];
    CreateObject(GOLD, xl, DRC_SeedDropY, NO_OWNER);
    CreateObject(GOLD, LandscapeWidth() - xl, DRC_SeedDropY, NO_OWNER);
  }
  return 1;
}

// ---- prototype layer: void line scan (design 2.4) ------------------------------
//
// void_line = deepest landscape solid row + 26, derived ONCE, bottom-up:
// the first sampled row (step 10) with any solid x-sample (step 50) is the
// deepest solid row. Budget: 4 rows per poll, 30 x-samples per row = 120
// GBackSolid calls per script line (walkprobe constraint ~376/line,
// terrain-facts.md:16). At seed 199 the first hit is the slab row 270
// (dune keels end ~260; nothing solid below the slab band anywhere), so
// void_line = 296 - above every legitimate play object, below nothing the
// loop needs.

global func FxDustrunnerScanTimer(target, effect, time)
{
  var rows = DRC_ScanRowsPerPoll;
  while (rows > 0)
  {
    if (g_drScanY < 0)
    {
      DRCFinishScan(0);   // no solid anywhere: degenerate map, void_line = pad
      return -1;
    }
    if (DRCRowHasSolid(g_drScanY))
    {
      DRCFinishScan(g_drScanY);
      return -1;
    }
    g_drScanY -= DRC_ScanRowStep;
    rows--;
  }
  return 1;
}

global func DRCRowHasSolid(int y)
{
  var x = 0;
  while (x < LandscapeWidth())
  {
    if (GBackSolid(x, y)) return 1;
    x += DRC_ScanSampleStep;
  }
  return 0;
}

global func DRCFinishScan(int row)
{
  g_drVoidLine = row + DRC_VoidPad;
  g_drScanDone = 1;
  var census = GetLength(FindObjects(Find_ID(GOLD)));
  Log(Format("DUSTRUNNER ROUNDSTART seed=%d voidline=%d cap=%d gold=%d",
    DRC_Seed, g_drVoidLine, DRC_CapTicks, census));
  DRCScoreInit();
  return 1;
}

// ---- prototype layer: visible score (design dustrunner-human D2) --------------
//
// Scoreboard, 2 columns x 2 rows, row ids = team ids. The window title
// carries the win condition; the label column (SBRD_Caption) names the
// sides; data column 1 shows banked/DRC_WinGold per side. Populated once
// at ROUNDSTART, updated event-driven only: each DEPOSIT rewrites the
// banking team's cell (DRCDetectDelivery). Exactly ONE DoScoreboardShow
// fires, ever - after a TAB-close nothing re-opens the panel. The opening
// announcement is a plain localized Log, NOT DUSTRUNNER-prefixed, so the
// pinned 32-event DUSTRUNNER sequence survives untouched.

global func DRCScoreInit()
{
  SetScoreboardData(SBRD_Caption, SBRD_Caption, "$MsgSBRDTitle$");
  SetScoreboardData(SBRD_Caption, 1, "$MsgSBRDBanked$");
  SetScoreboardData(1, SBRD_Caption, "$MsgSBRDLeft$");
  SetScoreboardData(2, SBRD_Caption, "$MsgSBRDRight$");
  DRCScoreRefresh(1);
  DRCScoreRefresh(2);
  Log("$MsgAnnounce$");
  DoScoreboardShow(1, 0);
  return 1;
}

global func DRCScoreRefresh(int team)
{
  SetScoreboardData(team, 1, Format("%d/%d", g_drScore[team], DRC_WinGold));
  return 1;
}

// ---- prototype layer: sweep + delivery + cap (design 2.1/2.4/2.6) --------------
//
// Every 3 ticks (gated on the void scan having finished): sweep uncontained
// GOLD below void_line (VOIDDROP + remove), flag crew below void_line
// (CARRIERVOID, latched per crew life), run the delivery detector, and
// enforce the round cap.

global func FxDustrunnerSweepTimer(target, effect, time)
{
  if (!g_drScanDone) return 1;
  DRCSweepGold();
  DRCCheckCarriers();
  DRCDetectDelivery();
  DRCCheckCap();
  return 1;
}

global func DRCSweepGold()
{
  var gold = FindObjects(Find_ID(GOLD), Find_NoContainer());
  var i;
  for (i = 0; i < GetLength(gold); i++)
  {
    var nug = gold[i];
    if (GetY(nug) > g_drVoidLine)
    {
      Log(Format("DUSTRUNNER VOIDDROP gold=1 x=%d y=%d", GetX(nug), GetY(nug)));
      RemoveObject(nug);
    }
  }
  return 1;
}

global func DRCCheckCarriers()
{
  var p, j;
  for (p = 0; p < GetPlayerCount(); p++)
  {
    var plr = GetPlayerByIndex(p);
    var cnt = GetCrewCount(plr);
    for (j = 0; j < cnt; j++)
    {
      var crew = GetCrew(plr, j);
      if (!crew) continue;
      if (GetY(crew) > g_drVoidLine)
        if (!GetEffect("DRCVoidFall", crew))
        {
          // NO interval: an effect carrying an interval but no timer function
          // is killed after its first tick (C4Effect.cpp:398-402) - the
          // interval form never latched and spammed CARRIERVOID
          AddEffect("DRCVoidFall", crew, 1);
          Log(Format("DUSTRUNNER CARRIERVOID carrier=%d team=%d x=%d y=%d",
            ObjectNumber(crew), GetPlayerTeam(plr), GetX(crew), GetY(crew)));
          // void drop is lethal (design 2.4): the engine does NOT kill a
          // void-fallen crew - it comes to rest ALIVE on the map bottom edge
          // (y = LH-10, out-of-bounds death needs y > LH, C4Movement.cpp:610),
          // which would deadlock the respawner refill; kill it so the team
          // respawner can refill (design 2.5 RETRY)
          DoEnergy(-100000, crew);
        }
    }
  }
  return 1;
}

// The touch IS the delivery (design 2.1): a crew member within
// DRC_DeliverDist px of the ENEMY chest while carrying GOLD hands the nugget
// over via script Enter() - which bypasses the chest's RejectCollect wall -
// then banks the team score. The driver tracks the nugget its carrier loaded
// (g_driCarry) and only THAT nugget is banked: the seeded nugget row spans the
// whole delivery slab, so a carrier standing at the chest with empty hands
// would vacuum-collect enemy supply nuggets and cascade 3-4 DEPOSITs per touch
// (task-2 calibration finding). Exactly one DEPOSIT line per touch; k=1
// carriage by design pin means amount=1 on driver touches. Crews without a
// driver-tracked nugget (none exist in this scenario) keep the generic
// deposit-everything path.

global func DRCDetectDelivery()
{
  if (g_drEnded) return 1;
  var p, j;
  for (p = 0; p < GetPlayerCount(); p++)
  {
    var plr = GetPlayerByIndex(p);
    var team = GetPlayerTeam(plr);
    if (team != 1 && team != 2) continue;
    var chest = g_pChestR;                // team 1 delivers into the right base
    if (team == 2) chest = g_pChestL;     // team 2 into the left base
    if (!chest) continue;
    var cnt = GetCrewCount(plr);
    for (j = 0; j < cnt; j++)
    {
      var crew = GetCrew(plr, j);
      if (!crew) continue;
      var dx = GetX(crew) - GetX(chest);
      var dy = GetY(crew) - GetY(chest);
      if (dx*dx + dy*dy > DRC_DeliverDist * DRC_DeliverDist) continue;
      var moved = 0;
      var tracked = g_driCarry[team];
      var nug = FindContents(GOLD, crew);
      if (tracked)
      {
        // driver-driven carrier: bank exactly the loaded nugget (k=1)
        if (nug && nug == tracked)
        {
          Enter(chest, nug);
          g_driCarry[team] = 0;
          moved = 1;
        }
      }
      else
      {
        while (nug)
        {
          Enter(chest, nug);
          moved++;
          nug = FindContents(GOLD, crew);
        }
      }
      if (moved > 0)
      {
        // CROSSTOUCH (design 3): enemy-chest reach, latched per attempt - the
        // driver owns the latch, so the line lands directly before DEPOSIT
        if (!g_driTouchDone[team])
        {
          g_driTouchDone[team] = 1;
          Log(Format("DUSTRUNNER CROSSTOUCH tick=%d carrier=%d team=%d x=%d",
            FrameCounter(), ObjectNumber(crew), team, GetX(crew)));
        }
        g_drScore[team] = g_drScore[team] + moved;
        DRCScoreRefresh(team);
        Log(Format("DUSTRUNNER DEPOSIT team=%d carrier=%d amount=%d score=%d",
          team, ObjectNumber(crew), moved, g_drScore[team]));
        // lift the carrier out of the enemy nugget row at once: the seeded
        // row spans the whole delivery slab, so a carrier standing at the
        // chest with empty hands is walkover-collecting enemy supply within
        // 1-2 engine contact passes (task-2 calibration finding); 30 px back
        // along the return direction lands on the standing bridge deck,
        // clear of the row. Only for driver-tracked carriers.
        if (tracked)
        {
          var back = -30;
          if (team == 2) back = 30;
          SetPosition(GetX(crew) + back, GetY(crew), crew);
        }
        if (g_drScore[team] >= DRC_WinGold)
        {
          g_drEnded = 1;
          Log(Format("DUSTRUNNER WIN team=%d score=%d-%d",
            team, g_drScore[1], g_drScore[2]));
          GameOver();
          // same-tick deposits of the other side must not slip in behind the
          // win (the sweep pass iterates both teams): the 5th bank ends the
          // round at once
          return 1;
        }
      }
    }
  }
  return 1;
}

global func DRCCheckCap()
{
  if (g_drEnded) return 1;
  if (FrameCounter() < DRC_CapTicks) return 1;
  g_drEnded = 1;
  Log(Format("DUSTRUNNER TIMEOUT tick=%d score=%d-%d",
    FrameCounter(), g_drScore[1], g_drScore[2]));
  GameOver();
  return 1;
}

// ---- prototype layer: side arming (design 2.3/2.5) ------------------------------
//
// Runs once per team, on the first team-dispatched InitializePlayer of that
// team: (a) creates the side's team-owned retry respawner (the ship's
// NO_OWNER pair is engine-inert - Respawner.c4d/Script.c Timer early-returns
// without an owner - so the prototype layer owns one RSPN per side with
// 24 charges / 3-crew threshold); (b) grants the 200-MBRG spell pool into
// that player's mage (GrabsteinRallyeRound.c4s:159 CreateContents precedent;
// latched so the second joiner of the team does not double the schema's
// GRANT lines - r2 NOTE 4).

global func DRCArmSide(int iPlr, int team)
{
  if (g_drArmed[team]) return 1;
  g_drArmed[team] = 1;
  // D1 (design dustrunner-human): gate the auto-carrier on the player name.
  // TestPlayer fixtures keep the bot driver (harness gate #218 + the pinned
  // 32-event sequence); any other name arms no driver - that side's humans
  // are the carriers.
  if (SEqual(GetPlayerName(iPlr), "TestPlayer"))
    g_driPlr[team] = iPlr;    // fixture: driver armed exactly as today
  else
    g_driPlr[team] = -1;      // human: no driver on this side
  var bx = 150;
  if (team == 2) bx = LandscapeWidth() - 150;
  var rspn = CreateObject(RSPN, bx, 270, iPlr);
  Local(0, rspn) = DRC_RspnCharges;
  Local(1, rspn) = DRC_RspnThreshold;
  var mage = DRCFindMage(iPlr);
  var k = 0;
  while (k < DRC_GrantSpells)
  {
    CreateContents(MBRG, mage);
    k++;
  }
  Log(Format("DUSTRUNNER GRANT team=%d spells=%d", team, DRC_GrantSpells));
  // the two driven carriers share one bridge lane: carriers meeting
  // mid-corridor lock into Fight and stall the round (task-2 calibration
  // finding), so the driven players are set non-hostile - the race is against
  // the void, not each other; MELE stays as the elimination backstop
  if (g_driPlr[1] >= 0 && g_driPlr[2] >= 0)
  {
    SetHostility(g_driPlr[1], g_driPlr[2], 0, 1);
    SetHostility(g_driPlr[2], g_driPlr[1], 0, 1);
  }
  return 1;
}

global func DRCFindMage(int iPlr)
{
  var j;
  var cnt = GetCrewCount(iPlr);
  for (j = 0; j < cnt; j++)
  {
    var c = GetCrew(iPlr, j);
    if (c && GetID(c) == MCLK) return c;
  }
  return GetCrew(iPlr, 0);
}

// ---- prototype layer: TASK 2 bot driver (design 2.5) ----------------------------
//
// One global effect (gotcha #4 form) ticks every DRC_DrvTick frames and runs a
// small state machine per side through the design 2.5 loop. DESIGN DELTA
// (engine fact found in task 2 calibration): a clonk's script-level inventory
// cap Clonk.c4d/Script.c:759-763 (RejectCollect -> GetNonSpecialCount() >=
// MaxContentsCount(), MaxContentsCount=1) refuses gold into any clonk that
// already holds items - the 200-MBRG pool and the nugget can never share one
// inventory, so each side runs a TWO-CREW RELAY instead of a single mage: the
// pool-keeper mage stays in the hut with the spell pool, a plain clonk is the
// carrier, and every cast relays one spell mage -> carrier via script Enter
// (FnEnter passes no reject flag, C4Script.cpp:367-372) before Activate - the
// bridge spawns relative to the spell item, i.e. at the carrier. k=1 carriage
// (design pin) is untouched; the GRANT pools still live in the mage.
//
//   EXTRACT  leave the home hut (Exit at the slab; the no-coords Exit found in
//            the cycle-235 calibration teleports to (0,0)-clamped map corner -
//            the driver always passes its target coordinates) or take over the
//            respawner's fresh clonk, normalized onto the lane by SetPosition
//            (an allowed driver verb per the same finding);
//   LOAD     walk to the next seeded nugget on the home slab (walkover pickup,
//            engine contact collection, C4GameObjects.cpp:185-193), k=1;
//   CROSS    cast-walk the MBRG chain toward the enemy chest: one bridge per
//            DRC_AdvStep of advance, deck top held level with the slab top
//            (each cast is preceded by a SetPosition lane re-anchor, so every
//            deck lands at the caster's feet line), CROSSENTRY on the first x
//            past the home slab inner edge;
//   DELIVER  stand at the chest; the 2.1 detector banks (CROSSTOUCH + DEPOSIT);
//   RETURN   cast-walk home over the same lane; a completed return starts the
//            next attempt at LOAD (attempt latches reset);
//   RETRY    on carrier loss (fell below void_line / left the crew list): wait
//            for the team respawner's clonk, re-attach, re-arm DRC_Regrant
//            spells with the GRANT respawn=1 line, restart at EXTRACT.
//
// Robustness: a carrier falling off the chain (faded deck, junction miss) is
// caught by an immediate cast from the fall point - the bridge spawns below
// the caster and lands at its feet; a wedged carrier is freed by the stuck
// watchdog (Stuck check -> lift, else re-cast, else nudge forward). A pool
// exhausted mid-corridor ends in the designed void drop + retry cycle.

global func FxDustrunnerDriveTimer(target, effect, time)
{
  if (g_drEnded) return -1;
  DRCDriveTeam(1);
  DRCDriveTeam(2);
  return 1;
}

// D1 (cycle 245, spec input-path-truth): driver-phase transition log —
// log-only, behavior-identical.
global func DRCSetPhase(int team, int new_phase)
{
  var old_phase = g_driPhase[team];
  g_driPhase[team] = new_phase;
  if (old_phase != new_phase)
    Log(Format("DUSTRUNNER PHASE team=%d old=%d new=%d", team, old_phase, new_phase));
  return 1;
}

global func DRCDriveTeam(int team)
{
  var phase = g_driPhase[team];
  // carrier-loss check on every live phase (design 2.5 RETRY trigger)
  if (phase != DRC_PH_WAIT && phase != DRC_PH_RETRY)
    if (!DRCCarrierOK(team))
    {
      DRCSetPhase(team, DRC_PH_RETRY);
      g_driWait[team] = 0;
      phase = DRC_PH_RETRY;
    }
  if (phase == DRC_PH_WAIT) return DRCDriveWait(team);
  if (phase == DRC_PH_EXTRACT) return DRCDriveExtract(team);
  if (phase == DRC_PH_LOAD) return DRCDriveLoad(team);
  if (phase == DRC_PH_CROSS) return DRCDriveCross(team);
  if (phase == DRC_PH_DELIVER) return DRCDriveDeliver(team);
  if (phase == DRC_PH_RETURN) return DRCDriveReturn(team);
  if (phase == DRC_PH_RETRY) return DRCDriveRetry(team);
  return 1;
}

// the tracked carrier is alive, still on its player's crew, and not doomed
global func DRCCarrierOK(int team)
{
  var crew = g_driCrew[team];
  if (!crew) return 0;
  if (!DRCCrewOf(g_driPlr[team], crew)) return 0;
  if (g_drScanDone)
    if (GetY(crew) > g_drVoidLine) return 0;
  return 1;
}

global func DRCCrewOf(int plr, object crew)
{
  if (plr < 0) return 0;
  var j, cnt = GetCrewCount(plr);
  for (j = 0; j < cnt; j++)
    if (GetCrew(plr, j) == crew) return 1;
  return 0;
}

global func DRCHomeX(int team)
{
  if (team == 1) return 140;
  return LandscapeWidth() - 140;
}

global func DRCDriveWait(int team)
{
  if (!g_drArmed[team]) return 1;
  var plr = g_driPlr[team];
  if (plr < 0) return 1;
  // two-crew relay (Clonk.c4d item cap: MaxContentsCount=1, RejectCollect
  // refuses gold into any clonk already holding items - the spell pool can
  // never share an inventory with the nugget): the mage keeps the 200-MBRG
  // pool inside the hut, a plain clonk is the carrier; each cast relays one
  // spell mage -> carrier via script Enter (bypasses the reject wall)
  var mage = DRCFindMage(plr);
  if (!mage) return 1;
  var carrier = DRCFindCarrier(plr, mage);
  if (!carrier) return 1;
  g_driMage[team] = mage;
  g_driCrew[team] = carrier;
  g_driWait[team] = 0;
  DRCSetPhase(team, DRC_PH_EXTRACT);
  return 1;
}

// first live crew member of <plr> that is not the pool-keeper mage
global func DRCFindCarrier(int plr, object mage)
{
  var j, cnt = GetCrewCount(plr);
  for (j = 0; j < cnt; j++)
  {
    var c = GetCrew(plr, j);
    if (c && c != mage) return c;
  }
  return 0;
}

global func DRCDriveExtract(int team)
{
  var crew = g_driCrew[team];
  g_driWait[team]++;
  if (g_driWait[team] == 1)
  {
    // proper-coords Exit: no-coords Exit clamps to the map corner (calib finding)
    if (Contained(crew)) Exit(crew, DRCHomeX(team), 270);
    return 1;
  }
  // normalize onto the lane (also frees a respawner clonk stuck in the slab)
  SetPosition(DRCHomeX(team), DRC_LaneY, crew);
  if (g_driWait[team] >= 3)
  {
    g_driWait[team] = 0;
    DRCResetAttempt(team);
    DRCSetPhase(team, DRC_PH_LOAD);
  }
  return 1;
}

global func DRCResetAttempt(int team)
{
  g_driEntDone[team] = 0;
  g_driTouchDone[team] = 0;
  g_driCarry[team] = 0;
  g_driAnchor[team] = 0;
  g_driLastX[team] = 0;
  g_driStuck[team] = 0;
  g_driWait[team] = 0;
  return 1;
}

global func DRCDriveLoad(int team)
{
  var crew = g_driCrew[team];
  if (ContentsCount(GOLD, crew) > 0)
  {
    // track the loaded nugget (or adopt one the carrier brought home - a
    // nugget walkover-collected off the delivery slab after a deposit);
    // only the tracked nugget is banked by the 2.1 detector
    if (!g_driCarry[team]) g_driCarry[team] = FindContents(GOLD, crew);
    SetComDir(COMD_Stop, crew);
    g_driAnchor[team] = 0;   // first cast fires at once: the deck overlaps the slab
    g_driLastX[team] = GetX(crew);
    g_driStuck[team] = 0;
    DRCSetPhase(team, DRC_PH_CROSS);
    return 1;
  }
  var nug = DRCNextNugget(team, crew);
  if (!nug)
  {
    SetComDir(COMD_Stop, crew);
    return 1;   // slab stripped: idle (cap or the other side ends the round)
  }
  var dx = GetX(nug) - GetX(crew);
  if (Abs(dx) <= 2)
  {
    // at the nugget: walkover collection needs one contact tick
    g_driWait[team]++;
    SetComDir(COMD_Stop, crew);
    if (g_driWait[team] > 20)
    {
      SetPosition(GetX(nug), GetY(crew), crew);
      g_driWait[team] = 0;
    }
    return 1;
  }
  g_driWait[team] = 0;
  DRCStuckCheck(team, crew);
  if (g_driStuck[team] >= DRC_StuckLimit)
  {
    SetPosition(GetX(nug), DRC_LaneY, crew);
    g_driStuck[team] = 0;
    return 1;
  }
  if (dx > 0) SetComDir(COMD_Right, crew);
  else SetComDir(COMD_Left, crew);
  return 1;
}

// nearest uncontained seeded nugget on the home slab band
global func DRCNextNugget(int team, object crew)
{
  var x0 = DRC_SlabLX0;
  if (team == 2) x0 = LandscapeWidth() - 180;
  var gold = FindObjects(Find_ID(GOLD), Find_NoContainer(),
    Find_InRect(x0, 250, 60, 40));
  var best, bd, i;
  for (i = 0; i < GetLength(gold); i++)
  {
    var d = Abs(GetX(gold[i]) - GetX(crew));
    if (!best || d < bd) { best = gold[i]; bd = d; }
  }
  return best;
}

global func DRCDriveCross(int team)
{
  var crew = g_driCrew[team];
  // the 2.1 detector may already have banked and lifted the carrier out of
  // the deliver radius (post-deposit reposition) - the driver, still in
  // CROSS, must not walk back into the enemy nugget row: an empty carrier
  // with nothing tracked means the delivery happened -> RETURN
  if (ContentsCount(GOLD, crew) == 0 && !g_driCarry[team])
  {
    g_driLastX[team] = GetX(crew);
    g_driStuck[team] = 0;
    g_driAnchor[team] = GetX(crew);
    DRCSetPhase(team, DRC_PH_RETURN);
    return 1;
  }
  // CROSSENTRY: first tick past the home slab inner edge (latched per attempt)
  if (!g_driEntDone[team])
  {
    var past = 0;
    if (team == 1 && GetX(crew) >= DRC_PastEdge) past = 1;
    if (team == 2 && GetX(crew) <= LandscapeWidth() - DRC_PastEdge) past = 1;
    if (past)
    {
      g_driEntDone[team] = 1;
      Log(Format("DUSTRUNNER CROSSENTRY tick=%d carrier=%d team=%d x=%d",
        FrameCounter(), ObjectNumber(crew), team, GetX(crew)));
    }
  }
  var chest = g_pChestR;
  if (team == 2) chest = g_pChestL;
  if (!chest) return 1;
  var dx = GetX(chest) - GetX(crew);
  var dy = GetY(chest) - GetY(crew);
  if (dx*dx + dy*dy <= DRC_DeliverDist * DRC_DeliverDist)
  {
    SetComDir(COMD_Stop, crew);
    // CROSSTOUCH (design 3): enemy-chest reach = cross end, latched per
    // attempt - emitted here, driver-side, so the Task 1 detector stays
    // byte-identical to the committed static layer
    if (!g_driTouchDone[team])
    {
      g_driTouchDone[team] = 1;
      Log(Format("DUSTRUNNER CROSSTOUCH tick=%d carrier=%d team=%d x=%d",
        FrameCounter(), ObjectNumber(crew), team, GetX(crew)));
    }
    DRCSetPhase(team, DRC_PH_DELIVER);
    return 1;
  }
  var dir = 1, edge = 1320;
  if (team == 2) { dir = -1; edge = 180; }
  DRCAdvance(team, crew, dir, GetX(chest), edge);
  return 1;
}

global func DRCDriveDeliver(int team)
{
  var crew = g_driCrew[team];
  SetComDir(COMD_Stop, crew);
  if (ContentsCount(GOLD, crew) == 0)
  {
    // the 2.1 detector banked it: turn around and walk home
    g_driLastX[team] = GetX(crew);
    g_driStuck[team] = 0;
    g_driAnchor[team] = GetX(crew);
    DRCSetPhase(team, DRC_PH_RETURN);
  }
  return 1;
}

global func DRCDriveReturn(int team)
{
  var crew = g_driCrew[team];
  var x = GetX(crew);
  var inBand = 0;
  if (team == 1 && x <= DRC_PastEdge) inBand = 1;
  if (team == 2 && x >= LandscapeWidth() - DRC_PastEdge) inBand = 1;
  if (inBand)
  {
    SetComDir(COMD_Stop, crew);
    DRCResetAttempt(team);
    DRCSetPhase(team, DRC_PH_LOAD);   // attempt complete: LOAD the next nugget
    return 1;
  }
  var dir = -1, edge = 180;
  if (team == 2) { dir = 1; edge = 1320; }
  DRCAdvance(team, crew, dir, DRCHomeX(team), edge);
  return 1;
}

// The cast-walk advance primitive shared by CROSS and RETURN. <dir> is the
// walking direction (+1 right / -1 left), <goalX> the walk target, <edgeX> the
// corridor-side edge of the goal slab (casts stop DRC_CastGate px before it -
// the last deck already overlaps the slab, casting on it would be waste).
global func DRCAdvance(int team, object crew, int dir, int goalX, int edgeX)
{
  var x = GetX(crew);
  DRCStuckCheck(team, crew);
  // falling: patch the gap with a lane-anchored cast (the bridge lands at the
  // caster's feet line and the carrier drops onto it)
  if (GetContact(crew, -1, 8) == 0 && GetY(crew) > DRC_LaneY + 3)
  {
    DRCCast(team, crew, dir, 1);
    return 1;
  }
  if (g_driStuck[team] >= DRC_StuckLimit)
  {
    g_driStuck[team] = 0;
    if (Stuck(crew)) SetPosition(x, GetY(crew) - 6, crew);
    else if (!DRCCast(team, crew, dir, 1))
      SetPosition(x + 6 * dir, DRC_LaneY, crew);
    return 1;
  }
  // cast cadence: one bridge per DRC_AdvStep of advance (bridge-to-bridge);
  // casts stop DRC_CastGate px before the goal slab edge (the last deck
  // already overlaps the slab - casting on it would be waste)
  var reach = (x - g_driAnchor[team]) * dir;
  var gateOpen = (edgeX - x) * dir > DRC_CastGate;
  if (gateOpen)
    if (g_driAnchor[team] == 0 || reach >= DRC_AdvStep)
      DRCCast(team, crew, dir, 1);
  // walk toward the goal
  if ((goalX - x) * dir > 2)
  {
    if (dir > 0) SetComDir(COMD_Right, crew);
    else SetComDir(COMD_Left, crew);
  }
  else SetComDir(COMD_Stop, crew);
  // lane hold: correct small standing drift so every deck lands level
  if (GetContact(crew, -1, 8))
    if (Abs(GetY(crew) - DRC_LaneY) > 7)
      SetPosition(GetX(crew), DRC_LaneY, crew);
  return 1;
}

global func DRCStuckCheck(int team, object crew)
{
  var x = GetX(crew);
  if (Abs(x - g_driLastX[team]) <= 1) g_driStuck[team]++;
  else { g_driStuck[team] = 0; g_driLastX[team] = x; }
  return 1;
}

// Cast one bridge at the carrier: relay one MBRG from the pool-keeper mage
// into the carrier via script Enter (FnEnter passes no reject flag, so the
// clonk's MaxContentsCount wall is bypassed), then call its Activate - the
// spell self-consumes and the bridge spawns relative to the item, i.e. at the
// carrier. <fReanchor>: SetPosition the caster onto the lane first so the deck
// top lands level with the slab top (also used to catch a falling carrier: it
// is lifted onto the lane and drops onto the fresh bridge).
global func DRCCast(int team, object crew, int dir, int fReanchor)
{
  var mage = g_driMage[team];
  if (!mage || !DRCCrewOf(g_driPlr[team], mage))
  {
    mage = DRCFindMage(g_driPlr[team]);
    if (!mage) return 0;
    g_driMage[team] = mage;
  }
  var it = FindContents(MBRG, mage);
  if (!it) return 0;
  if (fReanchor)
    SetPosition(GetX(crew), DRC_LaneY, crew);
  if (dir > 0) SetDir(DIR_Right(), crew);
  else SetDir(DIR_Left(), crew);
  Enter(crew, it);
  it->Activate(crew);
  g_driAnchor[team] = GetX(crew);
  return 1;
}

global func DRCDriveRetry(int team)
{
  var plr = g_driPlr[team];
  if (plr < 0) return 1;
  // carrier gone: wait for the team respawner's clonk (uncontained = the spawn)
  var newcrew = DRCNewCrew(plr, g_driCrew[team]);
  if (!newcrew) return 1;
  g_driCrew[team] = newcrew;
  // re-arm the spell pool (r2 NOTE 2) in the pool-keeper mage + schema line
  var mage = g_driMage[team];
  if (!mage || !DRCCrewOf(plr, mage))
  {
    mage = DRCFindMage(plr);
    g_driMage[team] = mage;
  }
  if (mage)
  {
    var k = 0;
    while (k < DRC_Regrant)
    {
      CreateContents(MBRG, mage);
      k++;
    }
    Log(Format("DUSTRUNNER GRANT team=%d spells=%d respawn=1", team, DRC_Regrant));
  }
  g_driWait[team] = 0;
  DRCSetPhase(team, DRC_PH_EXTRACT);
  return 1;
}

// first live uncontained crew member of <plr> that is not the dying carrier
global func DRCNewCrew(int plr, object old)
{
  var j, cnt = GetCrewCount(plr);
  for (j = 0; j < cnt; j++)
  {
    var c = GetCrew(plr, j);
    if (c && c != old && !Contained(c)) return c;
  }
  return 0;
}
