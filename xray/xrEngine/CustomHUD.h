#pragma once

ENGINE_API extern Flags32		psHUD_Flags;
#define HUD_CROSSHAIR			(1<<0)
#define HUD_CROSSHAIR_DIST		(1<<1)
#define HUD_WEAPON				(1<<2)
#define HUD_INFO				(1<<3)
#define HUD_DRAW				(1<<4)
#define HUD_CROSSHAIR_RT		(1<<5)
#define HUD_WEAPON_RT			(1<<6)
#define HUD_CROSSHAIR_DYNAMIC	(1<<7)
#define HUD_CROSSHAIR_RT2		(1<<8)
#define HUD_DRAW_RT				(1<<9)
#define HUD_WEAPON_RT2			(1<<10)
#define HUD_DRAW_RT2			(1<<11)
#define HUD_LR_CROSSHAIR		(1<<12)
#define HUD_LR_DRAW_MINIMAP		(1<<13)
#define HUD_LR_ALT_MINIMAP		(1<<14)
#define HUD_LR_DRAW_BACK_HUD	(1<<15)
#define HUD_LR_ALT_BACK_HUD		(1<<16)
#define HUD_LR_ANIMATIONS		(1<<17)
#define HUD_LR_HUDDRAW_TOGGLE	(1<<18)
#define HUD_LR_HEALTH			(1<<19)
#define HUD_LR_ICON_STAMINA		(1<<20)
#define HUD_LR_ALT_BACK_HUD_2	(1<<21)
#define HUD_LR_ALT_MINIMAP_2	(1<<22)
#define HUD_LR_ICON_HEALTH		(1<<23)
#define HUD_LR_ICON_QUICK		(1<<24)
#define HUD_LR_DRAW_BACK_WPN	(1<<25)

class ENGINE_API IRender_Visual;
class CUI;

class ENGINE_API CCustomHUD:
	public DLL_Pure,
	public IEventReceiver,
	public pureScreenResolutionChanged
{
public:
					CCustomHUD				();
	virtual			~CCustomHUD				();

	BENCH_SEC_SCRAMBLEVTBL2
	
	virtual		void		Render_First			(){;}
	virtual		void		Render_Last				(){;}

	BENCH_SEC_SCRAMBLEVTBL1
	
	virtual		void		OnFrame					(){;}
	virtual		void		OnEvent					(EVENT E, u64 P1, u64 P2){;}

	virtual		void		Load					(){;}
	virtual		void		OnDisconnected			()=0;
	virtual		void		OnConnected				()=0;
	virtual		void		RenderActiveItemUI		()=0;
	virtual		bool		RenderActiveItemUIQuery	()=0;
	virtual		void		net_Relcase				(CObject *object) = 0;
};

extern ENGINE_API CCustomHUD* g_hud;