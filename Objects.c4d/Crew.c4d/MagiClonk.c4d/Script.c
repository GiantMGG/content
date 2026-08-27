/*-- Zauberclonk --*/

#strict

#include CLNK

static const MCLK_ComboExtraDataName = "MCLK_PrefCombo";
static const MCLK_ManaTrainRate = 10; // Zauber geben Wert/x Magiephysicaltraining
static const MCLK_QSPrefix = "MCLK_QS";

local iCombo;			// Kombomen�modus (Nur Fantasypack): 0 (Normales Men�), 1 (Kombomen�), 2(Kombomen� mit festen links/rechts-Kombos)
local pComboMenu;		// Aktives Kombomen�; wird geschlossen, wenn der Zauberer gest�rt wird (Nur Fantasypack)
local idCurSpell, iCurPhase;        // Animationsphase und zugewiesener Zauber w�hrend des Zaubers

local idLastSpell;		// ID des gegenw�rtigen/letzten Zauberspruches

local iProcess;			// Verarbeitungszeit


public func MaxMagicPhysical()
{
  // Maximal trainierbarer Magiewert eines Zauberclonks (Achtung: DefinitionCall f�r Infoobjekte!)
  // Physical-Wert der Ursprungsdefinition plus 50.000
  return (GetPhysical("Magic", 0, 0, GetID()) + 50000);
}

protected func GetFairCrewPhysical(string sPhysical, int iRank, & piVal)
{
  // FairCrew-Zauberenergie: Rang 0 hat Basiswert. Rang 10 hat maximale Zauberenergie.
  if (sPhysical eq "Magic")
    {
    var iBase = GetPhysical("Magic", 0, 0, GetID());
    piVal = iBase + (MaxMagicPhysical()-iBase)*BoundBy(iRank,0,10)/10;
    return(true);
    }
  // Clonk-FairCrew-Physical�berladungen
  return(_inherited(sPhysical, iRank, piVal));
}

/* (De-)Initialisierung */

private func UpdatePhysical(string phys) 
{
  if (GetPhysical (phys, 1) < GetPhysical (phys, 0, 0, GetID ()))
    SetPhysical (phys, GetPhysical (phys, 0, 0, GetID ()), 1);
}

protected func Initialize(a,b,c)
  {
  // Clonk-Initialisierung
  inherited(a,b,c);
   // Neue Physicals dauerhaft setzen, wenn der Clonk nicht schon h�her trainiert hat
  UpdatePhysical ("Breath");
  UpdatePhysical ("Scale");
  UpdatePhysical ("Hangle");
  UpdatePhysical ("Dig");
  UpdatePhysical ("Swim");
  UpdatePhysical ("Push");
  UpdatePhysical ("Fight");
  UpdatePhysical ("Magic");
  return(1);
  }

protected func Destruction(a,b,c)
  {
  // Objekt wird gel�scht: Vorsichtshalber das Men� entfernen
  if (pComboMenu) RemoveObject(pComboMenu);
  if (pAimer) RemoveObject(pAimer);
  return(inherited(a,b,c));
  }
  
protected func Recruitment(int iPlayer, a,b,c)
  {
  // Kombo nach Spieler-Default
  if (IsComboSystemEnabled())
    iCombo = GetPlrExtraData(iPlayer, MCLK_ComboExtraDataName());
  return (_inherited(iPlayer, a,b,c));
  }
  
/* Steuerung */

protected func ControlThrow()
{
 // Kein Fallenlassen von Objekten w�hrend ein Zauber vorbereitet wird
 if(idCurSpell) return true;
 return _inherited(...);
}

