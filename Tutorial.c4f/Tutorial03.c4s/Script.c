#strict 2
func Initialize()
{
	// Always have some music
	Music("Frontend", 1);
	MusicLevel(30);
	// Lens flare
	CreateObject(LENS, 70, 50);
	// Bird song
	CreateObject(BRDS); CreateObject(BRDS); CreateObject(BRDS);
	// Create goal
	CreateObject(SCRG);
  // Buy the lorry first
  goto(86);
	// Start script
  ScriptGo(1);
	// Evaluation dialog options
  SetNextMission("Tutorial.c4f\\Tutorial03.c4s", "$BtnRepeatRound$", "$BtnRepeatRoundDesc$");
}

func InitializePlayer(int plr)
{
	// Set message position
	SetTutorialMessagePos(MSG_Bottom | MSG_Left | MSG_XRel | MSG_WidthRel, 10, -50, 35);
	// One-time skip hint shown at the top of the screen
	CustomMessage("$TxtSkipHint$", GetCrew(plr), 0, 0, -120, 0xffffff, DECO, "", MSG_Top | MSG_XRel | MSG_WidthRel, 35);
}

// Chop tree and make wood

func Script1()
{
  TutorialMessage("$TxtWalkovertooneofthetr$");
  SetPlrShowControl(0,"___345678_   345678 ______6_8_");
  SetArrow(177, 228);
  wait(15);
  return(1);
}
func Script2()
{
  if (!FindObject(TRE2,GetX(GetHiRank(0)),GetY(GetHiRank(0)))) return(goto(2));
  if (GetComDir(GetHiRank(0))!=COMD_Stop()) return(goto(2));
  RemoveArrow();
  SetPlrShowControl(0,"___345678_   345678 _____5____");
  TutorialMessage("$TxtHitdigtwicequicklyto$");
  wait(15);
  return(1);
}
func Script3()
{
  if (GetAction(GetHiRank(0))eq"Chop") return(goto(30));
  if (GetAction(GetHiRank(0))eq"Dig") return(goto(20));
  return(goto(3));
}
func Script20()
{
  Sound("Error");
  SetPlrShowControl(0,"___345678_   345678 _______7__");
  TutorialMessage("$TxtThatwasnotquickenoug$");
  RemoveArrow();
  wait(15);
  return(1);
}
func Script21()
{
  if (GetAction(GetHiRank(0))eq"Dig") return(goto(21));
  return(1);
}
func Script22()
{
  SetPlrShowControl(0,"___345678_   345678 _____5____");
  TutorialMessage("$TxtWalkbacktothetreeand$");
  wait(15);
  SetArrow(177, 228);
  return(goto(3));
}
func Script30()
{
  SetPlrShowControl(0,"___345678_   345678 __________");
  TutorialMessage("");
  RemoveArrow();
  return(1);
}
func Script31()
{
  if (!(GetCategory(FindObject(TRE2))&C4D_Vehicle())) return(goto(31));
  SetPlrShowControl(0,"___345678_   345678 _______7__");
  TutorialMessage("$TxtGrabthetreewithadoub$");
  wait(8);
  return(goto(50));
}
func Script50()
{
  if (GetAction(GetHiRank(0))ne"Push") return(goto(50));
  if (GetID(GetActionTarget(0,GetHiRank(0)))!=TRE2) return(goto(50));
  SetPlrShowControl(0,"___345678_   345678 ______6_8_");
  TutorialMessage("$TxtPushthetreeinfrontof$");
  wait(12);
  SetArrow(237, 263);
  return(1);
}
func Script51()
{
  var pTree=FindObject(TRE2);
  if (!FindObject(SAWM,GetX(pTree),GetY(pTree))) return(goto(51));
  RemoveArrow();
  SetPlrShowControl(0,"___345678_   345678 ____4_6_8_");
  TutorialMessage("$TxtHituptopushthetreein$");
  wait(15);
  return(1);
}
func Script52()
{
  if (!FindObject(WOOD)) return(goto(52));
  SetPlrShowControl(0,"___345678_   345678 __________");
  TutorialMessage("");
  return(1);
}
func Script60()
{
	TutorialMessage("$TxtTheSawmillProcessesWood$");
	return(0);
}
func Script85()
{
	// Cheap trick: magic all wood into the lorry
	var pWood;
	for (pWood in FindObjects(Find_ID(WOOD)))
		if (GetID(pWood->Contained()) != LORY)
			pWood->Enter(FindObject(LORY));			
	return(goto(300));

// Buy lorry

}
func Script90()
{
  SetPlrShowControl(0,"___345678_   345678 ____4_____");
  TutorialMessage("$TxtNowtakeyourclonkback$");
  wait(15);
  return(1);
}
func Script91()
{
  if (!Contained(GetHiRank(0))) return(goto(91));
  SetPlrShowControl(0,"___345678_   345678 ___3___7__");
  TutorialMessage("$TxtActivatethebuymenuCo$");
  wait(12);
  return(1);
}
func Script92()
{
	// No menu open: wait
  if (!GetMenu(GetHiRank(0))) return(goto(92));
  // Basic structure menu: wait
  if (GetMenu(GetHiRank(0)) == 14) return(goto(92));
  // Got the buy menu!
  if (GetMenu(GetHiRank(0)) == 4) return(goto(100));
  // Otherwise: wrong menu
  Sound("Error");
  TutorialMessage("$TxtThatisthewrongmenuHi$");
  SetPlrShowControl(0,"___345678_   345678 _____5____");
  wait(20);
  return(1);
}
func Script93()
{
	// Not back in structure menu yet: wait
  if (GetMenu(GetHiRank(0)) != 14) return(goto(93));
  SetPlrShowControl(0,"___345678_   345678 ___3___7__");
  TutorialMessage("$TxtActivatethebuymenuCo$");
  wait(10);
  return(goto(92));
}
func Script99()
{
  Sound("Ding");
  wait(5);
  return(1);
}
func Script100()
{
  SetPlrShowControl(0,"___345678_   345678 ___3______");
  TutorialMessage("$TxtBuyalorryCommandthro$");
  wait(8);
  return(1);
}
func Script101()
{
  if (!FindObject(LORY)) return(goto(101));
  if (!GetMenu(GetHiRank(0))) return(goto(103));
  TutorialMessage("$TxtHitdigtoclosethemenu$");
  SetPlrShowControl(0,"___345678_   345678 _____5____");
  wait(8);
  return();
}
func Script102()
{
  // Not back in structure menu: wait
  if (GetMenu(GetHiRank(0)) != 14) return(goto(102));
  return();
}
func Script103()
{
  SetPlrShowControl(0,"___345678_   345678 ___3___7__");
  TutorialMessage("$TxtNowchoosetheactivati$");
  wait(12);
  return(1);
}
func Script104()
{
	// No menu open: wait
  if (!GetMenu(GetHiRank(0))) return(goto(104));
  // Basic structure menu: wait
  if (GetMenu(GetHiRank(0)) == 14) return(goto(104));
	// Got the contents menu!
  if ( GetMenu(GetHiRank(0)) == 18) return(goto(125));
  // Otherwise: wrong menu
  Sound("Error");
  TutorialMessage("$TxtThatisthewrongmenuHi$");
  SetPlrShowControl(0,"___345678_   345678 _____5____");
  wait(15);
  return(1);
}
func Script105()
{
	// Not back in structure menu yet: wait
  if (GetMenu(GetHiRank(0)) != 14) return(goto(105));
  SetPlrShowControl(0,"___345678_   345678 ___3___7__");
  TutorialMessage("$TxtNowchoosetheactivati$");
  wait(12);
  return(goto(104));
}
func Script125()
{
  Sound("Ding");
  SetPlrShowControl(0,"___345678_   345678 ___3______");
  TutorialMessage("$TxtActivatethelorryCont$");
  wait(10);
  return(1);
}
func Script126()
{
  if (Contained(FindObject(LORY))) return(goto(126));
  SetPlrShowControl(0,"___345678_   345678 _____5____");
  TutorialMessage("$TxtClosethemenuandleave$");
  wait(10);
  return(1);
}
func Script127()
{
	// Clonk still in the hut
  if (Contained(GetHiRank(0))) return(goto(127));
  return(goto(200));

// Move lorry to sawmill

}
func Script200()
{
  TutorialMessage("$TxtNowgrabthelorrywitha$");
  SetPlrShowControl(0,"___345678_   345678 _______7__");
  wait(10);
  return();
}
func Script201()
{
  if ((GetAction(GetHiRank(0)) ne "Push") || (GetID(GetActionTarget(0,GetHiRank(0))) != LORY)) return(goto(201));
  return();
}
func Script202()
{
  TutorialMessage("$TxtMoveLorryToSawmill$");
  SetPlrShowControl(0,"___345678_   345678 ______6_8_");
  SetArrow(205, 250);
  wait(10);
  return();
}
func Script203()
{
	if (!FindObject(LORY, 206, 267)) return(goto(203));
	RemoveArrow();
	return();
}
func Script204()
{
  TutorialMessage("$TxtLetGoOfTheLorry$");
  SetPlrShowControl(0,"___345678_   345678 _______7__");
  wait(10);
  return();
}
func Script205()
{
  if (GetAction(GetHiRank(0)) eq "Push") return(goto(205));
  return();
}
func Script206()
{
	return(goto(1));

// Get the ore

}
func Script300()
{
	TutorialMessage("");
	return();
}
func Script305()
{
	TutorialMessage("$TxtDigOutTheChunkOfOre$");
  SetArrowToObj(CreateObject(ORE1, 521, 302, -1));
  wait(15);
	return();
}
func Script306()
{
	if (!GetCrew()->ContentsCount(ORE1)) goto(306);
	return();
}
func Script307()
{
	TutorialMessage("$TxtThrowthechunkoforein$");
	SetArrowToObj(FindObject(LORY));
  wait(15);
	return();
}
func Script308()
{
	if (!FindObject(LORY)->ContentsCount(ORE1)) goto(308);
	return();
}
func Script309()
{
  if (FindObject(LORY)->ContentsCount(WOOD) < 2) goto(310);
  else goto(315);
	return();
}
func Script310()
{
	TutorialMessage("$TxtCollectMoreWood$");
  wait(15);
	return(goto(309));
}
func Script315()
{
	TutorialMessage("$TxtMovethelorryintotheg$");
	SetArrow(368, 264);
  wait(15);
	return();
}
func Script316()
{
	if (!FindObject(LORY)->Contained()) goto(316);
	return();
}
func Script317()
{
  TutorialMessage("$TxtThefoundryturnswoodw$");
	RemoveArrow();
	return();	
}
func Script318()
{
  if (!FindObject(METL)) goto(318);
  return();
}
func Script319()
{
  TutorialMessage("$TxtMetalisneededtobuild$");
  RemoveArrow();
  return();
}
func Script325()
{
  FindObject(SCRG)->Fulfill();
  SetNextMission("Tutorial.c4f\\Tutorial04.c4s", "$BtnNextTutorial$", "$BtnNextTutorialDesc$");
  return();
}
