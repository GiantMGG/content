/*-- AERO: shared aerodynamics include library for Airships.c4d --
  Provides weather-coupled lift/wind/burn modifiers consumed by all
  three vehicles. #include AERO on each vehicle to inherit these
  helpers. --*/

#strict 2

/* ---- Aero_WeatherModifier ----
   Returns [lift_pct, wind_pct, burn_pct] as integer percentages
   (100 = 1.0x). Values are scaled by GetWeatherEventIntensity()
   linearly: intensity 0 -> no effect (100), intensity 100 -> full
   table value.
   Table (at intensity 100):
     nil/clear  lift=100  wind=100  burn=100
     STRM       lift= 90  wind=200  burn=100
     BLZD       lift= 75  wind=120  burn=200
     DRGT       lift=100  wind=100  burn=100
     HTWV       lift= 80  wind=100  burn=100
     FLDD       lift=100  wind=100  burn=100   */
public func Aero_WeatherModifier()
{
	var event = GetActiveWeatherEvent();
	var intensity = GetWeatherEventIntensity();

	var lift_pct = 100, wind_pct = 100, burn_pct = 100;

	if (event == C4Id("STRM"))      { lift_pct = 90;  wind_pct = 200; burn_pct = 100; }
	else if (event == C4Id("BLZD")) { lift_pct = 75;  wind_pct = 120; burn_pct = 200; }
	else if (event == C4Id("DRGT")) { lift_pct = 100; wind_pct = 100; burn_pct = 100; }
	else if (event == C4Id("HTWV")) { lift_pct = 80;  wind_pct = 100; burn_pct = 100; }
	else if (event == C4Id("FLDD")) { lift_pct = 100; wind_pct = 100; burn_pct = 100; }

	lift_pct = 100 + (lift_pct - 100) * intensity / 100;
	wind_pct = 100 + (wind_pct - 100) * intensity / 100;
	burn_pct = 100 + (burn_pct - 100) * intensity / 100;

	return [lift_pct, wind_pct, burn_pct];
}

/* ---- Aero_WindDrift ----
   Returns wind-driven XDir delta, clamped to +/-10.
   Formula: (GetWind()/4) * (wind_pct/100) * (fragility_pct/100)
   fragility_pct: 100 = 1.0x (zeppelin), 50 = 0.5x (steampack),
                  150 = 1.5x (balloon). */
public func Aero_WindDrift(int fragility_pct)
{
	var mods = Aero_WeatherModifier();
	var wind_pct = mods[1];
	var wind = GetWind();
	return BoundBy(wind * wind_pct * fragility_pct / 40000, -10, 10);
}

/* ---- Aero_BurnTick ----
    Wraps Burn_Consume with the burn-rate multiplier.
    need = base_rate * burn_pct / 100.
    Returns true if fuel was available (Burn_Consume succeeded).
    Fuel consumption itself is handled by FuelSystem.c4d's
    Burn_Consume (OBRL->BARL conversion, efficiency-sorted
    consumption, residual banking). */
public func Aero_BurnTick(int base_rate)
{
	var mods = Aero_WeatherModifier();
	var burn_pct = mods[2];
	var need = base_rate * burn_pct / 100;
	return Burn_Consume(this, need);
}

/* Note: vehicles call Burn_Consume(this, need) directly via the
   #include FUEL trait. This preserves FuelSystem.c4d's OBRL->BARL
   conversion and efficiency-sorted consumption order. There is no
   Aero_Burn wrapper -- Aero_BurnTick handles the weather-adjusted
   burn rate and delegates to Burn_Consume. */
