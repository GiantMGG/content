/*-- KillTarget: first team to reach TargetScore kills wins the round --*/

#strict
#include GOAL

// Goal-object state:
//   Local(0)    = TargetScore (default 8)
//   Local(1000) = fired flag (0/1) - the win log + VictorySting + GameOver
//                 fire exactly once, from the 35-frame Guard timer
//   Local(1001) = was-contested latch - at least two teams existed at some
//                 point (WIN-BY-WIPE may only end a contested round)
//   Local(tid)  = kills scored by team <tid> (arena teams are ids 1 and 2;
//                 any team id >= 1 maps to its own local slot)

protected func Initialize()
{
  // default target score
  Local(0) = 8;
  // base goal init: makes sure the GOAL controller object exists
  return _inherited();
}

// 35-frame round-end watchdog (DefCore Timer=35 + TimerCall=Guard). The
// round is decided HERE, not in IsFulfilled(): the engine's GOAL controller
// ends the round only when EVERY goal's IsFulfilled() returns true, and MELE
// stays unfulfilled while two teams are still fighting - so with
// Goals=MELE=1;KILT=1 a team reaching the kill target was announced but
// never enforced. Guard() fires the game-over itself, exactly once.
protected func Guard()
{
  if (Local(1000)) return;            // already fired
  if (EnoughTeams()) Local(1001) = 1; // was-contested latch
  // WIN-BY-SCORE: a team reached the kill target - the advertised win
  var iTarget = GetTarget();
  var t;
  for (t = 1; t < 64; t++)
    if (Local(t) >= iTarget)
    {
      // Atomic in a single Guard call: flag -> log -> sting -> GameOver, so
      // a smoke may treat Local(1000)==1 as "the goal ended the round".
      Local(1000) = 1;
      Log(Format("$MsgWin$", t, iTarget));
      if (SoundExists("VictorySting")) Sound("VictorySting");
      GameOver();
      return;
    }
  // WIN-BY-WIPE: the round was contested but only one team (or none) stands
  if (Local(1001) && !EnoughTeams())
  {
    var team = GetLastTeam();
    Local(1000) = 1;
    if (team > 0) Log(Format("$MsgWinLastTeam$", team));
    else Log("$MsgWinNoTeam$");
    if (SoundExists("VictorySting")) Sound("VictorySting");
    GameOver();
  }
  return;
}

public func IsFulfilled()
{
  // Tolerate too-few-players / unassigned teams: never fulfilled, never
  // crash. NO side effects in here: the win log / VictorySting / GameOver
  // live in Guard() only, so the controller's 250-frame poll stays pure.
  if (GetPlayerCount() < 2) return 0;
  if (!EnoughTeams()) return 0;
  // After Guard fired (Local 1000) the controller-/MELE-agreement remains
  // as a harmless backstop; AnyTeamAtTarget() is the pure target check.
  return Local(1000) || AnyTeamAtTarget();
}

// Target check without side effects (the boolean core of the old IsFulfilled
// loop).
private func AnyTeamAtTarget()
{
  var iTarget = GetTarget();
  var t;
  for (t = 1; t < 64; t++)
    if (Local(t) >= iTarget) return 1;
  return 0;
}

// Team id shared by all remaining players; 0 when none / ambiguous.
private func GetLastTeam()
{
  var team = 0, i;
  for (i = 0; i < GetPlayerCount(); i++)
  {
    var t = GetPlayerTeam(GetPlayerByIndex(i));
    if (t <= 0) continue;
    if (!team) team = t;
    else if (t != team) return 0;
  }
  return team;
}

protected func Activate(iPlr)
{
  // Standings window: one "Team: score/target" line per team
  var iTarget = GetTarget();
  var ms = 0;
  var i;
  for (i = 0; i < GetTeamCount(); i++)
  {
    var team = GetTeamByIndex(i);
    if (!team) continue;
    if (ms) ms = Format("%s | $MsgTeam$", ms, team, Local(team), iTarget);
    else ms = Format("$MsgTeam$", team, Local(team), iTarget);
  }
  if (!ms) ms = GetDesc();
  MessageWindow(ms, iPlr);
  return 1;
}

// Called by the crew death hook (KillHook.c4d) whenever a team kill lands
public func CreditKill(int iKillerPlr, int iKillerTeam)
{
  var iTarget = GetTarget();
  var iScore = Local(iKillerTeam) + 1;
  Local(iKillerTeam) = iScore;
  Log(Format("$MsgKill$", GetPlayerName(iKillerPlr), iScore, iTarget));
  if (SoundExists("KillPing")) Sound("KillPing");
  return iScore;
}

private func GetTarget()
{
  var iTarget = Local(0);
  if (!iTarget) iTarget = 8;
  return iTarget;
}

private func EnoughTeams()
{
  // Count distinct team ids among the players; needs at least two
  var iTeams = 0, i, j;
  for (i = 0; i < GetPlayerCount(); i++)
  {
    var team = GetPlayerTeam(GetPlayerByIndex(i));
    if (team > 0)
    {
      for (j = 0; j < i; j++)
        if (GetPlayerTeam(GetPlayerByIndex(j)) == team) break;
      if (j == i) iTeams++;
    }
  }
  return iTeams >= 2;
}
