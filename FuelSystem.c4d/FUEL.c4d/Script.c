/*-- FUEL: fuel-trait include library for FuelSystem.c4d --
  A fuel source is registered either by adding a branch to the fuel
  table below (zero-callback path) or by #including FUEL on the fuel
  item and overriding GetFuelValue()/GetFuelEfficiency() (callback
  path).  A burner consumes fuel via the global Burn_Consume(burner,
  need) helper.  Residual energy is banked in the burner's local
  fuel_residual. --*/

#strict 2

/* Residual-energy local on the burner. Burners that #include FUEL
   inherit this declaration so LocalN("fuel_residual") works. */
local fuel_residual;

/* ---- Fuel-source trait callbacks (defaults) ----
   Any def that #includes FUEL becomes a fuel source by default. */
public func IsFuel()            { return true; }
public func GetFuelValue()      { return 50; }
public func GetFuelEfficiency() { return 10; }
public func OnFuelConsumed(object byObject) {}

/* ---- Fuel table ----
   Efficiencies are scaled by 10 (12 = 1.2x, 7 = 0.7x, 15 = 1.5x)
   because this C4Aul dialect has no float literals.  Add a branch to
   register a fuel.  The per-item callback overrides the table. */
global func Fuel_TableIndex(id fuel_id)
{
	if (fuel_id == COAL) return 0;
	if (fuel_id == WOOD) return 1;
	if (fuel_id == OBRL) return 2;
	return -1;
}

global func Fuel_GetValue(object fuel)
{
	var v = fuel->~GetFuelValue();
	if (v) return v;
	var id = GetID(fuel);
	if (id == COAL) return 100;
	if (id == WOOD) return 40;
	if (id == OBRL) return 200;
	return 0;
}

global func Fuel_GetEfficiency(object fuel)
{
	var e = fuel->~GetFuelEfficiency();
	if (e) return e;
	var id = GetID(fuel);
	if (id == COAL) return 12;
	if (id == WOOD) return 7;
	if (id == OBRL) return 15;
	return 10;
}

global func Fuel_IsFuel(object fuel)
{
	if (fuel->~IsFuel()) return true;
	return Fuel_TableIndex(GetID(fuel)) >= 0;
}

/* ---- Burn_Consume: the burner helper ----
    1. Drain residual first.
    2. If need remains, collect fuel items from burner contents.
    3. Sort by efficiency descending (most clunker-efficient first).
    4. Consume items until need is met (OBRL -> BARL, else destroyed).
    5. Bank overshoot as residual.
    6. Call OnBurn(need) on the burner.
    Returns true if need was met (residual or fuel). */
global func Burn_Consume(object burner, int need)
{
	var orig_need = need;                 // M-1: pass original need to OnBurn
	/* 1. Drain residual first. */
	var res = burner->LocalN("fuel_residual");
	if (res > 0)
	{
		if (res >= need)
		{
			burner->LocalN("fuel_residual") = res - need;
			burner->~OnBurn(orig_need);
			return true;
		}
		need -= res;
		burner->LocalN("fuel_residual") = 0;
	}

	/* 2. Collect fuel items from burner contents. */
	var items = CreateArray();
	var i, obj;
	for (i = 0; obj = burner->Contents(i); ++i)
		if (Fuel_IsFuel(obj)) { items[GetLength(items)] = obj; }

	/* 3. Selection sort by efficiency descending. */
	var n = GetLength(items);
	for (i = 0; i < n - 1; ++i)
	{
		var best = i;
		var j;
		for (j = i + 1; j < n; ++j)
			if (Fuel_GetEfficiency(items[j]) > Fuel_GetEfficiency(items[best]))
				best = j;
		if (best != i)
		{
			var tmp = items[i];
			items[i] = items[best];
			items[best] = tmp;
		}
	}

	/* 4. Consume items until need is met. */
	var provided = 0;
	for (i = 0; i < n && provided < need; ++i)
	{
		var item = items[i];
		provided += Fuel_GetValue(item);
		item->~OnFuelConsumed(burner);
		if (GetID(item) == OBRL) ChangeDef(BARL, item);
		else                     RemoveObject(item);
	}

	/* 5. Bank overshoot as residual. */
	if (provided > need)
		burner->LocalN("fuel_residual") = provided - need;

	/* 6. Callback to the burner. */
	if (provided >= need)
		burner->~OnBurn(orig_need);
	return provided >= need;
}
