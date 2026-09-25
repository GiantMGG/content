/*-- ArenaBotRound: scripted 4-player bot round on the arena format.

     Four TestPlayer fixtures join two Random teams (2+2, Teams.txt).
     The script plays a whole first-to-8-kills round:
       - a team-1 fighter clonk (created + made crew) stages crew kills
         against the team-2 defender, one per 35-frame step;
       - every kill is credited to team 1 on the KILT goal (KillHook
         #appendto CLNK wiring) and asserted synchronously, with team 2
         never leaking a score (no friendly fire);
       - after the third kill the defender's crew is gone below the RSPN
         threshold, so the respawner (team-wide refill) visibly burns two
         of its three charges and tops the defender back up — the
         in-round respawn proof;
       - kills 4-8 finish the round: the 8th (deciding) kill lands on a
         fresh NON-crew team-2 clonk precisely so no player ever drops to
         0 crew — the pool contract (2 players x 3 crew + 3 charges = 9)
         nets only 7 avoid-elimination real-crew kills;
       - the KILT goal reaches its target -> IsFulfilled -> PASS log;
         MELE never fires (all four players stay active).

     Permanent gate for "a scripted 4-player bot round on the arena
     finishes" (cycle 172 arena-rotation). NO "Smoke" suffix in the
     scenario name: the Tests.c4f glob must not double-register it. --*/

#strict 2

static g_Step;
static g_Phase;      // 0 setup, 1 kills 1-3, 2 respawn wait, 3 kills 4-8, 4 final
static g_Kills;      // kills landed so far (1..8)
static g_Ticks;      // poll counter for the bounded respawn wait
static g_Goal;       // the KILT goal object
static g_Melee;      // the MELE goal object (must stay dormant)
static g_Rspn;       // team-2 respawner (serves the defender)
static g_Killer;     // team-1 fighter clonk that lands the kills
static g_A, g_A2;    // team-1 players (attacker + partner)
static g_D, g_D2;    // team-2 players (defender + partner)
static g_T1, g_T2;   // team ids
static g_D2Crew0;    // defender partner's initial crew count
static g_TCrew0;     // defender's initial crew count

protected func Initialize()
{
	g_Step = 0;
	g_Phase = 0;
	g_Kills = 0;
	g_Ticks = 0;
	AddEffect("RunTest", 0, 1, 35);
	return true;
}

global func FxRunTestTimer(target, effect, time)
{
	++g_Step;
	if (g_Phase == 0) return StepSetup();
	if (g_Phase == 1) return StepKillEarly();
	if (g_Phase == 2) return StepRespawnWait();
	if (g_Phase == 3) return StepKillLate();
	if (g_Phase == 4) return StepFinal();
	return 1;
}

