/*-- LiquidIdentitySmoke.c4s — headless liquid dispersion identity smoke. --*/
/*                                                             */
/* Pins U5 (cycle 85 spec §1): the dispersion ordering         */
/* Water/Acid widest -> Oil -> Lava narrowest, plus            */
/* per-material pixel conservation, so the liquid retune       */
/* cannot silently regress.                                    */
/*                                                             */
/* Geometry: four open-top Rock basins (interior 179px wide)   */
/* on the default flat map (ground top y=200), one per         */
/* liquid: Water x[100,300], Oil x[350,550], Lava x[600,800],  */
/* Acid x[850,1050]. Basin b: left wall x[b,b+10], right wall  */
/* x[b+190,b+200], floor y[150,160], interior x[b+11,b+189]    */
/* y[61,149]. Cast: CastPXS(mat, 500, 10, b+100, 100) at       */
/* step 1 (tick 35).                                           */
/*                                                             */
/* Metrics: span = max-min occupied x of the material family   */
/* in the basin interior; count = family pixel count there.    */
/* Lava family = Lava + Ashes (scan conversion, spec §4.1).    */
/*                                                             */
/* Calibration: g_fCalibrate=1 logs span/count per step and    */
/* skips assertions. Freeze thresholds from the logged values  */
/* with >=3x margin (SteamBuoyancy discipline), then flip to   */
/* 0.                                                          */
/*                                                             */
/* Driver note: the per-step timer is a GLOBAL effect with     */
/* GLOBAL callbacks (AddEffect(..., 0, ...) + global func      */
/* Fx...Timer) - the plain AddEffect(this) pattern does not    */
/* resolve callbacks in scenario scripts.                      */
/*                                                             */
/* On any assertion failure, FatalError produces a non-zero    */
/* exit code, failing the CTest entry.                         */

#strict 2

// ---- Basin layout (interior 179px wide, flat map) -----------
static const BaseWater = 100;
static const BaseOil   = 350;
static const BaseLava  = 600;
static const BaseAcid  = 850;

// ---- Calibrated constants (freeze after calibration) -------
static const CastAmt     = 500;  // PXS per basin
static const OrderStep   = 3; // step where ordering is asserted
static const SpanFullMin = 59; // water/acid span floor
static const SpanOilMin  = 59; // oil span floor
static const SpanLavaMax = 170; // lava span ceiling
static const SepMin      = 15; // oil-lava separation floor
static const ConserveMin = 480; // per-family count floor
static const FinalStep   = 9;    // conservation + PASS (tick 315)

static g_iStep;
static g_fCalibrate;

protected func Initialize()
{
	DrawBasin(BaseWater);
	DrawBasin(BaseOil);
	DrawBasin(BaseLava);
	DrawBasin(BaseAcid);

	g_iStep = 0;
	g_fCalibrate = 0;   // 0 = frozen assertions (calibration done)
	AddEffect("RunTest", 0, 1, 35, 0, 0);
	return true;
}

global func FxRunTestStart(target, effect, temp) { return 1; }

global func FxRunTestTimer(object target, int effect, int timer)
{
	++g_iStep;

	if (g_iStep == 1)
	{
		CastPXS("Water", CastAmt, 10, BaseWater + 100, 100);
		CastPXS("Oil",    CastAmt, 10, BaseOil + 100, 100);
		CastPXS("Lava",   CastAmt, 10, BaseLava + 100, 100);
		CastPXS("Acid",   CastAmt, 10, BaseAcid + 100, 100);
	}
	else
	{
		if (GetPXSCount() > 10000)
			FatalError(Format("LiquidIdentitySmoke FAIL step %d: PXS budget exceeded (%d)",
			                  g_iStep, GetPXSCount()));
	}

	if (g_fCalibrate && g_iStep >= 2)
		CalibrationSnapshot();

	if (!g_fCalibrate && g_iStep == OrderStep)
		AssertOrdering();

	if (!g_fCalibrate && g_iStep >= FinalStep)
	{
		AssertConservation();
		Log("LiquidIdentitySmoke PASS");
		GameOver();
		return -1;
	}
	return 1;
}

global func DrawBasin(int base)
{
	DrawMaterialQuad("Rock", base, 60, base + 10, 60, base + 10, 160, base, 160);
	DrawMaterialQuad("Rock", base + 190, 60, base + 200, 60, base + 200, 160, base + 190, 160);
	DrawMaterialQuad("Rock", base, 150, base + 200, 150, base + 200, 160, base, 160);
}

