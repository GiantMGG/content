/* SpellCompositor - reusable spell-combine protocol library */

#strict

local Combinations; // proplist registry: key "idA_idB" -> idResult
local idFirstCombine; // first pick remembered between CombineSelect and CombineExecute

func Initialize() {
	Combinations = {};
	idFirstCombine = 0;
	RegisterAllCombinations();
}

/* Auto-discover: iterate all loaded definitions and register every def
   that declares a combine recipe via GetSpellCombine(). This avoids any
   forward references to spell ids that may be defined in other packs. */
public func RegisterAllCombinations() {
	Combinations = {};
	var idDef, i = 0;
	while (idDef = GetDefinition(i++)) {
		var pair = idDef->~GetSpellCombine();
		if (pair && GetType(pair) == C4V_Array && GetLength(pair) >= 2)
			if (pair[0] && pair[1])
				RegisterCombination(pair[0], pair[1], idDef);
	}
}

public func RegisterCombination(id a, id b, id result) {
	// Store under both orderings so CombineSpell is a single lookup
	Combinations[Format("%i_%i", a, b)] = result;
	Combinations[Format("%i_%i", b, a)] = result;
}

public func CombineSpell(id a, id b) {
	return Combinations[Format("%i_%i", a, b)]; // nil if no recipe
}

public func StartCombineMenu(object pClonk) {
	if (!pClonk->~ReadyToMagic()) return();
	SetComDir(COMD_Stop(), pClonk);
	OpenSpellMenu(pClonk, this, "$MnuCombineSpells$: %s", "CombineSelect", GetOwner(pClonk), pClonk, "$MnuNoSpells$");
}

public func CombineSelect(id idFirst, object pClonk) {
	// Remember first pick, open second menu
	idFirstCombine = idFirst;
	OpenSpellMenu(pClonk, this, "$MnuCombineSpells2$: %s", "CombineExecute", GetOwner(pClonk), pClonk, "$MnuNoSpells$");
}

public func CombineExecute(id idSecond, object pClonk) {
	var idFirst = idFirstCombine;
	idFirstCombine = 0;
	var idResult = CombineSpell(idFirst, idSecond);
	if (!idResult) {
		PlayerMessage(GetOwner(pClonk), "$MsgNoCombination$", pClonk);
		Sound("Error", 0, pClonk, 100, GetOwner(pClonk)+1);
		return();
	}
	// Play magic action and fire the fused spell from the clonk's position.
	// CreateObject offsets by this() (the compositor), so AbsX/AbsY convert
	// the clonk's absolute position into a compositor-relative offset — the
	// result spell is thus created AT the clonk, not at the compositor.
	pClonk->SetAction("Magic");
	Sound("SpellForge*", 0, pClonk, 100, GetOwner(pClonk)+1);
	var pResult = CreateObject(idResult, AbsX(GetX(pClonk)), AbsY(GetY(pClonk)), GetOwner(pClonk));
	if (pResult) pResult->~Activate(pClonk, pClonk);
	CastParticles("NoGravSpark", 8, 30, GetX(pClonk), GetY(pClonk), 30, 45, RGB(255,215,0), RGB(255,215,0));
}
