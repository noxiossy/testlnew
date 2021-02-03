#include "stdafx.h"
#include "UIMainIngameWnd.h"
#include "UIMessagesWindow.h"
#include "../UIZoneMap.h"
#include <dinput.h>
#include "../actor.h"
#include "../ActorCondition.h"
#include "../EntityCondition.h"
#include "../CustomOutfit.h"
#include "../ActorHelmet.h"
#include "../PDA.h"
#include "../xrServerEntities/character_info.h"
#include "../inventory.h"
#include "../UIGameSP.h"
#include "../weaponmagazined.h"
#include "../missile.h"
#include "../Grenade.h"
#include "../xrServerEntities/xrServer_objects_ALife.h"
#include "../alife_simulator.h"
#include "../alife_object_registry.h"
#include "../game_cl_base.h"
#include "../level.h"
#include "../seniority_hierarchy_holder.h"
#include "../date_time.h"
#include "../xrServerEntities/xrServer_Objects_ALife_Monsters.h"
#include "../../xrEngine/LightAnimLibrary.h"
#include "UIInventoryUtilities.h"
#include "UIHelper.h"
#include "UIMotionIcon.h"
#include "UIXmlInit.h"
#include "UIPdaMsgListItem.h"
#include "UIPdaWnd.h"
#include "../alife_registry_wrappers.h"
#include "../string_table.h"
#ifdef DEBUG
#	include "../attachable_item.h"
#	include "../../xrEngine/xr_input.h"
#endif
#include "UIScrollView.h"
#include "map_hint.h"
#include "../game_news.h"
#include "static_cast_checked.hpp"
#include "game_cl_capture_the_artefact.h"
#include "UIHudStatesWnd.h"
#include "UIActorMenu.h"
#include "../../Include/xrRender/Kinematics.h"
#include "../../xrEngine/CustomHUD.h"

void test_draw	();
void test_key	(int dik);

using namespace InventoryUtilities;
//BOOL		g_old_style_ui_hud			= FALSE;
const u32	g_clWhite					= 0xffffffff;

#define		DEFAULT_MAP_SCALE			1.f

#define		C_SIZE						0.025f
#define		NEAR_LIM					0.5f

#define		SHOW_INFO_SPEED				0.5f
#define		HIDE_INFO_SPEED				10.f
#define		C_ON_ENEMY					D3DCOLOR_XRGB(0xff,0,0)
#define		C_DEFAULT					D3DCOLOR_XRGB(0xff,0xff,0xff)


CUIMainIngameWnd::CUIMainIngameWnd()
:/*m_pGrenade(NULL),m_pItem(NULL),*/m_pPickUpItem(NULL),m_pMPChatWnd(NULL),m_pMPLogWnd(NULL)
{
	UIZoneMap					= xr_new<CUIZoneMap>();
}

#include "UIProgressShape.h"
extern CUIProgressShape* g_MissileForceShape;

CUIMainIngameWnd::~CUIMainIngameWnd()
{
	DestroyFlashingIcons		();
	xr_delete					(UIZoneMap);
	HUD_SOUND_ITEM::DestroySound(m_contactSnd);
	xr_delete					(g_MissileForceShape);
}

