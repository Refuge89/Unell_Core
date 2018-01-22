#include "Unell_Script.h"

class Unell_PvP_Announce : public PlayerScript
{
public:
    Unell_PvP_Announce() : PlayerScript("Unell_PvP_Announce") {}

    void OnPVPKill(Player * Killer, Player * Victim)
    {
		if (sConfigMgr->GetBoolDefault("Unell.PvP.Announce.Kill.Enable", true))
		{
			char msg[250];
			uint32 maxValue = Victim->GetMaxHealth();
			
			std::string NameLinkK = ChatHandler(Killer->GetSession()).GetNameLink(Killer);
			std::string NameLinkV = ChatHandler(Victim->GetSession()).GetNameLink(Victim);
			
			uint32 PvPZoneID = sConfigMgr->GetIntDefault("Unell.PvP.Announce.Kill.ZoneID", 17);
			
			if(Killer->GetZoneId() == PvPZoneID)
			{
				snprintf(msg, 250, "|cffff0000[Анонс убийства]:|r |cff6C8CD5Игрок |cff14ECCF%s |cff6C8CD5убил игрока |cff14ECCF%s|cff6C8CD5.", NameLinkK.c_str(), NameLinkV.c_str());
				sWorld->SendZoneText(17, msg);
			}
		}
	}  
};

class Unell_PvP_Antifarm : public PlayerScript
{
public:
    Unell_PvP_Antifarm() : PlayerScript("Unell_PvP_Antifarm") {}

    void OnPVPKill(Player * Killer, Player * Victim)
    {
		char msg[250];
		uint32 maxValue = Victim->GetMaxHealth();
		std::string NameLinkK = ChatHandler(Killer->GetSession()).GetNameLink(Killer);
		
		if (sConfigMgr->GetBoolDefault("Unell.PvP.Antifarm.Enable", true))
		{
			uint32 minhealth = sConfigMgr->GetIntDefault("Unell.PvP.Antifarm.Min.Health", 15000);
			
			if(maxValue <= minhealth)
			{
				if (sConfigMgr->GetBoolDefault("Unell.PvP.Antifarm.Punishment.Victim", true))
				{
					Victim->ResurrectPlayer(1.0f);
					Victim->SpawnCorpseBones();
					Victim->AddAura(31797, Victim);
					Victim->AddAura(9036, Victim);
					Victim->GiveLevel(1);
					Victim->InitTalentForLevel();
					Victim->SetUInt32Value(PLAYER_XP, 0);
				}

				QueryResult result = CharacterDatabase.PQuery("SELECT guid FROM characters WHERE name = '%s'", Killer->GetName().c_str());
				if(result)
				{				
					Field * fields = result->Fetch();
					uint32 guid = fields[0].GetUInt32();
					
					QueryResult result1  = CharacterDatabase.PQuery("SELECT warnings FROM antipvp WHERE id = '%u'", guid);
					if (result1)
					{
						CharacterDatabase.PExecute("UPDATE `antipvp` SET `warnings`=`warnings`+1 WHERE (`id`='%u')", guid);
						QueryResult result2 = CharacterDatabase.PQuery("SELECT warnings FROM antipvp WHERE id = '%u'", guid);
						
						if (result2)
						{
							Field * fields = result2->Fetch();
							uint32 warnings = fields[0].GetUInt32();
							
							if (warnings == 2)
							{
								snprintf(msg, 250, "|cffff0000[Хищник]:|r |cff6C8CD5Игрок |cff14ECCF%s |cff6C8CD5уже неоднократно убивал слабого игрока и был наказан до прихода ГМ", NameLinkK.c_str());
								sWorld->SendServerMessage(SERVER_MSG_STRING, msg);
								Killer->AddAura(42201, Killer);
								Killer->AddAura(38505, Killer);
								Killer->AddAura(47543, Killer);
								Killer->AddAura(58540, Killer);
								Killer->AddAura(41083, Killer);
								Killer->AddAura(56756, Killer);
								Killer->AddAura(39258, Killer);
								Killer->AddAura(31797, Killer);
								Killer->AddAura(39594, Killer);
								Killer->AddAura(23775, Killer);
								Killer->AddAura(2647, Killer);
							}
							
							if (warnings >= 3)
							{
								std::string PerBan = "-1";
								snprintf(msg, 250, "|cffff0000[Хищник]:|r |cff6C8CD5Игрок |cff14ECCF%s |cff6C8CD5многократно убивал слабых игроков и был забанен", NameLinkK.c_str());
								sWorld->SendServerMessage(SERVER_MSG_STRING, msg);
								Killer->GetSession()->KickPlayer();
								sWorld->BanAccount(BAN_CHARACTER, Killer->GetName(), PerBan.c_str(), "AntiPvP AutoBan", "AntiPvP AutoBan");							
								CharacterDatabase.PExecute("UPDATE `antipvp` SET `warnings`= 0 WHERE (`id`='%u')", guid);							
							}	
						}
					}
					if (!result1)
					{
						CharacterDatabase.PExecute("INSERT INTO `antipvp` (`id`, `warnings`) VALUES ('%u', '1')", guid);					
					}				
					
				}			
			}
		}		
	}  
};

void AddSC_Unell_PvP_System()
{
	new Unell_PvP_Announce();
	new Unell_PvP_Antifarm();
}