protected func ControlSpecial(object pCaller)
  {
  [$CtrlQuickspellDesc$|Image=MCMQ]
  // Kombosystem: Zauber starten
  if(iCombo) return(DoComboCasting());
  
  // Men�system: Schnellzauber
  
  // Keine Schnellzauber belegt
  if (!Quickspells()) { DoCastError("$MsgNoQuickSpells$"); return(); }
  // Nicht bereit
  if (!ReadyToMagic()) return(0, DoCastError(Format("$MsgNotReadyToMagic$", GetName())));
  // Men� �ffnen
  OpenEmptySpellMenu(MCMS, this, this, this, "");
  for(var i=0; i < MaxQuickspells(); ++i)
  {
    var idData = GetPlrExtraData(GetOwner(),Format("%s%d",MCLK_QSPrefix(),i));
    if(IsQSID(idData))
      AddMenuItem("$MnuSlotQuickspell$: %s", "DoMagic", idData);
    else
      AddMenuItem("$MnuSlotQuickspell$: $MnuEmpty$", "Nothing", MCMX);
  }
  // in die Mitte
  SelectMenuItem(MaxQuickspells()/2);
  return(1);
  }

protected func ControlSpecialDouble(object pCaller)
  {
  [$descChooseSpellClass$|Image=MCMS|Condition=Combo]
  // Zauberei abbrechen
  if (IsCasting()) AbortCasting();
  // Fertig; keine Behandlung
  return();
  }


/* Kontextmen� */  
  
public func ContextComboHelp(object pByObject)
{
  [$CtxComboHelp$|Image=CXIN|Condition=ComboOn]
  // Hilfe anzeigen
  MessageWindow("$MsgComboHelp$", GetOwner(pByObject), CXIN, "$CtxComboHelp$");
  return(1);
}
  
public func ContextCombo(pByObject)
{
  [$CtxCombo$|Image=MCMC|Condition=IsComboSystemEnabled]
  // Kombosystem �ndern
  iCombo = (iCombo+1) % 3;
  // Meldung und Sound
  if(iCombo)
  {
    var sMsg; if (iCombo == 2) sMsg = "$ChangedToCombo2$"; else sMsg = "$ChangedToCombo$";
    PlayerMessage(GetOwner(),sMsg, this());
    Sound("SpellSysCombo*", 0, 0, 0, GetOwner()+1);
    CreateParticle("NoGravSpark", 0,0,  0,-6, 5*10, RGBa(255,255,0, 0), this());
    CreateParticle("NoGravSpark", 0,0, -5,-3, 5*10, RGBa(255,255,0, 0), this());
    CreateParticle("NoGravSpark", 0,0, -5, 3, 5*10, RGBa(255,255,0, 0), this());
    CreateParticle("NoGravSpark", 0,0,  5,-3, 5*10, RGBa(255,255,0, 0), this());
    CreateParticle("NoGravSpark", 0,0,  5, 3, 5*10, RGBa(255,255,0, 0), this());
    CreateParticle("NoGravSpark", 0,0,  0, 6, 5*10, RGBa(255,255,0, 0), this());
  }
  else
  {
    PlayerMessage(GetOwner(),"$ChangedToMenu$", this());
    Sound("SpellSysMenu*", 0, 0, 0, GetOwner()+1);
    CreateParticle("NoGravSpark", 0,0,  0,-6, 5*10, RGBa(255,0,0, 0), this());
    CreateParticle("NoGravSpark", 0,0, -5,-3, 5*10, RGBa(255,0,0, 0), this());
    CreateParticle("NoGravSpark", 0,0, -5, 3, 5*10, RGBa(255,0,0, 0), this());
    CreateParticle("NoGravSpark", 0,0,  5,-3, 5*10, RGBa(255,0,0, 0), this());
    CreateParticle("NoGravSpark", 0,0,  5, 3, 5*10, RGBa(255,0,0, 0), this());
    CreateParticle("NoGravSpark", 0,0,  0, 6, 5*10, RGBa(255,0,0, 0), this());
  }
 
  // Dies als Default f�r zuk�nftige Clonks
  SetPlrExtraData(GetController(), MCLK_ComboExtraDataName(), iCombo);
}
  
