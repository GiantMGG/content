/*-- Hoerx.c4d -- King redef (mirrors Castle.c4s local-def pattern). --*/

#strict 2

#include KING  // inherit base King behavior

/* On death, log + eliminate. The SiegeDirector detects the death
   independently via FindObject(KING), so no scenario-level hook
   is required here. */
