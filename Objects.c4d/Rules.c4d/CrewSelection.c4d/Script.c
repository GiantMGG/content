/*-- CrewSelection rule --*/

#strict

// Generic rule-injection callback. Called by the engine from
// C4ObjectMenu::AddContextFunctions for every C4D_Rule object in the section.
// pTarget = the right-clicked object (the crew Clonk).
// pCaller = the Clonk whose context menu this is (the acting crew member).
// Returns: an array of [caption, command, idSymbol, infoCaption] tuples,
//          or 0/empty to inject nothing.
public func GetContextMenuItems(object pTarget, object pCaller) {
    // Only inject for crew members owned by the calling player.
    if (!pTarget || !pCaller) return [];
    if (!(pTarget->GetOCF() & OCF_CrewMember)) return [];
    if (pTarget->GetOwner() != pCaller->GetOwner()) return [];

    return [
        [
            "$SelectCrew$",                                                                  // caption (stringtbl key)
            Format("Object(%d)->CrewSel_OpenMenu(Object(%d), this)", this->ObjectNumber(), pTarget->ObjectNumber()),  // command
            GetID(),                                                                          // symbol id (this rule)
            "$SelectCrewDesc$"                                                                // info caption
        ]
    ];
}

// Open the crew-selection submenu. pClonk = the right-clicked crew member
// (kept as a reference for "select up to here" if we ever add it; not used in v1).
public func CrewSel_OpenMenu(object pClonk, object pCaller) {
    var plr = pCaller->GetOwner();
    // Capture current cursor position so we can restore it after re-rendering.
    var iOldSel = GetMenuSelection(pCaller);
    // Create the submenu on the calling clonk. pCommandObj = this rule,
    // so OnMenuSelection / our CrewSel_* commands resolve against the rule object.
    pCaller->CreateMenu(GetID(), pCaller, this, 0, 0, 0, 1);

    // "Select All" / "Clear" entries. Use Format to build explicit command
    // strings so FnAddMenuItem doesn't fall back to "old style" (FuncName(idItem,
    // parameter)) which would pass a C4ID where an object is expected.
    var szCmdAll = Format("CrewSel_SelectAll(Object(%d))", pCaller->ObjectNumber());
    var szCmdClr = Format("CrewSel_Clear(Object(%d))",     pCaller->ObjectNumber());
    pCaller->AddMenuItem("$SelectAll$", szCmdAll, GetID(), pCaller, 0, 0, "$SelectAllDesc$");
    pCaller->AddMenuItem("$ClearSel$",  szCmdClr, GetID(), pCaller, 0, 0, "$ClearSelDesc$");

    // One entry per crew member
    var iCount = GetCrewCount(plr);
    for (var i = 0; i < iCount; i++) {
        var pCrew = GetCrew(plr, i);
        if (!pCrew) continue;
        // Skip dead crew (engine also filters, but be explicit for the menu)
        if (!(pCrew->GetAlive())) continue;
        // Command toggles this crew member; passes pCaller so the submenu can
        // be re-rendered on the calling clonk.
        var szCmd  = Format("CrewSel_Toggle(Object(%d), Object(%d))", pCrew->ObjectNumber(), pCaller->ObjectNumber());
        var szInfo = Format("%s, %d%% HP", pCrew->GetName(), pCrew->GetEnergy());
        // Parameter = crew object so AddMenuItem uses its portrait as the symbol.
        pCaller->AddMenuItem(pCrew->GetName(), szCmd, pCrew->GetID(), pCrew, 0, pCrew, szInfo);
    }

    // Restore cursor position (if valid) so re-renders don't reset the cursor.
    if (iOldSel >= 0) SelectMenuItem(iOldSel, pCaller);
    return true;
}

// Toggle handler — called when the player clicks a crew member's portrait.
public func CrewSel_Toggle(object pCrew, object pCaller) {
    if (!pCrew) return false;
    // Read the current selection state via the "Selected" property and flip it.
    // SelectCrew(iPlr, pObj, fSelect) is the engine-exposed way to toggle
    // (DoSelect/UnSelect are C++-only and not callable from C4Script).
    var fSelected = GetObjectVal("Selected", 0, pCrew);
    SelectCrew(pCrew->GetOwner(), pCrew, !fSelected);
    // Re-render the submenu in place so the checkmark/tint updates.
    CrewSel_OpenMenu(0, pCaller);
    return true;
}

public func CrewSel_SelectAll(object pCaller) {
    var plr = pCaller->GetOwner();
    for (var i = 0, c; c = GetCrew(plr, i); i++) SelectCrew(plr, c, true);
    CrewSel_OpenMenu(0, pCaller);
    return true;
}

public func CrewSel_Clear(object pCaller) {
    var plr = pCaller->GetOwner();
    for (var i = 0, c; c = GetCrew(plr, i); i++) SelectCrew(plr, c, false);
    CrewSel_OpenMenu(0, pCaller);
    return true;
}

protected func Activate(int iByPlayer) {
    MessageWindow(GetDesc(), iByPlayer);
    return 1;
}