public func ContextQuickspell(object pByObject, int iSelection)
{
  [$CtxQuickspellDesc$|Image=MCMQ|Condition=NoCombo]
  // Clonk soll anhalten
  SetComDir(COMD_Stop(), pByObject);
  // Men� �ffnen
  OpenEmptySpellMenu(MCMQ, this, this, this, "");
  for(var i=0; i < MaxQuickspells(); ++i)
  {
    var idData = GetPlrExtraData(GetOwner(),Format("%s%d",MCLK_QSPrefix(),i));
    if(IsQSID(idData))
      AddMenuItem(Format("$MnuSlotNumber$: %s", i+1, "%s"), "QuickspellEditMenu",idData,0,0,i);
    else
      AddMenuItem(Format("$MnuSlotNumber$: $MnuEmpty$",i+1), "QuickspellEditMenu",MCMX,0,0,i);
  }
  // mittleren Spruch ausw�hlen
  SelectMenuItem(MaxQuickspells()/2);
  return(1);
}

public func ContextMagic(object pByObject)
{
  [$CtxMagicDesc$|Image=MCMS|Condition=ReadyToMagic]
  // Nicht bereit
  if (!ReadyToMagic()) return(0, DoCastError(Format("$MsgNotReadyToMagic$", GetName())));
  // Clonk soll anhalten
  SetComDir(COMD_Stop(), pByObject);
  // Men� �ffnen
  OpenSpellMenu(this, this, "$MnuCast$: %s", "DoMagic", GetOwner(), this, "$MnuNoSpells$", idLastSpell);
  return(1);
}

public func ContextCombineSpells(object pByObject)
{
  [$CtxCombineDesc$|Image=MSCP|Condition=ReadyToMagic]
  if (!ReadyToMagic()) return(0, DoCastError(Format("$MsgNotReadyToMagic$", GetName())));
  if (!FindObject(MSCP)) return(0, DoCastError("$MsgNoCompositor$"));
  SetComDir(COMD_Stop(), pByObject);
  var pComp = FindObject(MSCP);
  pComp->~StartCombineMenu(this());
  return(1);
}

/* Zaubern */

public func ReadyToMagic()
{
  // Im Zaubern auch OK, wenn der letzte Zauber schon durch ist
  if (WildcardMatch(GetAction(), "*Magic") && GetPhase() > 5) return(true);
  // Nur wenn der Clonk steht, springt oder schwimmt, oder dies gleich tun wird
  if (GetAction() ne "Walk" && GetAction() ne "Jump" &&
      GetAction() ne "Swim" && GetAction() ne "Ride" && GetAction() ne "RideStill"
      && GetAction() ne "KneelDown" && GetAction() ne "KneelUp" && GetAction() ne "FlatUp"
      && GetAction() ne "Dive")
    return(0);
  // Kann jetzt auch im Springen und in Geb�uden zaubern, dies sollten die Zauber 
  // aber ber�cksichtigen
  return(1);
}

protected func DoMagic(idSpell)
{
  // ID des Zauberspruchs merken
  idLastSpell = idSpell;  
  
  // Nicht bereit
  if (!ReadyToMagic()) return(0, DoCastError(Format("$MsgNotReadyToMagic$", GetName())));
  
  // Kein Mana/Zutaten
  if (!CheckMagicRequirements(idSpell)) return();

  // Zauberaktion starten: Dies zaubert direkt im Schwimmen; ansonsten setzt es die Magieaktivit�t, die verz�gert zaubert
  return (SetMagicAction(idSpell));
}

public func ExecMagic(id idSpell)
{
  // Weiterer Mana/Zutatencheck: K�nnte w�hrend der Magieaktion Mana verloren haben
  if (!CheckMagicRequirements(idSpell)) return();
  
  // Kombosystem: Zaubernamen anzeigen
  if (iCombo) PlayerMessage(GetController(), GetName(0, idSpell), this());
  
  // Eventuelle �berbleibsel der letzten Zauberdaten zur�cksetzen
  pAimedSpellOrigin = 0;

  // Zauberobjekt erschaffen und aktivieren
  var pSpell = CreateObject(idSpell,0,0,GetOwner());
  if (!pSpell) return(); // Zauber fehlgeschlagen
  if (!pSpell->~Activate(this()))
    // Zauber fehlgeschlagen
    return(0);

  // Zauberenergie abziehen
  DoMagicEnergy(-Value(idSpell));

  // alchemische Zutaten abziehen
  if(ObjectCount(ALCO)) ReduceAlchem(idSpell);
  
  // Wenn nicht noch gezielt wird, ist der Zauber fertig
  if (!LocalN("pAimer")) this->~SpellSucceeded(idSpell);
  
  return(1);
}

