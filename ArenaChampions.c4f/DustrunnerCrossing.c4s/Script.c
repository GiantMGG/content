/*-- DustrunnerCrossing: Dustsprint's arena plus a scripted gold-delivery
  prototype layer (cycle 235, roadmap dustrunner-crossing-prototype, task 1:
  the STATIC layer - the bot driver loop is task 2).

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
  design 2.5) lands in the marked placeholder at the bottom. --*/

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

func Initialize() {
  SetWind(0);
  AddEffect("ArenaTeams", 0, 1, 35); // F3: auto-team + place teamless joins
  pBase1 = CreateConstruction(HUT3, 150, 270, NO_OWNER, 100, 1);
  pBase2 = CreateConstruction(HUT3, LandscapeWidth()-150, 270, NO_OWNER, 100, 1);
  g_pChestL = CreateObject(TCHS, 150, 270, NO_OWNER);
  g_pChestR = CreateObject(TCHS, LandscapeWidth()-150, 270, NO_OWNER);
  CreateObject(RSPN, 150, 270, NO_OWNER);
  CreateObject(RSPN, LandscapeWidth()-150, 270, NO_OWNER);
  CreateObject(BNDR, 0, 0, NO_OWNER);
  CreateObject(BNDR, LandscapeWidth(), 0, NO_OWNER);
  CreateObject(BNDR, 0, LandscapeHeight(), NO_OWNER);
  CreateObject(BNDR, LandscapeWidth(), LandscapeHeight(), NO_OWNER);
  // prototype layer: state + kickoffs (seeded gold, void scan, sweep)
  g_drScore = [0, 0, 0];
  g_drArmed = [0, 0, 0];
  g_drEnded = 0;
  g_drScanDone = 0;
  g_drVoidLine = 0;
  g_drScanY = LandscapeHeight() - DRC_ScanRowStep;
  DRCSeedGold();
  AddEffect("DustrunnerScan", 0, 1, 1);
  AddEffect("DustrunnerSweep", 0, 1, 3);
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
          AddEffect("DRCVoidFall", crew, 1, 1);
          Log(Format("DUSTRUNNER CARRIERVOID carrier=%d team=%d x=%d y=%d",
            ObjectNumber(crew), GetPlayerTeam(plr), GetX(crew), GetY(crew)));
        }
    }
  }
  return 1;
}

// The touch IS the delivery (design 2.1): a crew member within
// DRC_DeliverDist px of the ENEMY chest while carrying GOLD hands every
// carried nugget over via script Enter() - which bypasses the chest's
// RejectCollect wall - then banks the team score. Exactly one DEPOSIT line
// per touch; k=1 carriage by design pin means amount=1 on driver touches.

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
      var nug = FindContents(GOLD, crew);
      while (nug)
      {
        Enter(chest, nug);
        moved++;
        nug = FindContents(GOLD, crew);
      }
      if (moved > 0)
      {
        g_drScore[team] = g_drScore[team] + moved;
        Log(Format("DUSTRUNNER DEPOSIT team=%d carrier=%d amount=%d score=%d",
          team, ObjectNumber(crew), moved, g_drScore[team]));
        if (g_drScore[team] >= DRC_WinGold)
        {
          g_drEnded = 1;
          Log(Format("DUSTRUNNER WIN team=%d score=%d-%d",
            team, g_drScore[1], g_drScore[2]));
          GameOver();
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

// ---- prototype layer: TASK 2 DRIVER PLACEHOLDER ---------------------------------
//
// The bot driver loop (LOAD -> CROSS -> DELIVER -> RETURN -> RETRY, design
// 2.5) lands here in task 2: it walks each side's mage to the next seeded
// nugget on the home slab (walkover pickup, carriage k=1), casts the MBRG
// bridge chain across the corridor while holding the lane at or below the
// slab-top line, delivers the nugget at the enemy chest (the 2.1 detector
// banks it), returns over the standing chain, and re-arms after CARRIERVOID
// via the team respawner + a 40-MBRG re-grant. Task 1 ships the static
// layer only - everything the driver needs (seeded gold, spell pools, team
// respawners, detectors, emitters) is in place above this marker.
