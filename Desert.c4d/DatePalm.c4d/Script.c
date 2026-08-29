/*-- DatePalm (DATP) -- 3-stage desert crop. --*/
/* Wheat.c4d precedent: soil-agnostic growth (Grow never probes ground). */
/* Sandstorm pauses growth; drought blocks Seedling germination.         */

#strict 3

local grow_stage;  // 0=Seedling, 1=Growing, 2=Ready

protected func Construction()
{
	grow_stage = 0;
	SetAction("Seedling");
	return 1;
}

public func IsDatePalm() { return true; }
public func IsRipe() { return GetAction() == "Ready"; }

/* TimerCall: advances Seedling -> Growing -> Ready once each. */
public func Grow()
{
	var evt = GetActiveWeatherEvent();
	if (evt == SNDT) return 1;  // sandstorm pauses growth
	if (evt == DRGT && GetAction() == "Seedling") return 1;  // no germination

	if (GetAction() == "Seedling") { grow_stage = 1; SetAction("Growing"); Sound("Dig?"); return 1; }
	if (GetAction() == "Growing")  { grow_stage = 2; SetAction("Ready");   Sound("Chop?"); return 1; }
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