void CUIMainIngameWnd::Init()
{
	CUIXml						uiXml;
	
	BOOL  lr_alt_back_hud = ( psHUD_Flags.is(HUD_LR_ALT_BACK_HUD) );
	BOOL  lr_alt_back_hud_2 = ( psHUD_Flags.is(HUD_LR_ALT_BACK_HUD_2) );
		
	if(lr_alt_back_hud)
	{
		uiXml.Load					(CONFIG_PATH, UI_PATH, "maingame_alt.xml");
	}
	else if(lr_alt_back_hud_2)
	{
		uiXml.Load					(CONFIG_PATH, UI_PATH, "maingame_alt_2.xml");
	}
	else
	{
		uiXml.Load					(CONFIG_PATH, UI_PATH, "maingame.xml");
	}
		
	CUIXmlInit					xml_init;
	xml_init.InitWindow			(uiXml,"main",0,this);

	Enable(false);

	UIPickUpItemIcon			= UIHelper::CreateStatic		(uiXml, "pick_up_item", this);
	UIPickUpItemIcon->SetShader	(GetEquipmentIconsShader());

	m_iPickUpItemIconWidth		= UIPickUpItemIcon->GetWidth();
	m_iPickUpItemIconHeight		= UIPickUpItemIcon->GetHeight();
	m_iPickUpItemIconX			= UIPickUpItemIcon->GetWndRect().left;
	m_iPickUpItemIconY			= UIPickUpItemIcon->GetWndRect().top;
	//---------------------------------------------------------

	//индикаторы 
	UIZoneMap->Init				();

	// Flashing icons initialize
	uiXml.SetLocalRoot						(uiXml.NavigateToNode("flashing_icons"));
	InitFlashingIcons						(&uiXml);

	uiXml.SetLocalRoot						(uiXml.GetRoot());
	
	UIMotionIcon							= xr_new<CUIMotionIcon>(); UIMotionIcon->SetAutoDelete(true);
	UIZoneMap->MapFrame().AttachChild		(UIMotionIcon);
	UIMotionIcon->Init						(UIZoneMap->MapFrame().GetWndRect());

	UIStaticDiskIO							= UIHelper::CreateStatic(uiXml, "disk_io", this);

	m_ui_hud_states							= xr_new<CUIHudStatesWnd>();
	m_ui_hud_states->SetAutoDelete			(true);
	AttachChild								(m_ui_hud_states);
	m_ui_hud_states->InitFromXml			(uiXml, "hud_states");

	for(int i=0; i<4; i++)
	{
		m_quick_slots_icons.push_back	(xr_new<CUIStatic>());
		m_quick_slots_icons.back()	->SetAutoDelete(true);
		AttachChild				(m_quick_slots_icons.back());
		string32 path;
		xr_sprintf				(path, "quick_slot%d", i);
		CUIXmlInit::InitStatic	(uiXml, path, 0, m_quick_slots_icons.back());
		xr_sprintf				(path, "%s:counter", path);
		UIHelper::CreateStatic	(uiXml, path, m_quick_slots_icons.back());
	}
	m_QuickSlotText1				= UIHelper::CreateTextWnd(uiXml, "quick_slot0_text", this);
	m_QuickSlotText2				= UIHelper::CreateTextWnd(uiXml, "quick_slot1_text", this);
	m_QuickSlotText3				= UIHelper::CreateTextWnd(uiXml, "quick_slot2_text", this);
	m_QuickSlotText4				= UIHelper::CreateTextWnd(uiXml, "quick_slot3_text", this);

	UIStaticQuickHelp			= UIHelper::CreateTextWnd(uiXml, "quick_info", this);

	uiXml.SetLocalRoot			(uiXml.GetRoot());

	m_ind_lr_health			= UIHelper::CreateStatic(uiXml, "indicator_health", this);

	m_ind_boost_psy			= UIHelper::CreateStatic(uiXml, "indicator_booster_psy", this);
	m_ind_boost_radia		= UIHelper::CreateStatic(uiXml, "indicator_booster_radia", this);
	m_ind_boost_chem		= UIHelper::CreateStatic(uiXml, "indicator_booster_chem", this);
	m_ind_boost_wound		= UIHelper::CreateStatic(uiXml, "indicator_booster_wound", this);
	m_ind_boost_weight		= UIHelper::CreateStatic(uiXml, "indicator_booster_weight", this);
	m_ind_boost_health		= UIHelper::CreateStatic(uiXml, "indicator_booster_health", this);
	m_ind_boost_power		= UIHelper::CreateStatic(uiXml, "indicator_booster_power", this);
	m_ind_boost_rad			= UIHelper::CreateStatic(uiXml, "indicator_booster_rad", this);
	m_ind_boost_psy			->Show(false);
	m_ind_boost_radia		->Show(false);
	m_ind_boost_chem		->Show(false);
	m_ind_boost_wound		->Show(false);
	m_ind_boost_weight		->Show(false);
	m_ind_boost_health		->Show(false);
	m_ind_boost_power		->Show(false);
	m_ind_boost_rad			->Show(false);

	HUD_SOUND_ITEM::LoadSound				("maingame_ui", "snd_new_contact", m_contactSnd, SOUND_TYPE_IDLE);
}

