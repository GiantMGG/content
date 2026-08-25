/*-- Homestead Test Scenario --*/

#strict

protected func Initialize()
{
	// Grant all Agriculture pack knowledge to every player at start.
	var i;
	for (i = 0; i < GetPlayerCount(); i++)
	{
		var iPlr = GetPlayerByIndex(i);
		SetPlrKnowledge(iPlr, AGWM);
		SetPlrKnowledge(iPlr, AGSM);
		SetPlrKnowledge(iPlr, AGSK);
		SetPlrKnowledge(iPlr, AGFR);
		SetPlrKnowledge(iPlr, AGWS);
		SetPlrKnowledge(iPlr, AGAS);
	}
	// Place some fish in the lake so the fishing chain can be tested.
	for (var i = 0; i < 8; i++) PlaceAnimal(FISH);
	return(1);
}
