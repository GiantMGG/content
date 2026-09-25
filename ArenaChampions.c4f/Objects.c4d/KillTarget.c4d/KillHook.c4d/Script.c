/*-- KillTarget crew-death hook.
     Appended to CLNK only: MCLK (musket clonk) includes a CLNK component
     (DefCore Components=CLNK=1), so its deaths already arrive through the
     CLNK append. Appending to both would run the hook twice per MCLK death
     and double-credit the killer's team. _inherited keeps the base crew
     death running (sounds, OnClonkDeath broadcast, last-crew elimination). --*/

#strict

#appendto CLNK

protected func Death(int iKilledBy)
{
  var pVictim = this();
  var iVictimPlr = GetOwner(pVictim);
  if (iVictimPlr >= 0 && iVictimPlr < GetPlayerCount())
    if (iKilledBy >= 0 && iKilledBy < GetPlayerCount())
    {
      var iVictimTeam = GetPlayerTeam(iVictimPlr);
      var iKillerTeam = GetPlayerTeam(iKilledBy);
      // Team kills only: both sides on teams, different teams (no friendly fire)
      if (iVictimTeam > 0 && iKillerTeam > 0 && iKillerTeam != iVictimTeam)
      {
        var pGoal = FindObject(KILT);
        if (pGoal) pGoal->CreditKill(iKilledBy, iKillerTeam);
      }
    }
  return _inherited(iKilledBy);
}