float UIStaticDiskIO_start_time = 0.0f;
void CUIMainIngameWnd::Draw()
{
	CActor* pActor		= smart_cast<CActor*>(Level().CurrentViewEntity());

	// show IO icon
	bool IOActive	= (FS.dwOpenCounter>0);
	if	(IOActive)	UIStaticDiskIO_start_time = Device.fTimeGlobal;

	if ((UIStaticDiskIO_start_time+1.0f) < Device.fTimeGlobal)	UIStaticDiskIO->Show(false); 
	else {
		u32		alpha			= clampr(iFloor(255.f*(1.f-(Device.fTimeGlobal-UIStaticDiskIO_start_time)/1.f)),0,255);
		UIStaticDiskIO->Show		( true  ); 
		UIStaticDiskIO->SetTextureColor(color_rgba(255,255,255,alpha));
	}
	FS.dwOpenCounter = 0;

	if(!IsGameTypeSingle())
	{
		float		luminocity = smart_cast<CGameObject*>(Level().CurrentEntity())->ROS()->get_luminocity();
		float		power = log(luminocity > .001f ? luminocity : .001f)*(1.f/*luminocity_factor*/);
		luminocity	= exp(power);

		static float cur_lum = luminocity;
		cur_lum = luminocity*0.01f + cur_lum*0.99f;
		UIMotionIcon->SetLuminosity((s16)iFloor(cur_lum*100.0f));
	}
	if ( !pActor || !pActor->g_Alive() ) return;

	UIMotionIcon->SetNoise((s16)(0xffff&iFloor(pActor->m_snd_noise*100)));

	UIMotionIcon->Draw();


	UIZoneMap->visible = true;
	UIZoneMap->Render();

	bool tmp = UIMotionIcon->IsShown();
	UIMotionIcon->Show(false);
	CUIWindow::Draw();
	UIMotionIcon->Show(tmp);

	RenderQuickInfos();		
}


void CUIMainIngameWnd::SetMPChatLog(CUIWindow* pChat, CUIWindow* pLog){
	m_pMPChatWnd = pChat;
	m_pMPLogWnd  = pLog;
}

void CUIMainIngameWnd::Update()
{
	CUIWindow::Update();
	CActor* pActor = smart_cast<CActor*>(Level().CurrentViewEntity());

	if ( m_pMPChatWnd )
		m_pMPChatWnd->Update();

	if ( m_pMPLogWnd )
		m_pMPLogWnd->Update();

	if ( !pActor )
		return;

	UIZoneMap->Update();

	UpdatePickUpItem			();

	if( Device.dwFrame % 10 )
		return;

	game_PlayerState* lookat_player = Game().local_player;
	if (Level().IsDemoPlayStarted())
	{
		lookat_player = Game().lookat_player();
	}
	
	UpdateMainIndicators();
}


void CUIMainIngameWnd::RenderQuickInfos()
{
	CActor* pActor		= smart_cast<CActor*>(Level().CurrentViewEntity());
	if (!pActor)
		return;

	static CGameObject *pObject			= NULL;
	LPCSTR actor_action					= pActor->GetDefaultActionForObject();
	UIStaticQuickHelp->Show				(NULL!=actor_action);

	if(NULL!=actor_action)
	{
		if(stricmp(actor_action,UIStaticQuickHelp->GetText()))
			UIStaticQuickHelp->SetTextST				(actor_action);
	}

	if(pObject!=pActor->ObjectWeLookingAt())
	{
		UIStaticQuickHelp->SetTextST				(actor_action?actor_action:" ");
		UIStaticQuickHelp->ResetColorAnimation	();
		pObject	= pActor->ObjectWeLookingAt	();
	}
}

void CUIMainIngameWnd::ReceiveNews(GAME_NEWS_DATA* news)
{
	VERIFY(news->texture_name.size());

	CurrentGameUI()->m_pMessagesWnd->AddIconedPdaMessage(news);
	CurrentGameUI()->UpdatePda();
}

