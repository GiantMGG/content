/*-- Granary.c4d — scenario-local granary def (AGWM donor sprite), shared by
   the AutoTasksSmoke/AutoTasksAccept scenario pair (plan R4). Byte-identical
   mirror twin in the other scenario dir — keep in sync. --*/

#strict 3

/* Menu fleet-condition parity: GRNY spawns complete via CreateObject, so this
   is trivially true (Sawmill.c4d/Script.c:140-142 shape). */
protected func IsBuilt()
{
	return GetCon() >= 100;
}

/* Windmill.c4d/Script.c:91-99 shape, cap raised to 10. This is the
   engine-collect face; the headless script path is capped in Deposit (Task 2).
   Accept AGSH/AGAP while under their caps, reject everything else. */
protected func RejectCollect(id def, object obj)
{
	if (def == AGSH)
	{
		if (ContentsCount(AGSH) < 10) return(0);
		return(1);
	}
	if (def == AGAP)
	{
		if (ContentsCount(AGAP) < 10) return(0);
		return(1);
	}
	return(1);
}
