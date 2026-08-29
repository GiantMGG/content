/*-- BeaconLib (BCNL) -- beacon query shim. --*/
/* global BeaconBonusAt(x, y) returns 0..1 navigation bonus computed from  */
/* lit lighthouses in range. Additive to FxSeaCaravanAI -- never a pathing */
/* rewrite. Multiple overlapping lighthouses take the max (not the sum),   */
/* so overlap does not inflate the bonus (spec edge case #5).              */

#strict

global func BeaconBonusAt(int iX, int iY)
{
	var iBest = 0;
	var pLHGT;
	for (pLHGT in FindObjects(Find_ID(LHGT), Find_Func("IsLit")))
	{
		if (pLHGT->InBeaconCone(iX, iY))
		{
			var iBonus = 1;  // full bonus inside the cone of a lit beacon
			if (iBonus > iBest) iBest = iBonus;
		}
	}
	return iBest;
}