void CUIMainIngameWnd::SetFlashIconState_(EFlashingIcons type, bool enable)
{
	// Включаем анимацию требуемой иконки
	FlashingIcons_it icon = m_FlashingIcons.find(type);
	R_ASSERT2(icon != m_FlashingIcons.end(), "Flashing icon with this type not existed");
	icon->second->Show(enable);
}

void CUIMainIngameWnd::InitFlashingIcons(CUIXml* node)
{
	const char * const flashingIconNodeName = "flashing_icon";
	int staticsCount = node->GetNodesNum("", 0, flashingIconNodeName);

	CUIXmlInit xml_init;
	CUIStatic *pIcon = NULL;
	// Пробегаемся по всем нодам и инициализируем из них статики
	for (int i = 0; i < staticsCount; ++i)
	{
		pIcon = xr_new<CUIStatic>();
		xml_init.InitStatic(*node, flashingIconNodeName, i, pIcon);
		shared_str iconType = node->ReadAttrib(flashingIconNodeName, i, "type", "none");

		// Теперь запоминаем иконку и ее тип
		EFlashingIcons type = efiPdaTask;

		if		(iconType == "pda")		type = efiPdaTask;
		else if (iconType == "mail")	type = efiMail;
		else	R_ASSERT(!"Unknown type of mainingame flashing icon");

		R_ASSERT2(m_FlashingIcons.find(type) == m_FlashingIcons.end(), "Flashing icon with this type already exists");

		CUIStatic* &val	= m_FlashingIcons[type];
		val			= pIcon;

		AttachChild(pIcon);
		pIcon->Show(false);
	}
}

void CUIMainIngameWnd::DestroyFlashingIcons()
{
	for (FlashingIcons_it it = m_FlashingIcons.begin(); it != m_FlashingIcons.end(); ++it)
	{
		DetachChild(it->second);
		xr_delete(it->second);
	}

	m_FlashingIcons.clear();
}

void CUIMainIngameWnd::UpdateFlashingIcons()
{
	for (FlashingIcons_it it = m_FlashingIcons.begin(); it != m_FlashingIcons.end(); ++it)
	{
		it->second->Update();
	}
}

void CUIMainIngameWnd::AnimateContacts(bool b_snd)
{
	UIZoneMap->Counter_ResetClrAnimation();

	if(b_snd)
		HUD_SOUND_ITEM::PlaySound	(m_contactSnd, Fvector().set(0,0,0), 0, true );

}


void CUIMainIngameWnd::SetPickUpItem	(CInventoryItem* PickUpItem)
{
	m_pPickUpItem = PickUpItem;
};

void CUIMainIngameWnd::UpdatePickUpItem	()
{
	if (!m_pPickUpItem || !Level().CurrentViewEntity() || !smart_cast<CActor*>(Level().CurrentViewEntity())) 
	{
		UIPickUpItemIcon->Show(false);
		return;
	};


	shared_str sect_name	= m_pPickUpItem->object().cNameSect();

	//properties used by inventory menu
	int m_iGridWidth	= pSettings->r_u32(sect_name, "inv_grid_width");
	int m_iGridHeight	= pSettings->r_u32(sect_name, "inv_grid_height");

	int m_iXPos			= pSettings->r_u32(sect_name, "inv_grid_x");
	int m_iYPos			= pSettings->r_u32(sect_name, "inv_grid_y");

	float scale_x = m_iPickUpItemIconWidth/
		float(m_iGridWidth*INV_GRID_WIDTH);

	float scale_y = m_iPickUpItemIconHeight/
		float(m_iGridHeight*INV_GRID_HEIGHT);

	scale_x = (scale_x>1) ? 1.0f : scale_x;
	scale_y = (scale_y>1) ? 1.0f : scale_y;

	float scale = scale_x<scale_y?scale_x:scale_y;

	Frect					texture_rect;
	texture_rect.lt.set		(m_iXPos*INV_GRID_WIDTH, m_iYPos*INV_GRID_HEIGHT);
	texture_rect.rb.set		(m_iGridWidth*INV_GRID_WIDTH, m_iGridHeight*INV_GRID_HEIGHT);
	texture_rect.rb.add		(texture_rect.lt);
	UIPickUpItemIcon->GetStaticItem()->SetTextureRect(texture_rect);
	UIPickUpItemIcon->SetStretchTexture(true);


	UIPickUpItemIcon->SetWidth(m_iGridWidth*INV_GRID_WIDTH*scale*UI().get_current_kx());
	UIPickUpItemIcon->SetHeight(m_iGridHeight*INV_GRID_HEIGHT*scale);

	UIPickUpItemIcon->SetWndPos(Fvector2().set(	m_iPickUpItemIconX+(m_iPickUpItemIconWidth-UIPickUpItemIcon->GetWidth())/2.0f,
												m_iPickUpItemIconY+(m_iPickUpItemIconHeight-UIPickUpItemIcon->GetHeight())/2.0f) );

	UIPickUpItemIcon->SetTextureColor(color_rgba(255,255,255,192));
	UIPickUpItemIcon->Show(false);
};

