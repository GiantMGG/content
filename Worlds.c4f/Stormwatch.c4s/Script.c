/*-- Stormwatch -- five-storm coastal showcase. --*/
/*                                                                          */
/* Pure C4Script content. No engine files are touched. See                  */
/* specs/2026-08-27-1100-weather-showcase-scenario.md.                      */
/*                                                                          */
/* The scenario's [WeatherEvents] block (in Scenario.txt) is the engine's   */
/* random scheduler fallback. The scripted path below uses the explicit     */
/* LaunchWeatherEvent / StopWeatherEvent / GameOver C4Script APIs so a      */
/* modder reading this source sees both approaches.                         */

#strict 2

static g_fInitializedPlayers;
static g_iChapter;

// Five chapters: [C4ID, prepMsgKey, threatMsgKey, debriefMsgKey, durationTicks]
// Duration is in 35-frame timer ticks (~1s each); 700 ticks ~= 20s real time.
// The spec's §Mid-game table drives the order.
static const g_Chapters = [
	[STRM, "$MsgStormPrep$",  "$MsgStormHit$",   "$MsgStormDone$",  700],
	[BLZD, "$MsgBlizPrep$",   "$MsgBlizHit$",    "$MsgBlizDone$",   700],
	[DRGT, "$MsgDrgtPrep$",   "$MsgDrgtHit$",    "$MsgDrgtDone$",   700],
	[HTWV, "$MsgHtwvPrep$",   "$MsgHtwvHit$",    "$MsgHtwvDone$",   700],
	[FLDD, "$MsgFloodPrep$",  "$MsgFloodHit$",   "$MsgFloodDone$",  700]
];

// Local TutorialMessage wrapper so the scenario does not require the
// Tutorial.c4f system group. Mirrors ColonyBay.c4s:13-18.
global func TutorialMessage(string strMessage)
{
	var iPlr = 0;
	if (GetPlayerCount() > 0) iPlr = GetPlayerByIndex(0);
	return CustomMessage(strMessage, 0, iPlr, 0, 0, 0xffffff, DECO, "Portrait:SCLK::0000ff::1", MSG_Bottom, 300);
}

protected func Initialize()
{
	g_iChapter = 0;

	// --- Pre-place the settlement (spec §Opening) ---

	// Ruined HUT2 on the headland (40% completion -- Colony Bay ruin pattern).
	var iHeadX = LandscapeWidth() * 70 / 100;
	var iHeadY = LandscapeHeight() * 35 / 100;
	CreateConstruction(HUT2, iHeadX, iHeadY, -1, 40, 1);

	// Wheat field (3 AGWH seedlings) on the headland.
	for (var i = 0; i < 3; i++)
		CreateObject(AGWH, iHeadX - 20 + i * 10, iHeadY + 5, NO_OWNER);

	// WOOD stockpile near the hut (ignites under HTWV).
	for (var i = 0; i < 5; i++)
		CreateObject(WOOD, iHeadX + 10 + Random(20), iHeadY + 10, NO_OWNER);

	// Low CNKT sea wall along the beachfront (3 segments @ 30% completion --
	// low enough that FLDD's CastPXS overtops it; spec §Note).
	var iBeachY = LandscapeHeight() * 70 / 100;
	for (var i = 0; i < 3; i++)
		CreateConstruction(CNKT, LandscapeWidth() * 30 / 100 + i * 15, iBeachY, -1, 30, 1);

	// --- Start the crisis director (35-frame timer ~= 1s) ---
	AddEffect("CrisisDirector", this, 1, 35, this);

	TutorialMessage("$MsgIntro$");
	return true;
}

func InitializePlayer(iPlr)
{
	// Guard against double-init (mirrors Frontier.c4s:17-24 and ColonyBay.c4s:62).
	if (g_fInitializedPlayers) return;
	g_fInitializedPlayers = 1;
	// Starting equipment is handled by [Player1] HomeBaseMaterial.
	return true;
}

// --- CrisisDirector effect ---

func FxCrisisDirectorStart(object target, int effect, int temp)
{
	if (temp) return;
	// Chapter 0 prep message fires immediately (the director's Start runs
	// before the first Timer call).
	return 1;
}

func FxCrisisDirectorTimer(object target, int effect, int timer)
{
	var chapter = g_Chapters[g_iChapter];
	var iDuration = chapter[4];

	// Phase 1: prep window (first ~15% of duration) -- event not yet launched.
	if (timer < iDuration / 7)
		return 1;

	// Phase 2: launch the event (one-shot at the ~15% mark).
	if (timer == iDuration / 7)
	{
		var idEvent = chapter[0];
		LaunchWeatherEvent(idEvent, 50, chapter[4]);
		GameMsg(chapter[2]);
		return 1;
	}

	// Phase 3: event runs -- wait for the chapter duration to elapse.
	if (timer < iDuration)
		return 1;

	// Phase 4: stop the event, debrief, advance chapter.
	StopWeatherEvent();
	GameMsg(chapter[3]);
	++g_iChapter;

	if (g_iChapter >= GetLength(g_Chapters))
	{
		GameMsg("$MsgAllDone$");
		GameOver();
		return -1;  // kill the effect
	}

	// Next chapter prep message.
	var nextChapter = g_Chapters[g_iChapter];
	TutorialMessage(nextChapter[1]);
	return 1;
}
