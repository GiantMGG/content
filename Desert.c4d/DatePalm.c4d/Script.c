/*-- DatePalm (DATP) -- 3-stage desert crop. --*/
/* Wheat.c4d precedent: soil-agnostic growth (Grow never probes ground). */
/* Sandstorm pauses growth; drought blocks Seedling germination.         */

#strict 3

protected func Construction()
{
	SetAction("Seedling");
	return 1;
}

public func IsDatePalm() { return true; }
public func IsRipe() { return GetAction() == "Ready"; }

/* TimerCall: advances Seedling -> Growing -> Ready once each.
   Palms near an oasis are growth-boosted (spec §5): one Grow()
   call advances two stages. */
public func Grow()
{
	var evt = GetActiveWeatherEvent();
	if (evt == SNDT) return 1;  // sandstorm pauses growth
	if (evt == DRGT && GetAction() == "Seedling") return 1;  // no germination

	// Growth boost query: an oasis within 100px doubles the pace.
	var boosted = FindObject2(Find_ID(OASS), Find_Distance(100));

	if (GetAction() == "Seedling")
	{
		SetAction("Growing");
		Sound("Dig?");
		if (!boosted) return 1;
	}
	if (GetAction() == "Growing")
	{
		SetAction("Ready");
		Sound("Chop?");
		return 1;
	}
	return 1;
}

/* Harvested by hand or sickle: yields 2 Date items. */
public func Harvest(object pClonk)
{
	if (!IsRipe()) return 0;
	var i;
	for (i = 0; i < 2; i++)
	{
		var pDate = CreateObject(DATE, 0, 0, GetOwner());
		if (!pDate) continue;
		if (pClonk) pClonk->Collect(pDate);
		else pDate->SetPosition(GetX(), GetY() - 10);
	}
	RemoveObject();
	return 1;
}