void CUIMainIngameWnd::OnConnected()
{
	UIZoneMap->SetupCurrentMap();
	if ( m_ui_hud_states )
	{
		m_ui_hud_states->on_connected();
	}
}

void CUIMainIngameWnd::OnSectorChanged(int sector)
{
	UIZoneMap->OnSectorChanged(sector);
}

void CUIMainIngameWnd::reset_ui()
{
	m_pPickUpItem					= NULL;
	UIMotionIcon->ResetVisibility	();
	if ( m_ui_hud_states )
	{
		m_ui_hud_states->reset_ui();
	}
}

void CUIMainIngameWnd::ShowZoneMap( bool status ) 
{ 
	UIZoneMap->visible = status; 
}

void CUIMainIngameWnd::DrawZoneMap() 
{ 
	UIZoneMap->Render(); 
}

void CUIMainIngameWnd::UpdateZoneMap() 
{ 
	UIZoneMap->Update(); 
}

void CUIMainIngameWnd::UpdateMainIndicators()
{
	CActor* pActor = smart_cast<CActor*>(Level().CurrentViewEntity());
	if(!pActor)
		return;

	UpdateQuickSlots();
	if (IsGameTypeSingle())
		CurrentGameUI()->PdaMenu().UpdateRankingWnd();

	u8 flags = 0;
	flags |= LA_CYCLIC;
	flags |= LA_ONLYALPHA;
	flags |= LA_TEXTURECOLOR;
	
// Health fullscreen
	BOOL  lr_health_hud = ( psHUD_Flags.is(HUD_LR_HEALTH) );
	
	float lr_health = pActor->GetfHealth();	
	if(lr_health_hud)
	{
		if(lr_health>0.6f)
		{
			m_ind_lr_health->Show(false);
			m_ind_lr_health->ResetColorAnimation();
		}
		else
		{
			m_ind_lr_health->Show(true);
			if(lr_health>0.55f)
			{
				m_ind_lr_health->InitTexture("lr_circle_health");
				m_ind_lr_health->SetColorAnimation(NULL, 0);
				m_ind_lr_health->SetTextureColor(color_rgba(255,255,255,25));
			}
			else if(lr_health>0.5f)
			{
				m_ind_lr_health->InitTexture("lr_circle_health");
				m_ind_lr_health->SetColorAnimation(NULL, 0);
				m_ind_lr_health->SetTextureColor(color_rgba(255,255,255,50));
			}
			else if(lr_health>0.45f)
			{
				m_ind_lr_health->InitTexture("lr_circle_health");
				m_ind_lr_health->SetColorAnimation(NULL, 0);
				m_ind_lr_health->SetTextureColor(color_rgba(255,255,255,75));
			}
			else if(lr_health>0.4f)
			{
				m_ind_lr_health->InitTexture("lr_circle_health");
				m_ind_lr_health->SetColorAnimation(NULL, 0);
				m_ind_lr_health->SetTextureColor(color_rgba(255,255,255,100));
			}
			else if(lr_health>0.35f)
			{
				m_ind_lr_health->InitTexture("lr_circle_health");
				m_ind_lr_health->SetColorAnimation("ui_slow_blinking_alpha", flags);
				m_ind_lr_health->SetTextureColor(color_rgba(255,255,255,125));
			}
			else if(lr_health>0.3f)
			{
				m_ind_lr_health->InitTexture("lr_circle_health");
				//m_ind_lr_health->SetColorAnimation("ui_slow_blinking_alpha", flags);
				m_ind_lr_health->SetTextureColor(color_rgba(255,255,255,150));
			}
			else if(lr_health>0.25f)
			{
				m_ind_lr_health->InitTexture("lr_circle_health");
				//m_ind_lr_health->SetColorAnimation("ui_slow_blinking_alpha", flags);
				m_ind_lr_health->SetTextureColor(color_rgba(255,255,255,175));
			}
			else if(lr_health>0.2f)
			{
				m_ind_lr_health->InitTexture("lr_circle_health");
				m_ind_lr_health->SetColorAnimation("ui_medium_blinking_alpha", flags);
				m_ind_lr_health->SetTextureColor(color_rgba(255,255,255,200));
			}
			else if(lr_health>0.15f)
			{
				m_ind_lr_health->InitTexture("lr_circle_health");
				//m_ind_lr_health->SetColorAnimation("ui_medium_blinking_alpha", flags);
				m_ind_lr_health->SetTextureColor(color_rgba(255,255,255,225));
			}
			else
			{
				m_ind_lr_health->InitTexture("lr_circle_health");
				//m_ind_lr_health->SetColorAnimation("ui_medium_blinking_alpha", flags);
				m_ind_lr_health->SetTextureColor(color_rgba(255,255,255,255));
			}
		}
	}
	else
	{
		if(lr_health>0.01f)
		{
			m_ind_lr_health->Show(false);
			m_ind_lr_health->SetColorAnimation(NULL, 0);
			m_ind_lr_health->ResetColorAnimation();
		}
	}

	
// Health icon
//	BOOL  lr_health_icon = ( psHUD_Flags.is(HUD_LR_ICON_HEALTH) );

//	float lr_health_icon_stat = pActor->GetfHealth();			
	
	
// Power icon
//	BOOL  lr_stamina_icon = ( psHUD_Flags.is(HUD_LR_ICON_STAMINA) );

//	float lr_power = pActor->conditions().GetPower();		
	

//	u16 slot = pActor->inventory().GetActiveSlot();
//	m_ind_weapon_broken->Show(false);
//	if(slot==INV_SLOT_2 || slot==INV_SLOT_3)
	
}