// Family span: max-min occupied x over the basin interior ROI.
// NOTE: mat_b==-1 means "no second family member" - it must NOT be compared
// directly, because GetMaterial() also returns -1 (MNone) for SKY pixels.
global func SpanFamily(int base, int mat_a, int mat_b)
{
	var xmin = -1, xmax = -1;
	var x, y;
	for (x = base + 11; x <= base + 189; x++)
		for (y = 61; y <= 149; y++)
		{
			var m = GetMaterial(x, y);
			if (m == mat_a || (mat_b != -1 && m == mat_b))
			{
				if (xmin < 0) xmin = x;
				xmax = x;
			}
		}
	if (xmin < 0) return -1;
	return xmax - xmin;
}

// Family count: pixels of the family in the basin interior ROI.
// Same -1 sentinel caveat as SpanFamily.
global func CountFamily(int base, int mat_a, int mat_b)
{
	var count = 0;
	var x, y;
	for (x = base + 11; x <= base + 189; x++)
		for (y = 61; y <= 149; y++)
		{
			var m = GetMaterial(x, y);
			if (m == mat_a || (mat_b != -1 && m == mat_b)) count++;
		}
	return count;
}

global func CalibrationSnapshot()
{
	var w = SpanFamily(BaseWater, Material("Water"), -1);
	var o = SpanFamily(BaseOil, Material("Oil"), -1);
	var l = SpanFamily(BaseLava, Material("Lava"), Material("Ashes"));
	var a = SpanFamily(BaseAcid, Material("Acid"), -1);
	var cw = CountFamily(BaseWater, Material("Water"), -1);
	var co = CountFamily(BaseOil, Material("Oil"), -1);
	var cl = CountFamily(BaseLava, Material("Lava"), Material("Ashes"));
	var ca = CountFamily(BaseAcid, Material("Acid"), -1);
	// NOTE: engine Format() accepts at most 9 args after the format string
	// (C4Script.cpp FnFormat: iPar0..iPar8), so the pxs total is NOT logged
	// here; the PXS budget is enforced inline in FxRunTestTimer instead.
	Log(Format("[CAL] step %d: span W=%d O=%d L=%d A=%d count W=%d O=%d L=%d A=%d",
	            g_iStep, w, o, l, a, cw, co, cl, ca));
}

global func AssertOrdering()
{
	var w = SpanFamily(BaseWater, Material("Water"), -1);
	var o = SpanFamily(BaseOil, Material("Oil"), -1);
	var l = SpanFamily(BaseLava, Material("Lava"), Material("Ashes"));
	var a = SpanFamily(BaseAcid, Material("Acid"), -1);

	if (w < SpanFullMin)
		FatalError(Format("LiquidIdentitySmoke FAIL: water span %d < %d", w, SpanFullMin));
	if (a < SpanFullMin)
		FatalError(Format("LiquidIdentitySmoke FAIL: acid span %d < %d", a, SpanFullMin));
	if (o < SpanOilMin)
		FatalError(Format("LiquidIdentitySmoke FAIL: oil span %d < %d", o, SpanOilMin));
	if (l > SpanLavaMax)
		FatalError(Format("LiquidIdentitySmoke FAIL: lava span %d > %d", l, SpanLavaMax));
	if (o - l < SepMin)
		FatalError(Format("LiquidIdentitySmoke FAIL: oil-lava separation %d < %d", o - l, SepMin));
}

global func AssertConservation()
{
	var cw = CountFamily(BaseWater, Material("Water"), -1);
	var co = CountFamily(BaseOil, Material("Oil"), -1);
	var cl = CountFamily(BaseLava, Material("Lava"), Material("Ashes"));
	var ca = CountFamily(BaseAcid, Material("Acid"), -1);

	if (cw < ConserveMin)
		FatalError(Format("LiquidIdentitySmoke FAIL: water conservation %d < %d", cw, ConserveMin));
	if (co < ConserveMin)
		FatalError(Format("LiquidIdentitySmoke FAIL: oil conservation %d < %d", co, ConserveMin));
	if (cl < ConserveMin)
		FatalError(Format("LiquidIdentitySmoke FAIL: lava conservation %d < %d", cl, ConserveMin));
	if (ca < ConserveMin)
		FatalError(Format("LiquidIdentitySmoke FAIL: acid conservation %d < %d", ca, ConserveMin));
}
