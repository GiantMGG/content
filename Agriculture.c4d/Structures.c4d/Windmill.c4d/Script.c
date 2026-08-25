/*-- Muehle (Agriculture) --*/

#strict

local grind_time;   // computed at ProductionStart; counts down in Grinding

protected func Initialize()
{
	CreateWing();
	return(1);
}

private func CreateWing()
{
	var pWing = CreateObject(AGWG, 0, -40, GetOwner());
	if (pWing) ObjectSetAction(pWing, "Turn", this());
	return(pWing);
}

/* TimerCall — repair/replace the wing if missing (mirrors WMIL). */
public func CheckWing()
{
	if (GetCon() == 100)
		if (!FindObject(AGWG, 0, 0, 0, 0, 0, "Turn", this()))
			CreateWing();
	return(1);
}

/* Production UX (mirrors Oven/Doughpot). */

public func ContextGrind(object pCaller)
{
	[$TxtGrind$|Image=FLOU]
	return(ProductionOrder(pCaller));
}

public func ControlDigDouble(object pCaller)
{
	[$TxtGrind$|Image=FLOU]
	return(ProductionOrder(pCaller));
}

public func ProductionOrder(object pWorker)
{
	// Already have wheat on hand?
	if (!FindContents(AGSH))
	{
		// Acquire a wheat sheaf
		AddCommand(pWorker, "Put", this, 0, 0, 0, 0, AGSH);
		AddCommand(pWorker, "Acquire", 0, 0, 0, this, 0, AGSH, 5, 3);
	}
	// Walk to the windmill, then start
	AddCommand(pWorker, "MoveTo", 0, GetX(), GetY());
	AddCommand(pWorker, "Call", this, 0, 0, 0, 0, "ProductionStart");
	return(1);
}

public func ProductionStartFailed() { return(1); }

public func ProductionStart()
{
	if (!FindContents(AGSH))
	{
		Message("$TxtNowheat$", this);
		return(false);
	}
	// Consume one wheat sheaf
	RemoveObject(FindContents(AGSH));
	// Compute grinding time: wind scales throughput
	var wind = Abs(GetWind());
	grind_time = Max(80, 200 - wind * 2);
	if (wind < 5) Message("$TxtLowwind$", this);
	SetAction("Grinding");
	return(1);
}

private func Grinding()
{
	if (GetActTime() < grind_time) return(1);
	SetAction("Idle");
	// Output Flour (FLOU — the existing Knights item)
	var pFlour = CreateObject(FLOU, 0, 0, GetOwner());
	if (pFlour) pFlour->Exit(RandomX(-6, 6), -10);
	Sound("WoodHit*");
	return(1);
}

/* Only accept wheat sheaves as material input. */
protected func RejectCollect(id def, object obj)
{
	if (def == AGSH)
	{
		if (ContentsCount(AGSH) < 5) return(0);
		Message("$TxtMorewheatwontfit$", this);
		return(1);
	}
	return(1);
}
