#strict

static pBase1, pBase2;

func Initialize() {
  SetWind(0);
  AddEffect("ArenaTeams", 0, 1, 35); // F3: auto-team + place teamless joins
  // cycle 185: vein pulse over the live-scanned gold bbox
  VeinScanBranches();
  gVeinPhase = 0; gVeinTick = 0; gVeinBranch = Random(3);
  AddEffect("VeinPulse", 0, 1, 35);
  Log(Format("$MsgVeinIgnite$", gVeinBranch + 1));
  var cx = LandscapeWidth()/2;
  var cy = LandscapeHeight()/2;
  pBase1 = CreateConstruction(HUT3, cx - 300, cy - 200, NO_OWNER, 100, 1);
  pBase2 = CreateConstruction(HUT3, cx + 300, cy + 200, NO_OWNER, 100, 1);
  CreateObject(TCHS, cx - 300, cy - 200, NO_OWNER);
  CreateObject(TCHS, cx + 300, cy + 200, NO_OWNER);
  CreateObject(RSPN, cx - 300, cy - 200, NO_OWNER);
  CreateObject(RSPN, cx + 300, cy + 200, NO_OWNER);
  CreateObject(BNDR, 0, 0, NO_OWNER);
  CreateObject(BNDR, LandscapeWidth(), 0, NO_OWNER);
  CreateObject(BNDR, 0, LandscapeHeight(), NO_OWNER);
  CreateObject(BNDR, LandscapeWidth(), LandscapeHeight(), NO_OWNER);
  return 1;
}

protected func InitializePlayer(iPlr, x, y, bas, team) {
  // F3 (cycle-172 critic fix-now): with TeamDistribution=Free a join reaches
  // this callback teamless (team=0) BEFORE the team list is compiled - do
  // nothing here; the ArenaTeams poll below assigns a team and re-enters
  // this callback (via InitScenarioPlayer's ~InitializePlayer broadcast)
  // with the chosen team on the first 35-frame tick.
  if (team == 1) PlacePlayer1(iPlr);
  else if (team == 2) PlacePlayer2(iPlr);
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
  if (!done)
  {
    if (GetPlayerCount() > 0)
      Log(Format("$MsgVeinTeams$", GetTeamName(1), GetTeamName(2)));
    return -1;   // every joined player has a team: stop polling
  }
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
  } else for (var i; i < GetCrewCount(iPlr); i++) SetPosition(LandscapeWidth()/2 - 300, LandscapeHeight()/2 - 200, GetCrew(iPlr, i));
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
  } else for (var i; i < GetCrewCount(iPlr); i++) SetPosition(LandscapeWidth()/2 + 300, LandscapeHeight()/2 + 200, GetCrew(iPlr, i));
  Log("$TeamRightJoin$", GetPlayerName(iPlr), Format("$TeamRight$"));
  return 1;
}

// --- Vein pulse (cycle 185) -----------------------------------------------
// State: phase 0 burn / 1 cool; branch 0..2 = vertical third of the gold
// bbox; 35-frame timer ticks. Nuggets freed from the burning branch are
// flagged VeinRich (seared) and bank double in the Team Chest (S2).
static gVeinPhase, gVeinBranch, gVeinTick, gVeinRects;

global func VeinPulsePhase() { return gVeinPhase; }
global func VeinPulseBranch() { return gVeinBranch; }

// One-time gold scan (grid step 3) -> bbox -> three branch rects. No gold
// (degenerate seed): fall back to a map-centered 300x300 rect.
private func VeinScanBranches()
{
	var minX = -1, minY = -1, maxX = -1, maxY = -1;
	var x, y;
	for (x = 0; x < LandscapeWidth(); x += 3)
		for (y = 0; y < LandscapeHeight(); y += 3)
			if (GetMaterial(x, y) == Material("Gold"))
			{
				if (minX < 0 || x < minX) minX = x;
				if (minY < 0 || y < minY) minY = y;
				if (maxX < 0 || x > maxX) maxX = x;
				if (maxY < 0 || y > maxY) maxY = y;
			}
	if (minX < 0)
	{
		Log("$MsgVeinNoGold$");
		minX = LandscapeWidth()/2 - 150; maxX = minX + 299;
		minY = LandscapeHeight()/2 - 150; maxY = minY + 299;
	}
	var bx = minX, by = minY, bw = maxX - minX + 1, bh = maxY - minY + 1;
	var third = bh / 3;
	gVeinRects = [
		bx, by,             bw, third,
		bx, by + third,     bw, third,
		bx, by + 2*third,   bw, bh - 2*third ];
	return 1;
}

global func FxVeinPulseTimer(target, effect, time)
{
	++gVeinTick;
	if (gVeinPhase == 0)
	{
		if (gVeinTick > 20)   // 20 x 35f = 700 frames of burn
		{
			gVeinPhase = 1; gVeinTick = 0;
			Log(Format("$MsgVeinCool$", gVeinBranch + 1));
			return 1;
		}
		VeinBurnBranch(gVeinBranch);
	}
	else if (gVeinTick > 10)  // 10 x 35f = 350 frames of cool
	{
		gVeinPhase = 0; gVeinTick = 0;
		gVeinBranch = Random(3);
		Log(Format("$MsgVeinIgnite$", gVeinBranch + 1));
	}
	return 1;
}

// One burn tick on branch b: damage + ignite in-branch clonks; sear
// in-branch free GOLD. Damage 40/tick: ~3 consecutive ticks (~3 s) lethal.
// NOTE (cycle 185): must be global - a global effect timer resolves calls
// through the ENGINE's global func table only; a private (script-local)
// callee parses as "unknown identifier" (verified on 2026-10-01, boot
// errors at Script.c:159). The mirror smoke declares MirrorBurnBranch
// global for the same reason.
global func VeinBurnBranch(int b)
{
	var rx = gVeinRects[b*4 + 0], ry = gVeinRects[b*4 + 1];
	var rw = gVeinRects[b*4 + 2], rh = gVeinRects[b*4 + 3];
	var i, obj;
	var clonks = FindObjects(Find_InRect(rx, ry, rw, rh), Find_OCF(OCF_Alive), Find_NoContainer());
	for (i = 0; i < GetLength(clonks); i++)
	{
		obj = clonks[i];
		// DoEnergy (-40/tick) is the energy-damage path (arrow precedent).
		// DoDamage only accumulates the Damage var + fires ~Damage, which
		// CLNK does not define, and CLNK DefCore NoBurnDamage=1/NoBurnDecay=1
		// neuter the fire's own damage - verified on 2026-10-01 that the
		// pulse was inert against clonks with DoDamage (smoke caught it).
		obj->DoEnergy(-40);
		obj->Incinerate();
		CastParticles("MSpark", 10, 25, GetX(obj), GetY(obj), 15, 15);
	}
	var gold = FindObjects(Find_ID(GOLD), Find_InRect(rx, ry, rw, rh), Find_NoContainer());
	for (i = 0; i < GetLength(gold); i++)
	{
		obj = gold[i];
		if (!GetEffect("VeinRich", obj))
		{
			AddEffect("VeinRich", obj, 2);
			Log(Format("$MsgVeinSeared$", b + 1, FrameCounter()));
		}
	}
	return 1;
}
