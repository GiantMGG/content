/*-- CoastalMapLib (CMLB) -- map-preset ScriptAlgo* callbacks + post-gen stamp. --*/
/* The algo=script overlays in Coastal.c4d/Landscape.txt invoke the global       */
/* ScriptAlgoCoast/Channel/Flats functions below (driver:                        */
/* LegacyClonk/src/C4MapCreatorS2.cpp:1465-1482). Callbacks receive (x, y,       */
/* alpha, beta) in map-pixel space and return bool (true = inside overlay).      */
/*                                                                                */
/* OnMapGenerated is NOT an engine callback; scenarios call it from Initialize()  */
/* after the map exists to stamp a default SeaLaneMarker (SLMR) pair + Dock       */
/* (DKST) foundation at the harbor bay. It no-ops gracefully if OceanTrade.c4d    */
/* (SLMR) is absent — uses C4Id("…") so it compiles without that pack loaded.     */

#strict

/* ---- Pixel callbacks (called per pixel by the map creator) ---- */

global func ScriptAlgoCoast(int iX, int iY, int iAlpha, int iBeta)
{
	// Irregular coastline at ~y=20 (of a 40-tall map), sin-perturbed.
	// Water below the coastline (overlay paints Water where this is true).
	var iCoastY = 20 + Sin(iX * 12, 6);
	return iY > iCoastY;
}

global func ScriptAlgoChannel(int iX, int iY, int iAlpha, int iBeta)
{
	// Deep-water shipping channel: a vertical band near x=50 from the
	// lower map edge inward toward the harbor bay.
	return (iX > 45 && iX < 55) && (iY > 25);
}

global func ScriptAlgoFlats(int iX, int iY, int iAlpha, int iBeta)
{
	// Tidal flats: a 3px Sand band sitting just above the coastline.
	var iCoastY = 20 + Sin(iX * 12, 6);
	return (iY > iCoastY - 3) && (iY < iCoastY);
}

/* ---- Post-generation stamp (scenarios call this from Initialize) ---- */

global func OnMapGenerated()
{
	var iW = LandscapeWidth();
	var iH = LandscapeHeight();
	var iBayX = iW / 2;
	var iBayY = iH - 40;
	var idSLMR = C4Id("SLMR");
	var idDKST = C4Id("DKST");

	if (idSLMR && !FindObject(idSLMR))
	{
		CreateObject(idSLMR, iBayX - 60, iBayY, NO_OWNER);
		CreateObject(idSLMR, iBayX + 60, iBayY, NO_OWNER);
	}
	if (idDKST && !FindObject(idDKST))
		CreateObject(idDKST, iBayX, iBayY, NO_OWNER);
}
