#pragma once
#include "../BaseMonster/base_monster.h"
#include "../controlled_entity.h"
#include "../ai_monster_bones.h"
#include "../anim_triple.h"
#include "../../../../xrServerEntities/script_export_space.h"
#include "ai/monsters/controlled_actor.h"

#define FAKE_DEATH_TYPES_COUNT	4

class CZombie :	public CBaseMonster,
				public CControlledEntity<CZombie>, public CControlledActor {
	
	typedef		CBaseMonster				inherited;
	typedef		CControlledEntity<CZombie>	CControlled;

	bonesManipulation	Bones;

public:
					CZombie		();
	virtual			~CZombie	();	

	virtual void	Load				(LPCSTR section);
	virtual BOOL	net_Spawn			(CSE_Abstract* DC);
	virtual void	reinit				();
	virtual	void	reload				(LPCSTR section);

    virtual void UpdateCL();
	
	virtual	void	Hit					(SHit* pHDS);

	virtual bool	ability_pitch_correction () {return false;}

	virtual void	shedule_Update		(u32 dt);
	
	static	void 	BoneCallback		(CBoneInstance *B);
			void	vfAssignBones		();

	virtual bool	use_center_to_aim				() const {return true;}
	virtual	char*	get_monster_class_name () { return "zombie"; }


	CBoneInstance			*bone_spine;
	CBoneInstance			*bone_head;

	SAnimationTripleData	anim_triple_death[FAKE_DEATH_TYPES_COUNT];
	u8				active_triple_idx;
	
	u32				time_dead_start;
	u32				last_hit_frame;
	u32				time_resurrect;

	u8				fake_death_count;
	float			health_death_threshold;
	u8				fake_death_left;

	bool			fake_death_fall_down	(); //return true if everything is ok
	void			fake_death_stand_up		();

    //--------------------------------------------------------------------
    // Vampire
    //--------------------------------------------------------------------
//public:
    u32 m_vampire_min_delay;
    static u32 m_time_last_vampire;
    SAnimationTripleData anim_triple_vampire;

    SPPInfo pp_vampire_effector;

    void ActivateVampireEffector();
    bool WantVampire();
    void SatisfyVampire();

    u32 get_last_critical_hit_tick() { return m_last_critical_hit_tick; }
    void clear_last_critical_hit_tick() { m_last_critical_hit_tick = 0; }
private:
    TTime m_last_critical_hit_tick;
    float m_critical_hit_chance; // 0..1
    float m_critical_hit_camera_effector_angle;

    float m_vampire_want_value;
    float m_vampire_want_speed; // load from ltx
    float m_vampire_wound;
    float m_vampire_gain_health;
    float m_vampire_distance;
    u32 m_vampire_pause_time;
    u32 m_vampire_pause_time_static;
    u32 m_vampire_chance;
    float m_vis_state;
    bool m_drag_anim_jump;
    bool m_animated;
    static void animation_end_jump(CBlend* B);

    void LoadVampirePPEffector(LPCSTR section);

public:
    enum EZombieSounds
    {
        eAdditionalSounds = MonsterSound::eMonsterSoundCustom,

        eVampireGrasp = eAdditionalSounds | 0,
        eVampireSucking = eAdditionalSounds | 1,
        eVampireHit = eAdditionalSounds | 2,
        eVampireStartHunt = eAdditionalSounds | 3,

    };
	
public:
    float get_vampire_distance() const { return m_vampire_distance; }
    u32 get_vampire_pause_time() const { return m_vampire_pause_time; }
	u32 get_vampire_chance() const { return m_vampire_chance; }
    void update_vampire_pause_time();
	
public:
    u32 m_hits_before_vampire;
    u32 m_sufficient_hits_before_vampire;
    int m_sufficient_hits_before_vampire_random;
    virtual void on_attack_on_run_hit();
    bool done_enough_hits_before_vampire();
	
#ifdef _DEBUG
	virtual void	debug_on_key			(int key);
#endif


	DECLARE_SCRIPT_REGISTER_FUNCTION
};

add_to_type_list(CZombie)
#undef script_type_list
#define script_type_list save_type_list(CZombie)
