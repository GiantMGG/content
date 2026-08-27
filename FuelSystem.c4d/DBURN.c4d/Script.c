/*-- DBURN: dummy burner for the FuelSystem smoke test --
  A minimal C4D_StaticBack container that #includes FUEL so it inherits
  the local fuel_residual slot. IsFuel() returns false so DBURN itself
  is never consumed as fuel. OnBurn(need) is the callback Burn_Consume
  fires when the need is met; here it just logs for observability. --*/

#strict 2

#include FUEL

/* The burner is not a fuel source. */
public func IsFuel() { return false; }

/* Burn_Consume fires this when need is met. */
public func OnBurn(int need)
{
	Log("DBURN OnBurn(%d)", need);
}

protected func Initialize() { return true; }
