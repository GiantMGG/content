/*-- FirstObjectSmoke.c4s -- headless tutorial verification. --*/
/*                                                              */
/* Asserts the reader's GlowStone.c4d (id=GWST) spawns,         */
/* round-trips its id and Name, fires Initialize (via the       */
/* LocalN("g_initialized") flag), survives removal, and         */
/* idempotently re-creates. 7 steps, all run synchronously in   */
/* Initialize() — follows the SiegeSmoke.c4s contract.          */
/*                                                              */
/* On any assertion failure, FatalError produces a non-zero     */
/* exit code, failing the CTest entry smoke_FirstObjectSmoke.   */

#strict 2

protected func Initialize()
{
	RunSmokeSteps();
	return true;
}

func RunSmokeSteps()
{
	var g_glow;

	// Step 0: CreateObject(GWST); assert it spawned.
	g_glow = CreateObject(GWST, 100, 100, NO_OWNER);
	if (!g_glow)
		FatalError("FirstObjectSmoke FAIL step 0: CreateObject(GWST) returned 0 — is Objects.c4d loaded?");

	// Step 1: assert GetID round-trips the DefCore id.
	if (GetID(g_glow) != GWST)
		FatalError("FirstObjectSmoke FAIL step 1: GetID does not match GWST");

	// Step 2: assert GetName round-trips the Names.txt / DefCore Name.
	var name = GetName(g_glow);
	if (!name || name != "Glow Stone")
		FatalError(Format("FirstObjectSmoke FAIL step 2: GetName == '%s', expected 'Glow Stone'", name));

	// Step 3: assert the Initialize callback fired (g_initialized flag).
	if (!LocalN("g_initialized", g_glow))
		FatalError("FirstObjectSmoke FAIL step 3: LocalN(\"g_initialized\") is false — Initialize did not fire");

	// Step 4: remove the object; assert it is gone.
	RemoveObject(g_glow);
	if (FindObject(GWST))
		FatalError("FirstObjectSmoke FAIL step 4: FindObject(GWST) still returns an object after RemoveObject");
	g_glow = 0;

	// Step 5: re-create; assert the callback fires again (idempotent).
	g_glow = CreateObject(GWST, 100, 100, NO_OWNER);
	if (!g_glow)
		FatalError("FirstObjectSmoke FAIL step 5: re-CreateObject(GWST) returned 0");
	if (!LocalN("g_initialized", g_glow))
		FatalError("FirstObjectSmoke FAIL step 5: LocalN(\"g_initialized\") is false on re-create — Initialize not idempotent");

	// Step 6: pass + end.
	Log("FirstObjectSmoke PASS");
	GameOver();
}