global func StepSetup()
{
	var i;
	var p0 = GetPlayerByIndex(0);
	var p1 = GetPlayerByIndex(1);
	var p2 = GetPlayerByIndex(2);
	var p3 = GetPlayerByIndex(3);
	// 1) four players, every one on a team (Random 2+2 by Teams.txt)
	if (GetPlayerCount() != 4)
		FatalError(Format("ArenaBotRound FAIL: expected 4 players, got %d", GetPlayerCount()));
	if (GetPlayerTeam(p0) <= 0 || GetPlayerTeam(p1) <= 0 ||
	    GetPlayerTeam(p2) <= 0 || GetPlayerTeam(p3) <= 0)
		FatalError("ArenaBotRound FAIL: a player is on no team (Teams.txt TeamDistribution=Random)");
	g_T1 = GetPlayerTeam(p0);
	g_A = p0;
	// split the remaining three into A's partner / the two team-2 players;
	// locals only - statics must not leak across a StepSetup() retry after
	// an earlier FatalError aborts before g_Phase advances
	var A2 = 0, D = 0, D2 = 0;
	var rest = [p1, p2, p3];
	var restTeam = [GetPlayerTeam(p1), GetPlayerTeam(p2), GetPlayerTeam(p3)];
	for (i = 0; i < 3; ++i)
	{
		if (restTeam[i] == g_T1)
		{
			if (A2) FatalError("ArenaBotRound FAIL: more than two players on team 1");
			A2 = rest[i];
		}
		else
		{
			if (D2) FatalError("ArenaBotRound FAIL: more than two players on team 2");
			if (!D) D = rest[i];
			else D2 = rest[i];
		}
	}
	if (!A2 || !D || !D2)
		FatalError("ArenaBotRound FAIL: teams not split 2+2");
	g_A2 = A2;
	g_D = D;
	g_D2 = D2;
	g_T2 = GetPlayerTeam(g_D);
	if (g_T2 != GetPlayerTeam(g_D2) || g_D2 == g_D)
		FatalError("ArenaBotRound FAIL: team 2 split across teams");
	// 2) KILT goal exists, unfulfilled, target 8; MELE exists and is dormant
	g_Goal = FindObject(KILT);
	if (!g_Goal)
		FatalError("ArenaBotRound FAIL: no KILT goal object (Goals=MELE=1;KILT=1)");
	if (Local(0, g_Goal) != 8)
		FatalError(Format("ArenaBotRound FAIL: KILT target local is %d, expected 8", Local(0, g_Goal)));
	if (g_Goal->~IsFulfilled())
		FatalError("ArenaBotRound FAIL: KILT fulfilled at round start");
	g_Melee = FindObject(MELE);
	if (!g_Melee)
		FatalError("ArenaBotRound FAIL: no MELE goal object");
	if (g_Melee->~IsFulfilled())
		FatalError("ArenaBotRound FAIL: MELE fulfilled at round start");
	// 3) all five arena sounds must resolve
	var s1 = SoundExists("KillPing");
	var s2 = SoundExists("RespawnChime");
	var s3 = SoundExists("ChestOpen");
	var s4 = SoundExists("TideAlert");
	var s5 = SoundExists("VictorySting");
	if (!(s1 && s2 && s3 && s4 && s5))
		FatalError(Format("ArenaBotRound FAIL: sounds %d %d %d %d %d (KillPing RespawnChime ChestOpen TideAlert VictorySting)", s1, s2, s3, s4, s5));
	// 4) crew pool contract: every player carries the section crew (3+)
	var aC = GetCrewCount(g_A);
	var a2C = GetCrewCount(g_A2);
	g_TCrew0 = GetCrewCount(g_D);
	g_D2Crew0 = GetCrewCount(g_D2);
	if (aC < 3 || a2C < 3 || g_TCrew0 < 3 || g_D2Crew0 < 3)
		FatalError(Format("ArenaBotRound FAIL: crew pool %d %d %d %d too small", aC, a2C, g_TCrew0, g_D2Crew0));
	// 5) fighter, respawner + map flavor objects
	g_Killer = CreateObject(CLNK, 20, 8, g_A);
	if (!g_Killer)
		FatalError("ArenaBotRound FAIL: could not create fighter clonk");
	MakeCrewMember(g_Killer, g_A);
	g_Rspn = CreateObject(RSPN, 60, 20, g_D);
	if (!g_Rspn)
		FatalError("ArenaBotRound FAIL: could not create RSPN");
	if (Local(0, g_Rspn) != 3 || Local(1, g_Rspn) != 2)
		FatalError(Format("ArenaBotRound FAIL: RSPN charges %d threshold %d", Local(0, g_Rspn), Local(1, g_Rspn)));
	if (GetOwner(g_Rspn) != g_D)
		FatalError(Format("ArenaBotRound FAIL: RSPN owned by %d, expected %d", GetOwner(g_Rspn), g_D));
	var chest = CreateObject(TCHS, 50, 20, g_D);
	var flag = CreateObject(FLAG, 45, 20, g_D);
	Log(Format("ArenaBotRound setup: teams %d/%d A=%d D=%d", g_T1, g_T2, g_A, g_D));
	Log(Format("ArenaBotRound crew: %d %d %d %d chest=%d flag=%d", aC, a2C, g_TCrew0, g_D2Crew0, chest != 0, flag != 0));
	g_Phase = 1;
	return 1;
}

// one defender crew kill per step; phase flips to the respawn wait on the 4th
global func StepKillEarly()
{
	if (g_Kills >= 3)
	{
		g_Phase = 2;
		g_Ticks = 0;
		return 1;
	}
	++g_Kills;
	StageCrewKill(g_D);
	return 1;
}