void CUIMainIngameWnd::UpdateQuickSlots()
{
	BOOL  lr_quick_icon = ( psHUD_Flags.is(HUD_LR_ICON_QUICK) );

	string32 tmp;
		
	if(lr_quick_icon)
	{	
		LPCSTR str = CStringTable().translate("quick_use_str_1").c_str();
		strncpy_s(tmp, sizeof(tmp), str, 3);
		if(tmp[2]==',')
			tmp[1] = '\0';
		m_QuickSlotText1->SetTextST(tmp);

		str = CStringTable().translate("quick_use_str_2").c_str();
		strncpy_s(tmp, sizeof(tmp), str, 3);
		if(tmp[2]==',')
			tmp[1] = '\0';
		m_QuickSlotText2->SetTextST(tmp);

		str = CStringTable().translate("quick_use_str_3").c_str();
		strncpy_s(tmp, sizeof(tmp), str, 3);
		if(tmp[2]==',')
			tmp[1] = '\0';
		m_QuickSlotText3->SetTextST(tmp);

		str = CStringTable().translate("quick_use_str_4").c_str();
		strncpy_s(tmp, sizeof(tmp), str, 3);
		if(tmp[2]==',')
			tmp[1] = '\0';
		m_QuickSlotText4->SetTextST(tmp);
	}
	else
	{
		m_QuickSlotText1->SetTextST(" ");
		m_QuickSlotText2->SetTextST(" ");
		m_QuickSlotText3->SetTextST(" ");
		m_QuickSlotText4->SetTextST(" ");
	}

	CActor* pActor = smart_cast<CActor*>(Level().CurrentViewEntity());
	if(!pActor)
		return;

	for(u8 i=0;i<4;i++)
	{
		CUIStatic* wnd = smart_cast<CUIStatic* >(m_quick_slots_icons[i]->FindChild("counter"));
		if(wnd)
		{
			shared_str item_name = g_quick_use_slots[i];
			if(item_name.size())
			{
				u32 count = pActor->inventory().dwfGetSameItemCount(item_name.c_str(), true);
				string32 str;
				xr_sprintf(str, "x%d", count);
				wnd->TextItemControl()->SetText(str);
				wnd->Show(true);

				CUIStatic* main_slot = m_quick_slots_icons[i];
				main_slot->SetShader(InventoryUtilities::GetEquipmentIconsShader());
				Frect texture_rect;
				texture_rect.x1	= pSettings->r_float(item_name, "inv_grid_x")		*INV_GRID_WIDTH;
				texture_rect.y1	= pSettings->r_float(item_name, "inv_grid_y")		*INV_GRID_HEIGHT;
				texture_rect.x2	= pSettings->r_float(item_name, "inv_grid_width")	*INV_GRID_WIDTH;
				texture_rect.y2	= pSettings->r_float(item_name, "inv_grid_height")*INV_GRID_HEIGHT;
				texture_rect.rb.add(texture_rect.lt);
				main_slot->SetTextureRect(texture_rect);
				main_slot->TextureOn();
				main_slot->SetStretchTexture(true);
				if(lr_quick_icon)
				{
					if(!count)
					{
						wnd->SetTextureColor(color_rgba(255,255,255,0));
						wnd->TextItemControl()->SetTextColor(color_rgba(255,255,255,0));
						m_quick_slots_icons[i]->SetTextureColor(color_rgba(255,255,255,100));
					}
					else
					{
						wnd->SetTextureColor(color_rgba(255,255,255,255));
						wnd->TextItemControl()->SetTextColor(color_rgba(255,255,255,255));
						m_quick_slots_icons[i]->SetTextureColor(color_rgba(255,255,255,255));
					}
				}
				else if(!lr_quick_icon)
				{
					wnd->Show(false);
					m_quick_slots_icons[i]->SetTextureColor(color_rgba(255,255,255,0));
				}	
			}
			else
			{
				wnd->Show(false);
				m_quick_slots_icons[i]->SetTextureColor(color_rgba(255,255,255,0));
//				m_quick_slots_icons[i]->Show(false);
			}
		}
	}
}

