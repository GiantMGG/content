/*-- HarborMaster (HMST) -- port-duty rule. --*/
/* FxPortDuty ticks every ~200 frames, sums DKST throughput across all    */
/* docks, and grants a "port duty" DoWealth bonus. GetPortThroughput()    */
/* exposes the running tally for the scenario goal. TickPortDuty() is the */
/* smoke-test hook (called directly so the smoke need not wait for the    */
/* effect timer).                                                         */

#strict

local iPortThroughput;

protected func Initialize()
{
	iPortThroughput = 0;
	AddEffect("FxPortDuty", this, 1, 200, this);
	return 1;
}

public func TickPortDuty()
{
	var iSum = 0;
	var pDock;
	for (pDock in FindObjects(Find_ID(DKST)))
		iSum += pDock->~GetThroughput();
	if (iSum > 0)
	{
		iPortThroughput += iSum;
		var iPlr = GetAnyPlayer();
		if (iPlr >= 0) DoWealth(iPlr, iSum);
	}
}

func FxPortDutyTimer(object pTarget, int fx)
{
	pTarget->~TickPortDuty();
	return 1;
}

public func GetPortThroughput() { return iPortThroughput; }
