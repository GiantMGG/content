/*-- RTAP: ReactionTap - player-operable material faucet. --*/
/* Double-click toggles open/closed; an open tap emits its material    */
/* every Timer ticks via DefCore Timer/TimerCall. A vane variant sets  */
/* wind instead. Object-relative CastPXS aims the nozzle (C4Script.cpp */
/* :2766-2770 object-context offset).                                   */

#strict 2

local tapOpen;   // 0=closed, 1=open
local tapMat;    // dispensed material name ("Water","Lava",...)
local tapAmt;    // PXS per tick while open
local tapVane;   // 1 = wind vane variant: SetWind(tapWindForce)
local tapWindForce;

public func SetTapMaterial(string szMat)    { tapMat = szMat; return true; }
public func SetTapAmount(int iAmt)          { tapAmt = iAmt; return true; }
public func SetTapVane(int iForce)          { tapVane = 1; tapWindForce = iForce; return true; }

protected func ControlDigDouble(object pClonk)
{
	tapOpen = !tapOpen;
	SetAction(tapOpen ? "Open" : "Idle");
	if (pClonk) pClonk->Message(tapOpen ? "$TapOpen$" : "$TapClosed$");
	Sound("RTAP_Click");  /* no asset: silent in console; harmless in GUI */
	return true;
}

func TapTick()
{
	if (!tapOpen) return;
	if (tapVane) { SetWind(tapWindForce); return; }
	CastPXS(tapMat, tapAmt, 8, 0, -5);
	return;
}