void CUIMainIngameWnd::DrawMainIndicatorsForInventory()
{
	CActor* pActor = smart_cast<CActor*>(Level().CurrentViewEntity());
	if(!pActor)
		return;

	UpdateMainIndicators();
	
	if(m_ind_lr_health->IsShown())
	{
		m_ind_lr_health->Update();
		m_ind_lr_health->Draw();
	}
	
/*	UpdateQuickSlots();
	UpdateBoosterIndicators(pActor->conditions().GetCurBoosterInfluences());

	for(int i=0;i<4;i++)
		m_quick_slots_icons[i]->Draw();

	m_QuickSlotText1->Draw();
	m_QuickSlotText2->Draw();
	m_QuickSlotText3->Draw();
	m_QuickSlotText4->Draw();

	if(m_ind_boost_psy->IsShown())
	{
		m_ind_boost_psy->Update();
		m_ind_boost_psy->Draw();
	}

	if(m_ind_boost_radia->IsShown())
	{
		m_ind_boost_radia->Update();
		m_ind_boost_radia->Draw();
	}

	if(m_ind_boost_chem->IsShown())
	{
		m_ind_boost_chem->Update();
		m_ind_boost_chem->Draw();
	}

	if(m_ind_boost_wound->IsShown())
	{
		m_ind_boost_wound->Update();
		m_ind_boost_wound->Draw();
	}

	if(m_ind_boost_weight->IsShown())
	{
		m_ind_boost_weight->Update();
		m_ind_boost_weight->Draw();
	}

	if(m_ind_boost_health->IsShown())
	{
		m_ind_boost_health->Update();
		m_ind_boost_health->Draw();
	}

	if(m_ind_boost_power->IsShown())
	{
		m_ind_boost_power->Update();
		m_ind_boost_power->Draw();
	}

	if(m_ind_boost_rad->IsShown())
	{
		m_ind_boost_rad->Update();
		m_ind_boost_rad->Draw();
	}

	m_ui_hud_states->DrawZoneIndicators();*/
}

