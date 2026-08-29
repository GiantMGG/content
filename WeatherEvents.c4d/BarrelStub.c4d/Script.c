/*-- BarrelStub: provides GetBarrelType() so ObjectInsertMaterial in
    planet/System.c4g/C4.c resolves when only Agriculture + WeatherEvents
    are loaded (no Objects.c4d). Scoped to WeatherEvents.c4d so it does
    not leak into scenarios that don't load WeatherEvents. --*/

#strict 3

global func GetBarrelType() { return 0; }
public func BarrelDoFill(int iChange, int iMat) { return false; }