public func CheckMagicRequirements(id idSpell, bool fSilent)
{
  // Nicht genug Zauberenergie
  var iNumCast=MCLK_UnlimitedCast, iNumCast2=MCLK_UnlimitedCast;
  if(!(iNumCast = CheckMana(idSpell)))
  {
    if (!fSilent) DoCastError("$MsgNotEnoughEnergy$");
    return();
  }

  // Nicht genug alchemische Komponenten
  if(ObjectCount(ALCO)) if(!(iNumCast2=CheckAlchem(idSpell,this())))
  {
    if (!fSilent) DoCastError(Format("$MsgNotEnoughIngredients$", GetNeededAlcStr(idSpell,this())));
    return();
  }
  
  // Zaubern OK
  return (Min(iNumCast, iNumCast2));
}

private func SetMagicAction(id idForSpell)
{
  // Im Schwimmen direkt zaubern
  if (GetAction() eq "Swim" && idForSpell)
    return (ExecMagic(idForSpell));
  idCurSpell = idForSpell;
  // Im Sprung
  if (GetAction() eq "Jump" || GetAction() eq "Hold") 
    return(SetAction("JumpMagic"));
  // Beim Reiten
  if (IsRiding())
    return(SetAction("RideMagic", GetActionTarget()));
  // Bei KNEEL erstmal nicht; EndCall der KNEEL-Aktivit�t spricht den Zauber
  if (GetAction() eq "KneelDown" || GetAction() eq "KneelUp" || GetAction() eq "FlatUp")
    return(true);
  // Bei Dive �bergangsaktion
  if (GetAction() eq "Dive")
    {
    var iPhase = Min(8-GetPhase(), 6);
    SetAction("DiveBack");
    SetPhase(iPhase);
    return(true);
    }
  // Wenn bereits gezaubert wird, dann wird nur die Phase zurueckgesetzt,
  // als Ma�nahme gegen den AbortCall
  // Es gibt einen Zauberzeitbonus von zwei Frames extra!
  if(GetAction() eq "Magic" || GetAction() eq "JumpMagic")
  {
   SetPhase(2);
   return true;
  }
  // Aus allen anderen Situationen heraus
  return(SetAction("Magic"));
}


// Der normale Zauberclonk hat keine spezielle Schwebeaktion f�rs Zaubern
// Diese bringt erst der Magus mit
private func SetCastAction() {}

private func EndMagicAction()
{
  // Zielen abbrechen
  // Aimer zur�cksetzen, damit nicht rekursiv sofort wieder Aim gestartet wird...
  if(GetAction() S= "AimMagic")
    {
    var pAimerBck = pAimer; pAimer = 0;
    SetAction("Walk");
    pAimer = pAimerBck;
    return(true);
    }
  // Hat er �berhaupt eine Magieaktion?
  if (!IsCasting()) return();
  // Zur�ck zum Reiten
  if (IsRiding())
    SetAction("Ride");
  // Sonst zur�ck zum Gehen
  else
    SetAction("Walk");
}

// Zur Sicherheit, damit �berladungen keinen Fehler beim Starten geben
protected func Scaling()
{
  var szDesiredAction;
  if (GetYDir()>0) szDesiredAction = "ScaleDown"; else szDesiredAction = "Scale";
  if (GetAction() ne szDesiredAction) SetAction(szDesiredAction);
  return(1);   
}