void CUIMainIngameWnd::UpdateBoosterIndicators(const xr_map<EBoostParams, SBooster> influences)
{
	m_ind_boost_psy->Show(false);
	m_ind_boost_radia->Show(false);
	m_ind_boost_chem->Show(false);
	m_ind_boost_wound->Show(false);
	m_ind_boost_weight->Show(false);
	m_ind_boost_health->Show(false);
	m_ind_boost_power->Show(false);
	m_ind_boost_rad->Show(false);

	LPCSTR str_flag	= "ui_slow_blinking_alpha";
	u8 flags = 0;
	flags |= LA_CYCLIC;
	flags |= LA_ONLYALPHA;
	flags |= LA_TEXTURECOLOR;

	xr_map<EBoostParams, SBooster>::const_iterator b = influences.begin(), e = influences.end();
	for(; b!=e; b++)
	{
		switch(b->second.m_type)
		{
			case eBoostHpRestore: 
				{
					m_ind_boost_health->Show(true);
					if(b->second.fBoostTime<=3.0f)
						m_ind_boost_health->SetColorAnimation(str_flag, flags);
					else
						m_ind_boost_health->ResetColorAnimation();
				}
				break;
			case eBoostPowerRestore: 
				{
					m_ind_boost_power->Show(true); 
					if(b->second.fBoostTime<=3.0f)
						m_ind_boost_power->SetColorAnimation(str_flag, flags);
					else
						m_ind_boost_power->ResetColorAnimation();
				}
				break;
			case eBoostRadiationRestore: 
				{
					m_ind_boost_rad->Show(true); 
					if(b->second.fBoostTime<=3.0f)
						m_ind_boost_rad->SetColorAnimation(str_flag, flags);
					else
						m_ind_boost_rad->ResetColorAnimation();
				}
				break;
			case eBoostBleedingRestore: 
				{
					m_ind_boost_wound->Show(true); 
					if(b->second.fBoostTime<=3.0f)
						m_ind_boost_wound->SetColorAnimation(str_flag, flags);
					else
						m_ind_boost_wound->ResetColorAnimation();
				}
				break;
			case eBoostMaxWeight: 
				{
					m_ind_boost_weight->Show(true); 
					if(b->second.fBoostTime<=3.0f)
						m_ind_boost_weight->SetColorAnimation(str_flag, flags);
					else
						m_ind_boost_weight->ResetColorAnimation();
				}
				break;
			case eBoostRadiationImmunity: 
			case eBoostRadiationProtection: 
				{
					m_ind_boost_radia->Show(true); 
					if(b->second.fBoostTime<=3.0f)
						m_ind_boost_radia->SetColorAnimation(str_flag, flags);
					else
						m_ind_boost_radia->ResetColorAnimation();
				}
				break;
			case eBoostTelepaticImmunity: 
			case eBoostTelepaticProtection: 
				{
					m_ind_boost_psy->Show(true); 
					if(b->second.fBoostTime<=3.0f)
						m_ind_boost_psy->SetColorAnimation(str_flag, flags);
					else
						m_ind_boost_psy->ResetColorAnimation();
				}
				break;
			case eBoostChemicalBurnImmunity: 
			case eBoostChemicalBurnProtection: 
				{
					m_ind_boost_chem->Show(true); 
					if(b->second.fBoostTime<=3.0f)
						m_ind_boost_chem->SetColorAnimation(str_flag, flags);
					else
						m_ind_boost_chem->ResetColorAnimation();
				}
				break;
		}
	}
}