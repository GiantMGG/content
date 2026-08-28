/*-- Kraftwerk --*/

#strict

#include DOOR
#include BAS4

static const BURN_NEED = 100;   // one COAL (value 100) = one burn cycle
local fuel_residual;            // Burn_Consume banks overshoot here

/* Initialisierung */

protected func Initialize()
{
  // Netterweise gibt's zu Beginn einmal Kohle
  CreateContents(COAL);
}

/* Eingangssteuerung */

private func SoundOpenDoor()
{
  Sound("GateOpen");
}

private func SoundCloseDoor()
{
  Sound("GateClose");
}
  
protected func Collection(object pObj)
{
  Sound("Clonk");
}

/* Produktion */

private func Burning()
{
  // Rauch
  Smoke(-15,-20,12);
  Smoke(-1,-23,8);
  // Energieerzeugung
  DoEnergy(+25);
  // Weiter
  if(GetActTime() < GetBurnTime()) return(1);
  // Fertig
  SetAction("Idle");
}

/* Inhalts�berpr�fung */

private func ContentsCheck()
{
  // Noch aktiv
  if(!ActIdle()) return(1);
  var i = -1, obj, def;
  while(obj = Contents(++i) )
  {
    if(obj->~IsLorry() )
    {
      SetCommand(obj, "Exit");
    }
  }

  // Erstes Inhaltsobjekt pr�fen
  obj = Contents(0);
  def = GetID(obj);
  
  // Alles au�er Brennmaterial raus
  if(def != COAL && def != OBRL && def != WOOD)
    SetCommand(obj, "Exit");
    
  // Opt-in overlay: FuelSystem.c4d geladen -> Bibliothek nutzen.
  if(DefinitionCall(FUEL, "IsFuel"))
  {
    if(CheckEnergyNeedChain() && Call("Burn_Consume", this, BURN_NEED))
      SetAction("Burning");
    return(1);
  }

  // Legacy-Pfad (FuelSystem.c4d nicht geladen) — unverändert.
  // Wird Energie ben�tigt
  if(!CheckEnergyNeedChain() ) return(1);
  
  // Holz?
  if(ContentsCount(WOOD) > 1)
    return(BurnWood() );
    
  // �l?
  if(ContentsCount(OBRL) )
    return(BurnOil() );
    
  // Kohle
  if(!(obj = FindContents(COAL)) ) return(1);
  RemoveObject(obj);
  SetAction("Burning");
}

private func BurnWood()
{
  // Zwei Holz l�schen
  for(var i = 0; i < 2; ++ i)
    RemoveObject(FindContents(WOOD) );

  // Los gehts
  SetAction("Burning");
}

private func BurnOil()
{
  // Fass entleeren
  var barrel = FindContents(OBRL);
  if(!barrel) return(0);
  ChangeDef(BARL, barrel);
  
  // Los geth.
  SetAction("Burning");
}

/* Brenndauer */
private func GetBurnTime() { return(2000); }
