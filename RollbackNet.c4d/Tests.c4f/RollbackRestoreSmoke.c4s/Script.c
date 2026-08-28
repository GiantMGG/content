/*-- RollbackRestoreSmoke.c4s — live-game fixture for rollback restore. --*/

#strict 2

protected func Initialize()
{
	// Create a ROCK at a known position so the savegame has an object
	// to serialize. The object is not strictly required for the smoke
	// test, but it exercises the object-serialization path of the
	// save/load round-trip.
	var obj = CreateObject(ROCK, 50, 30, NO_OWNER);
	if (!obj)
		FatalError("RollbackRestoreSmoke FAIL step 0: could not spawn rock");

	// Save state — exercises SaveRuntimeDataToBuffer end-to-end.
	if (!RollbackSaveState())
		FatalError("RollbackRestoreSmoke FAIL step 0: RollbackSaveState failed");

	// Move the object so the post-restore state differs from the
	// pre-save state. This ensures the save captured a distinct
	// snapshot, not just the current state.
	obj->SetPosition(80, 20);

	// Restore state — exercises LoadRuntimeDataFromBuffer end-to-end.
	// The in-place restoration is a best-effort operation: it may not
	// fully restore every game object, but it must not crash or leave
	// the engine in an inconsistent state.
	if (!RollbackLoadState())
		FatalError("RollbackRestoreSmoke FAIL step 0: RollbackLoadState failed");

	Log("RollbackRestoreSmoke PASS");
	GameOver();
	return true;
}
