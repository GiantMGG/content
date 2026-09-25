/*-- KillTarget: first team to reach TargetScore kills wins the round --*/

#strict
#include GOAL

// Goal-object state:
//   Local(0)    = TargetScore (default 8)
//   Local(1000) = fulfilled flag (0/1) - the win log + VictorySting fire
//                 exactly once, on the first poll that sees the target hit
//   Local(tid)  = kills scored by team <tid> (arena teams are ids 1 and 2;
//                 any team id >= 1 maps to its own local slot)

protected func Initialize()
{
  // default target score
  Local(0) = 8;
  // base goal init: makes sure the GOAL controller object exists
  return _inherited();
}

public func IsFulfilled()
{
  // Tolerate too-few-players / unassigned teams: never fulfilled, never crash
  if (GetPlayerCount() < 2) return 0;
  if (!EnoughTeams()) return 0;
  var iTarget = GetTarget();
  var t;
  for (t = 1; t < 64; t++)
    if (Local(t) >= iTarget)
    {
      if (!Local(1000))
      {
        Local(1000) = 1;
        Log(Format("$MsgWin$", t, iTarget));
        if (SoundExists("VictorySting")) Sound("VictorySting");
      }
      return 1;
    }
  return 0;
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
