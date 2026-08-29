/*-- CoastalHarbor.c4s -- playable settlement+port demo. --*/
/* Uses the "coastal" Landscape.txt preset. Initialize calls the           */
/* CoastalMapLib OnMapGenerated() hook to stamp the default SLMR pair +    */
/* DKST foundation at the harbor bay, then starts the HarborMaster rule    */
/* and registers SILK trade at the dock so the ocean-trade caravan can     */
/* buy/sell against a local price curve. Goal: MONE=100 via port duty.     */

#strict

protected func Initialize()
{
	// Stamp the default harbor markers + dock foundation from the preset.
	OnMapGenerated();

	// Start the port-duty rule.
	CreateObject(HMST, 10, 10, NO_OWNER);

	// Register SILK trade at every dock so caravans can deal here.
	var pDock;
	for (pDock in FindObjects(Find_ID(DKST)))
		RegisterTradeGood(SILK, pDock, 10);

	return 1;
}