protected func CheckStuck()
{                   
  // Verhindert Festh�ngen am Mittelvertex
  if(!GetXDir()) if(Abs(GetYDir()) < 5)
    if(GBackSolid(0, 3))
      SetPosition(GetX(), GetY() + 1);
}

/* ++++++ Men�kombokram +++++ */

/* Men�zeugs */

protected func QuickspellEditMenu(id idSpell, int iSelection) {
  // Clonk soll anhalten
  SetComDir(COMD_Stop());
  // Men� �ffnen
  OpenEmptySpellMenu(MCMQ, this, this, this, "$MnuNoSpells$");
  var idMagic, i = 0;
  // "Schnellzauber l�schen"
  if ((idSpell != MCM1) && (idSpell != MCM2) && (idSpell != MCM3) && (idSpell != MCM4) && (idSpell != MCM5))
    AddMenuItem(Format("$MnuSlotNumber$: $MnuDelQuickspell$", iSelection+1),"QuickspellDel",MCMX,0,0,iSelection);
  // Alles au�er schon ausgew�hlte anzeigen
  while (idMagic = GetPlrMagic(GetOwner(), 0, i++))
    if(!IsQuickspell(idMagic))
      AddMenuItem(Format("$MnuSlotNumber$: %s", iSelection+1, "%s"), "QuickspellAdd", idMagic,0,0,iSelection);
}

protected func QuickspellAdd(id idSpell, int iSelection) {
  AddQuickspell(idSpell, iSelection);
  ContextQuickspell(this(),iSelection);
}

protected func QuickspellDel(id idSpell, int iSelection) {
  DelQuickspell(idSpell, iSelection);
  ContextQuickspell(this(),iSelection);
}

/* Schnellzauber */

protected func AddQuickspell(id idSpell, int iPos)	// f�gt einen Zauber zum Schnellzugriff hinzu
{ SetPlrExtraData(GetOwner(),Format("%s%d",MCLK_QSPrefix(),iPos),idSpell); }

protected func DelQuickspell(id idSpell, int iPos)	// l�scht einen Zauber aus dem Schnellzugriff
{ SetPlrExtraData(GetOwner(),Format("%s%d",MCLK_QSPrefix(),iPos),false); }

private func MaxQuickspells() { return(15); }		// Maximale Anzahl der Zauber im Schnellzugriff

private func Quickspells() {
  for(var i=0; i<MaxQuickspells(); ++i)
    if(IsQSID(GetPlrExtraData(GetOwner(),Format("%s%d",MCLK_QSPrefix(),i)) ))
      return(true);
}

private func IsQuickspell(id idSpell)			// der angegebene Zauber ist immo im Schnellzugriff
{
  var idData;
  for(var i=0; IsQSID(idData = GetPlrExtraData(GetOwner(),Format("%s%d",MCLK_QSPrefix(),i)) ) && i<MaxQuickspells(); ++i)
    if(idData == idSpell) return(1);
  return(0);
}

private func IsQSID(id idItem) {				// Ist eine geladene ID und als Schnellzauber zul�ssig
  if(GetType(idItem) != C4V_C4ID()) return(false);
  if(!FindDefinition(idItem)) return(false);
  if(!GetPlrMagic(GetOwner(),idItem)) return(false);
  return(true);
}  

protected func NoComboReadyToMagic() { return(ReadyToMagic() && !iCombo); }
protected func NoCombo() { return(!iCombo); }
protected func ComboOn() { return(iCombo); }

protected func Nothing() {}


/* PhaseCall der Magieaktion */

private func CheckMagic()
{
  // Nur in Phase 5 zaubern
  iCurPhase = GetPhase();
  if(iCurPhase != 5) return();
  
  // Kein Zauber? :[
  if(!idCurSpell) return();
  var idSpell = idCurSpell;
  idCurSpell = 0;
 
  // Zauberspruch ausf�hren
  ExecMagic(idSpell);
}

// EndCall von Kneel: Nach fertigem Kneel/DiveBack eventuell geplanten Zauber zaubern
private func CheckMagicStart()
{
  if (idCurSpell)
    if (GetContact(0, -1, 8)) SetAction("Magic"); else SetAction("JumpMagic");
}

