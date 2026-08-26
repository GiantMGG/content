#strict 2
func Initialize()
{
	// Always have some music
	Music("Frontend", 1);
	MusicLevel(30);
	// Glint
	//CreateObject(EGLN);
	// Lens flare
	CreateObject(LENS, 500, 50);
	// Bird song
	//CreateObject(BRDS); CreateObject(BRDS); CreateObject(BRDS);
	// Create goal
	CreateObject(SCRG);
	// Clear some grass
	var pGrass;
  while (pGrass = FindObject(GRAS,490,240,30,30))
  	pGrass->RemoveObject();
	// Start script
  ScriptGo(1);
	// Evaluation dialog options
  SetNextMission("Tutorial.c4f\\Tutorial04.c4s", "$BtnRepeatRound$", "$BtnRepeatRoundDesc$");
}

func InitializePlayer(int plr)
{
	// Set message position
	SetTutorialMessagePos(MSG_Bottom | MSG_Left | MSG_XRel | MSG_WidthRel, 10, -30, 35);
	// One-time skip hint shown at the top of the screen
	CustomMessage("$TxtSkipHint$", GetCrew(plr), 0, 0, -120, 0xffffff, DECO, "", MSG_Top | MSG_XRel | MSG_WidthRel, 35);
}

func Script1()
{
  goto(10);
  return(1);
}
func Script10()
{
  SetPlrShowControl(0,"___345678_   345678 ____4_____");
  TutorialMessage("$TxtEnteryourhomebase$");
  wait(9);
  return(1);
}
func Script11()
{
  if (!Contained(GetHiRank(0))) return(goto(11));
  SetPlrShowControl(0,"___345678_   345678 ___3______");
  TutorialMessage("$TxtChoosetheactivationm$");
  wait(11);
  return(1);
}
func Script12()
{
	// No menu open: wait
  if (!GetMenu(GetHiRank(0))) return(goto(12));
  // Basic structure menu: wait
  if (GetMenu(GetHiRank(0)) == 14) return(goto(12));
	// Got the contents menu!
  if (GetMenu(GetHiRank(0)) == 18) return(goto(25));
  // Otherwise: wrong menu
  Sound("Error");
  TutorialMessage("$TxtThatisthewrongmenuCl$");
  wait(16);
  SetPlrShowControl(0,"___345678_   345678 _____5____");
  return(1);
}
func Script13()
{
	// Not back in structure menu yet: wait
  if (GetMenu(GetHiRank(0)) != 14) return(goto(13));
  SetPlrShowControl(0,"___345678_   345678 ___3______");
  TutorialMessage("$TxtChoosetheactivationm$");
  wait(10);
  return(goto(12));
}
func Script25()
{
  Sound("Ding");
  SetPlrShowControl(0,"___345678_   345678 ___3______");
  TutorialMessage("$TxtActivatetheconstruct$");
  wait(10);
  return(1);
}
func Script26()
{
	// Clonk doesn't have conkit yet
	if (!GetCrew()->FindContents(CNKT)) return(goto(26));
  SetPlrShowControl(0,"___345678_   345678 _____5____");
  TutorialMessage("$TxtClosethemenuandleave$");
  wait(8);
  return(1);
}
func Script27()
{
	// Clonk still in hut
  if (Contained(GetHiRank(0))) return(goto(27));
  TutorialMessage("$TxtWiththeconstructionk$");
  SetArrow(500, 250);
  wait(16);
  return(1);
}
func Script28()
{
	// Clonk not there yet
  if (!FindObject(0,320,0,200,1000,OCF_Living())) return(goto(28));
  RemoveArrow();
  TutorialMessage("$TxtHitdigtwicequicklyto$");
  SetPlrShowControl(0,"___345678_   345678 _____5____");
  wait(14);
  return(1);
}
func Script29()
{
  if (GetMenu(GetHiRank(0)) == 1) return(goto(130));
  if (GetMenu(GetHiRank(0)) == CXCN) return(goto(130));
  if (GetAction(GetHiRank(0)) eq "Dig") return(goto(80));
  return(goto(29));
}
func Script80()
{
  Sound("Error");
  SetPlrShowControl(0,"___345678_   345678 _______7__");
  TutorialMessage("$TxtThatwasnotquickenoug$");
  wait(12);
  return(1);
}
func Script81()
{
  if (GetAction(GetHiRank(0))eq"Dig") return(goto(81));
  return(1);
}
func Script82()
{
  SetPlrShowControl(0,"___345678_   345678 _____5____");
  TutorialMessage("$TxtNowwalkbacktothesurf$");
  wait(14);
  return(goto(29));
}
func Script130()
{
  Sound("Ding");
  SetPlrShowControl(0,"___345678_   345678 ___3______");
  TutorialMessage("$TxtCreateanelevatorcons$");
  wait(10);
  return(1);
}
func Script131()
{
  if (!FindObject(ELEV)) return(goto(131));
  SetPlrShowControl(0,"___345678_   345678 _______7__");
  TutorialMessage("$TxtAttheconstructionsit$");
  wait(10);
  return(1);
}
func Script132()
{
  if (GetAction(GetHiRank(0))ne"Build") return(goto(132));
  SetPlrShowControl(0,"___345678_   345678 __________");
  TutorialMessage("");
  return(1);
}
func Script133()
{
  if (!FindObject(ELEC)) return(goto(133));
  Sound("Applause");
  return(1);
}
func Script134()
{
  SetPlrShowControl(0,"___345678_   345678 _______7__");
  TutorialMessage("$TxtGrabtheelevatorcasew$");
  wait(10);
  return(1);
}
func Script135()
{
  if (GetAction(GetHiRank(0)) ne "Push") return(goto(135));
  if (GetID(GetActionTarget(0,GetHiRank(0))) != ELEC) return(goto(135));
  SetPlrShowControl(0,"___345678_   345678 _____5____");
  TutorialMessage("$TxtUseadoubledigtoactiv$");
  wait(12);
  return(1);
}
func Script136()
{
  // Wait until clonk at bottom of shaft
  if (!FindObject(0,470,340,80,50,OCF_Living())) return(goto(136));
  return(goto(150));
}
func Script150()
{
  SetPlrShowControl(0,"___345678_   345678 ____4_____");
  TutorialMessage("$TxtNowyoucanridetheelev$");
  wait(17);
  return(goto(170));
}
func Script170()
{
  // Wait until clonk back at surface
  if (!FindObject(0,470,240,80,30,OCF_Living())) return(goto(170));  
  SetPlrShowControl(0,"___345678_   345678 _______7__");
  TutorialMessage("$TxtLetgooftheelevatorca$");
  wait(14);
  return(1);
}
func Script171()
{
	// Wait until let go
  if (GetAction(GetHiRank(0)) eq "Push") return(goto(171));
  SetPlrShowControl(0,"___345678_   345678 __________");
  return(1);
}
func Script172()
{
  var pFlint=CreateContents(TFLN,FindObject(HUT2));
  SetCommand(pFlint,"Exit");
  SetArrow(574, 259);
  TutorialMessage("$TxtWalkbacktoyourhomeba$");
  wait(13);
  return(1);
}
func Script173()
{
  if (GetID(Contents(0,GetHiRank(0))) != TFLN) return(goto(173));
  TutorialMessage("$TxtRidebackdowntheshaft$");
  SetArrow(495,380);
  wait(12);
  return(1);
}
func Script174()
{
  // Wait until clonk at bottom of shaft
  if (!FindObject(0,470,340,80,50,OCF_Living())) return(goto(174));
	TutorialMessage("$TxtDigATunnelToTheGold$");
  SetPlrShowControl(0,"___345678_   345678 _____5____");
	SetArrow(392, 388);
  wait(10);
	return(1);
}
func Script175()
{
	// Wait until clonk near gold
  if (!FindObject(0,397-40,388-40,80,80,OCF_Living())) return(goto(175));
  SetPlrShowControl(0,"___345678_   345678 ___3______");
  TutorialMessage("$TxtDiguntilyoumakeconta$");
  wait(5);
  RemoveArrow();
  return(1);
}
func Script176()
{
	// Got some gold!
  if (FindObject(GOLD)) return(goto(180));
  // Wasted t-flint
  if (!FindObject(TFLN)) return(goto(200));
  // Waiting
  return(goto(176));
}
func Script180()
{
  SetPlrShowControl(0,"___345678_   345678 __________");
  Sound("Applause");
  TutorialMessage("");
  return(1);
}
func Script182()
{
  TutorialMessage("$TxtExcellent$");
	return(1);
}
func Script185()
{
  TutorialMessage("");
	return(1);
}
func Script200()
{
  SetPlrShowControl(0,"___345678_   345678 __________");
  CreateContents(TFLN,FindObject(HUT2));
  CreateContents(TFLN,FindObject(HUT2));
  CreateContents(TFLN,FindObject(HUT2));
  TutorialMessage("$TxtYouwillfindmoreTFlin$");
  wait(30);
  return(1);
}
func Script201()
{
	// Wait until clonk back in hut
  if (!Contained(GetHiRank(0))) return(goto(201));
  TutorialMessage("$TxtUsetheactivationmenu$");
  return(1);
}
func Script250()
{
  TutorialMessage("$TxtGain25goldpointstoco$");
  return(1);
}
func Script251()
{
  if (GetWealth(0)<25) return(goto(251));
  FindObject(SCRG)->Fulfill();
  SetNextMission("Tutorial.c4f\\Tutorial05.c4s", "$BtnNextTutorial$", "$BtnNextTutorialDesc$");
  return(1);
  }
