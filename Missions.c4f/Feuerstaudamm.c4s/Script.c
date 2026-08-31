/*-- Feuerstaudamm.c4s -- the "Feuerstaudamm" cascade showcase. --*/
/*                                                                 */
/* A lava reservoir, an oil pan, the open sea - and one diggable   */
/* sandstone keystone between them. Dig or blast the keystone and  */
/* the whole roadmap cascade fires: oil fire, sea crusted to       */
/* stone, a steam plume condensing to rain over the burnt          */
/* terrace, and the storm-launched dune burying the beach walkers. */
/*                                                                 */
/* The shared CascadeDirector (CDIR) paints the apparatus and      */
/* narrates each cascade beat as it lands; this host stages the    */
/* victims + drift director and polls AllBeats() for victory.      */

#strict 2

protected func Initialize()
{
	// Shared director: paint + beat narration.
	var dir = CreateObject(CDIR, 0, 0, NO_OWNER);
	dir->SetGroundRow(300);
	dir->SetLaunchStorm(true);
	dir->SetSeedSea(true);
	dir->PaintCascade(300);

	// Beach victims at the reservoir wall base, in the dune's
	// +x drift path (SDRF budget ramps under the SNDT storm).
	CreateObject(SCRP, 94, 288, NO_OWNER);
	CreateObject(SCRP, 97, 275, NO_OWNER);

	// Wind-driven surface-sand migration director (self-ticking).
	CreateObject(SDRF, 60, 245, NO_OWNER);

	// Calm wind: the dune idles below the |wind| >= 30 drift
	// threshold until the storm ramps it to gale.
	SetWind(20);

	AddEffect("VictoryPoll", 0, 1, 35, 0, 0);

	Log("Dig the sandstone keystone under the lava lake - or blast it.");
	return true;
}

global func FxVictoryPollStart(target, effect, temp) { return 1; }

global func FxVictoryPollTimer(object target, int effect, int timer)
{
	var dir = FindObject(CDIR);
	if (dir && dir->AllBeats())
	{
		Log("The cascade is complete. Feuerstaudamm PASS");
		GameOver();
		return -1;
	}
	return 1;
}