// bounded poll for the mid-round respawn: the defender must be topped back
// up to the RSPN threshold and exactly two charges burned (kill 2 and 3
// each dropped the defender below the threshold)
global func StepRespawnWait()
{
	++g_Ticks;
	var crew = GetCrewCount(g_D);
	var charges = Local(0, g_Rspn);
	if (crew >= 2 && charges == 1)
	{
		if (GetCrewCount(g_D2) != g_D2Crew0)
			FatalError(Format("ArenaBotRound FAIL: defender partner crew %d -> %d", g_D2Crew0, GetCrewCount(g_D2)));
		Log(Format("ArenaBotRound respawn proven: D crew %d charges %d", crew, charges));
		g_Phase = 3;
		return 1;
	}
	if (g_Ticks >= 13)
		FatalError(Format("ArenaBotRound FAIL: no respawn within %d ticks (D crew %d charges %d)", g_Ticks * 35, crew, charges));
	return 1;
}

// kills 4-8: defender twice more (charges empty thereafter), partner twice,
// then the deciding non-crew kill that keeps everyone at >= 1 crew
global func StepKillLate()
{
	if (g_Kills >= 8)
	{
		g_Phase = 4;
		return 1;
	}
	++g_Kills;
	if (g_Kills <= 5) StageCrewKill(g_D);
	else if (g_Kills <= 7) StageCrewKill(g_D2);
	else StageFinalKill();
	return 1;
}

// kill a real crew member of <plr> via the team-1 fighter; assert the KILT
// team-score locals right after (Death is synchronous with Kill)
global func StageCrewKill(int plr)
{
	var victim = GetCrew(plr, 0);
	if (!victim)
		FatalError(Format("ArenaBotRound FAIL: kill %d - player %d has no crew", g_Kills, plr));
	g_Killer->Kill(victim);
	if (Local(g_T1, g_Goal) != g_Kills)
		FatalError(Format("ArenaBotRound FAIL: after kill %d team %d score %d", g_Kills, g_T1, Local(g_T1, g_Goal)));
	if (Local(g_T2, g_Goal) != 0)
		FatalError(Format("ArenaBotRound FAIL: team %d scored illegally %d", g_T2, Local(g_T2, g_Goal)));
	if (GetCrewCount(plr) < 1)
		FatalError(Format("ArenaBotRound FAIL: kill %d eliminated player %d", g_Kills, plr));
	if (GetPlayerCount() != 4)
		FatalError(Format("ArenaBotRound FAIL: player lost after kill %d (count %d)", g_Kills, GetPlayerCount()));
	return 1;
}

// the deciding kill: a fresh non-crew team-2 clonk must still credit team 1
// and must not reduce any player's crew
global func StageFinalKill()
{
	var victim = CreateObject(CLNK, 70, 16, g_D2);
	if (!victim)
		FatalError("ArenaBotRound FAIL: final victim not created");
	g_Killer->Kill(victim);
	if (Local(g_T1, g_Goal) != 8)
		FatalError(Format("ArenaBotRound FAIL: final kill team %d score %d", g_T1, Local(g_T1, g_Goal)));
	if (Local(g_T2, g_Goal) != 0)
		FatalError(Format("ArenaBotRound FAIL: team %d scored illegally %d", g_T2, Local(g_T2, g_Goal)));
	if (GetPlayerCount() != 4)
		FatalError(Format("ArenaBotRound FAIL: player lost on final kill (count %d)", GetPlayerCount()));
	return 1;
}

global func StepFinal()
{
	var i;
	if (Local(g_T1, g_Goal) != 8)
		FatalError(Format("ArenaBotRound FAIL: final team 1 score %d != 8", Local(g_T1, g_Goal)));
	if (!g_Goal->~IsFulfilled())
		FatalError("ArenaBotRound FAIL: KILT not fulfilled after 8 kills");
	if (!g_Goal->~IsFulfilled())
		FatalError("ArenaBotRound FAIL: KILT fulfilled flag flapped");
	// MELE may not have ended the round early: all four players active
	if (GetPlayerCount() != 4)
		FatalError(Format("ArenaBotRound FAIL: %d players left, MELE would have fired", GetPlayerCount()));
	if (g_Melee->~IsFulfilled())
		FatalError("ArenaBotRound FAIL: MELE fulfilled before KILT");
	// nobody eliminated: every player keeps at least one crew member
	for (i = 0; i < 4; ++i)
		if (GetCrewCount(GetPlayerByIndex(i)) < 1)
			FatalError(Format("ArenaBotRound FAIL: player %d eliminated (crew 0)", GetPlayerByIndex(i)));
	Log(Format("ArenaBotRound PASS: team %d first to 8 kills, %d players alive", g_T1, GetPlayerCount()));
	GameOver();
	return -1;
}
