/*-- CogWreck (CWCK) -- sunken cargo ship, post-foundering. --*/
/* Flooded Cogs ChangeDef to this def when they founder (see the flood    */
/* machine in Cog.c4d/Script.c). Float is not set here (default 0), so    */
/* engine gravity sinks the wreck; SinkTimer clamps descent to a fixed    */
/* terminal velocity so the sink rate is frame-assertable (spec Risk 1,   */
/* gotchas-file R3). The clamp mirrors the Cog's x-axis deterministic-    */
/* propulsion idiom and stays off while the wreck is stuck (bottomed).    */

#strict

static const SINK_TERMINAL = 4; // wreck terminal-velocity clamp, px/frame

protected func SinkTimer() {
	// R3 mitigation: clamp descent to a fixed terminal velocity so the
	// sink rate is frame-assertable (spec Risk 1).
	if (!Stuck() && GetYDir() != SINK_TERMINAL) SetYDir(SINK_TERMINAL);
	return 1;
}
