/*-- CTGT.c4d -- script-only Call target for CallSemanticsSmoke. --*/
/*                                                               */
/* Hosts FnX (the normal-path callee: logs CX:ran, returns       */
/* false) and ~FnXFailed (the fail-callback the engine would     */
/* fire on a failed Call command). The smoke scenario pins that  */
/* ~FnXFailed is structurally unreachable for C4CMD_Call.        */

#strict 2

local fRan;
local fRanCount;
local fFailed;

/* Normal-path callee: MUST log CX:ran and return false. The     */
/* engine's C4Command::Call() never reads this return.           */
func FnX()
{
	fRan = true;
	fRanCount++;
	Log("CX:ran");
	return false;
}

/* Fail-callback: logged in the MUT-red drift only. On stock     */
/* code this must NEVER fire (Finish(true) precedes the callee). */
func FnXFailed()
{
	fFailed = true;
	Log("CX:failed");
	return true;
}