private func AfterMagic()
{
  // Aktuellen Spruch zur�cksetzen
  iCurPhase = 0;
  idCurSpell = 0;
  
  return(1);
}

// Action-AbortCall
private func AbortSpell()
  {
  if (!idCurSpell) return();
  iCurPhase=0; idCurSpell=0;
  DoCastError(Format("$MsgNotReadyToMagic$", GetName()));
  return(true);
  }

private func AfterMagicJump()
{
  if(idCurSpell && GetAction() S= "Walk")
  {
    SetAction("Magic");
    SetPhase(iCurPhase);
  }
  else AfterMagic();
}

protected func DoComboCasting()
  {
  // Schon beim Zaubern? Abbrechen
  if (IsCasting()) return(EndMagicAction());
  // Bereit?
  if (!ReadyToMagic()) return(0, DoCastError(Format("$MsgNotReadyToMagic$", GetName())));
  pComboMenu = CreateSpellMenu(this(), this(), 0, iCombo==2);
  if (!pComboMenu) return();
  // Clonk soll anhalten
  SetComDir(COMD_Stop());
  // Zauberaktivit�t setzen
  SetCastAction();
  // Auswahlmen� zur�ckgeben
  return(pComboMenu);
  }

public func OnComboMenuEnter(id idSpell)
  {
  // Spruch ausf�hren
  return(DoMagic(idSpell));
  }

public func OnComboMenuAbort()
  {
  // Redundant; das Men� ist eh gel�scht
  pComboMenu = 0;
  // Zaubern abbrechen
  EndMagicAction();
  }

public func OnComboMenuSpecial()
  {
  // Spezial im Komnbomen�: Klassenwahl (Momentan gar nix)
  return(ControlSpecialDouble());
  }


/* Zielsteuerung - nur aktiv, wenn das Fantasypack die globalen Funktionen CreateAimer und CreateSelector �berladen hat */

// StartCall von Walk, wechselt bei laufendem Aimer wieder in die Aim-Aktion
private func ReAim()
{
  if(pAimer && pAimedSpell)
    if(pAimer->GetID() == AIMR) // In pAimer wird auch der Selektor gespeichert
      AimingAngle(pAimer->~GetCurrentAngle() );
}

// Momentanen Zauber abbrechen
protected func AbortCasting(a,b,c)
  {
  if (pComboMenu) pComboMenu->Abort();
  return(_inherited(a,b,c));
  }


/* Magief�higkeit tempor�r geben (Aufgerufen vom Aufwertungszauber) */

public func OnRevaluation()
  {
  // Magief�higkeit geben
  MagicEnergy();
  // Kombo nach Spieler-Default
  if (IsComboSystemEnabled())
    iCombo = GetPlrExtraData(GetOwner(), MCLK_ComboExtraDataName());
  _inherited();
  }

public func MagicEnergy()
{
  // Normalerweise wird die Physical schon im Redefine2 gesetzt - dies ist ein Workaround f�r Aufwertungen von Clonks,
  // die kein #include CLNK im Script haben
  if (!GetPhysical("Magic")) SetPhysical("Magic", GetPhysical("Magic", 0, 0, GetID()), PHYS_Temporary);
  // Und gleich auff�llen
  for (var i = 0; i < 10; i++)
    DoMagicEnergy(10);
  return(1);
}


/* Feedback */

private func DoCastError(string sErr)
{
  PlayerMessage(GetController(), sErr,this());
  Sound("Error", 0, this(), 100, GetController()+1);
  return(true);
}

/* Status */

// Der normale Zauberclonk hat keine dauerhafte Cast-Aktion
public func IsCasting() { return(false); }  

/* Kombosystem ist nun immer aktiviert (Abwaertskompatibilitaet fuer Szenarien, die die Funbktion in einer Kombosystem-Ueberladung noch nutzen) */

global func IsComboSystemEnabled() { return(true); }
