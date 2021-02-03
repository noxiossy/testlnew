#include "stdafx.h"
#include "zombie.h"
#include "zombie_state_manager.h"
#include "../../../profiler.h"
#include "../../../../Include/xrRender/KinematicsAnimated.h"
#include "../../../entitycondition.h"
#include "../monster_velocity_space.h"
#include "vampire/vampire_effector.h"
#include "Actor.h"
#include "ActorEffector.h"
#include "sound_player.h"

#include "../xrEngine/CameraBase.h"
#include "ActorCondition.h"
#include "PHDestroyable.h"
#include "CharacterPhysicsSupport.h"
#include "GamePersistent.h"
#include "game_object_space.h"

#include "../control_animation_base.h"
#include "../control_movement_base.h"

#ifdef _DEBUG
#include <dinput.h>
#endif

u32 CZombie::m_time_last_vampire = 0;

CZombie::CZombie()
{
	StateMan = xr_new<CStateManagerZombie>(this);
	
	CControlled::init_external(this);
}

CZombie::~CZombie()
{
	xr_delete		(StateMan);
}

void CZombie::Load(LPCSTR section)
{
	inherited::Load	(section);

	anim().accel_load			(section);
	anim().accel_chain_add		(eAnimWalkFwd,		eAnimRun);

	fake_death_count		= 1 + u8(Random.randI(pSettings->r_u8(section,"FakeDeathCount")));
	health_death_threshold	= pSettings->r_float(section,"StartFakeDeathHealthThreshold");

	SVelocityParam &velocity_none		= move().get_velocity(MonsterMovement::eVelocityParameterIdle);	
	SVelocityParam &velocity_turn		= move().get_velocity(MonsterMovement::eVelocityParameterStand);
	SVelocityParam &velocity_walk		= move().get_velocity(MonsterMovement::eVelocityParameterWalkNormal);
	SVelocityParam &velocity_run		= move().get_velocity(MonsterMovement::eVelocityParameterRunNormal);
	//SVelocityParam &velocity_walk_dmg	= move().get_velocity(MonsterMovement::eVelocityParameterWalkDamaged);
	//SVelocityParam &velocity_run_dmg	= move().get_velocity(MonsterMovement::eVelocityParameterRunDamaged);
	//SVelocityParam &velocity_steal		= move().get_velocity(MonsterMovement::eVelocityParameterSteal);
	//SVelocityParam &velocity_drag		= move().get_velocity(MonsterMovement::eVelocityParameterDrag);


	anim().AddAnim(eAnimStandIdle,		"stand_idle_",			-1, &velocity_none,		PS_STAND,	"fx_stand_f", "fx_stand_b", "fx_stand_l", "fx_stand_r");
	anim().AddAnim(eAnimStandTurnLeft,	"stand_turn_ls_",		-1, &velocity_turn,		PS_STAND,	"fx_stand_f", "fx_stand_b", "fx_stand_l", "fx_stand_r");
	anim().AddAnim(eAnimStandTurnRight,	"stand_turn_rs_",		-1, &velocity_turn,		PS_STAND,	"fx_stand_f", "fx_stand_b", "fx_stand_l", "fx_stand_r");
	anim().AddAnim(eAnimWalkFwd,			"stand_walk_fwd_",		-1, &velocity_walk,		PS_STAND,	"fx_stand_f", "fx_stand_b", "fx_stand_l", "fx_stand_r");
	anim().AddAnim(eAnimRun,				"stand_run_",			-1,	&velocity_run,		PS_STAND,	"fx_stand_f", "fx_stand_b", "fx_stand_l", "fx_stand_r");
	anim().AddAnim(eAnimAttack,			"stand_attack_",		-1, &velocity_turn,		PS_STAND,	"fx_stand_f", "fx_stand_b", "fx_stand_l", "fx_stand_r");
	anim().AddAnim(eAnimDie,				"stand_die_",			0, &velocity_none,		PS_STAND,	"fx_stand_f", "fx_stand_b", "fx_stand_l", "fx_stand_r");

	anim().LinkAction(ACT_STAND_IDLE,	eAnimStandIdle);
	anim().LinkAction(ACT_SIT_IDLE,		eAnimStandIdle);
	anim().LinkAction(ACT_LIE_IDLE,		eAnimStandIdle);
	anim().LinkAction(ACT_WALK_FWD,		eAnimWalkFwd);
	anim().LinkAction(ACT_WALK_BKWD,		eAnimWalkFwd);
	anim().LinkAction(ACT_RUN,			eAnimRun);
	anim().LinkAction(ACT_EAT,			eAnimStandIdle);
	anim().LinkAction(ACT_SLEEP,			eAnimStandIdle);
	anim().LinkAction(ACT_REST,			eAnimStandIdle);
	anim().LinkAction(ACT_DRAG,			eAnimStandIdle);
	anim().LinkAction(ACT_ATTACK,		eAnimAttack);
	anim().LinkAction(ACT_STEAL,			eAnimWalkFwd);
	anim().LinkAction(ACT_LOOK_AROUND,	eAnimStandIdle);

    m_hits_before_vampire = 0;

#ifdef DEBUG	
	anim().accel_chain_test		();
#endif

    LoadVampirePPEffector(pSettings->r_string(section, "vampire_effector"));
    m_vampire_min_delay = pSettings->r_u32(section, "Vampire_Delay");
	
	m_vampire_pause_time = pSettings->r_u32(section, "Vampire_Pause")+Device.dwTimeGlobal;
	m_vampire_pause_time_static = pSettings->r_u32(section, "Vampire_Pause");
	m_vampire_chance = pSettings->r_u32(section, "Vampire_Chance");

    m_vampire_want_speed = pSettings->r_float(section, "Vampire_Want_Speed");
    m_vampire_wound = pSettings->r_float(section, "Vampire_Wound");
	m_vampire_gain_health			= pSettings->r_float(section,"Vampire_GainHealth");
	m_vampire_distance				= pSettings->r_float(section,"Vampire_Distance");
	m_sufficient_hits_before_vampire	=	pSettings->r_u32(section,"Vampire_Sufficient_Hits");
    m_sufficient_hits_before_vampire_random = -1 + (rand() % 3);

    PostLoad(section);
}

void CZombie::update_vampire_pause_time()
{
	m_vampire_pause_time = m_vampire_pause_time_static+Device.dwTimeGlobal;
}

void CZombie::reinit()
{
	inherited::reinit();
    CControlledActor::reinit();

	Bones.Reset();
	
    com_man().ta_fill_data(anim_triple_vampire, "stand_attack_4_", "stand_attack_5_", "stand_attack_6_",
        TA_EXECUTE_LOOPED, TA_DONT_SKIP_PREPARE, 0); // ControlCom::eCapturePath | ControlCom::eCaptureMovement);

	time_dead_start			= 0;
	last_hit_frame			= 0;
	time_resurrect			= 0;
	fake_death_left			= fake_death_count;

    m_vampire_want_value = 0.f;

	active_triple_idx		= u8(-1);
}

void CZombie::reload(LPCSTR section)
{
	inherited::reload(section);

    sound().add(pSettings->r_string(section, "Sound_Vampire_Grasp"), DEFAULT_SAMPLE_COUNT, SOUND_TYPE_MONSTER_ATTACKING,
        MonsterSound::eHighPriority + 4, MonsterSound::eBaseChannel, eVampireGrasp, "bip01_head");
    sound().add(pSettings->r_string(section, "Sound_Vampire_Sucking"), DEFAULT_SAMPLE_COUNT,
        SOUND_TYPE_MONSTER_ATTACKING, MonsterSound::eHighPriority + 3, MonsterSound::eBaseChannel, eVampireSucking,
        "bip01_head");
    sound().add(pSettings->r_string(section, "Sound_Vampire_Hit"), DEFAULT_SAMPLE_COUNT, SOUND_TYPE_MONSTER_ATTACKING,
        MonsterSound::eHighPriority + 2, MonsterSound::eBaseChannel, eVampireHit, "bip01_head");
    sound().add(pSettings->r_string(section, "Sound_Vampire_StartHunt"), DEFAULT_SAMPLE_COUNT,
        SOUND_TYPE_MONSTER_ATTACKING, MonsterSound::eHighPriority + 5, MonsterSound::eBaseChannel, eVampireStartHunt,
        "bip01_head");

	com_man().ta_fill_data(anim_triple_death[0],	"fake_death_0_0",	"fake_death_0_1",	"fake_death_0_2",	true, false);
	com_man().ta_fill_data(anim_triple_death[1],	"fake_death_1_0",	"fake_death_1_1",	"fake_death_1_2",	true, false);
	com_man().ta_fill_data(anim_triple_death[2],	"fake_death_2_0",	"fake_death_2_1",	"fake_death_2_2",	true, false);
	com_man().ta_fill_data(anim_triple_death[3],	"fake_death_3_0",	"fake_death_3_1",	"fake_death_3_2",	true, false);
}

void CZombie::LoadVampirePPEffector(LPCSTR section)
{
    pp_vampire_effector.duality.h = pSettings->r_float(section, "duality_h");
    pp_vampire_effector.duality.v = pSettings->r_float(section, "duality_v");
    pp_vampire_effector.gray = pSettings->r_float(section, "gray");
    pp_vampire_effector.blur = pSettings->r_float(section, "blur");
    pp_vampire_effector.noise.intensity = pSettings->r_float(section, "noise_intensity");
    pp_vampire_effector.noise.grain = pSettings->r_float(section, "noise_grain");
    pp_vampire_effector.noise.fps = pSettings->r_float(section, "noise_fps");
    VERIFY(!fis_zero(pp_vampire_effector.noise.fps));

    sscanf(pSettings->r_string(section, "color_base"), "%f,%f,%f", &pp_vampire_effector.color_base.r,
        &pp_vampire_effector.color_base.g, &pp_vampire_effector.color_base.b);
    sscanf(pSettings->r_string(section, "color_gray"), "%f,%f,%f", &pp_vampire_effector.color_gray.r,
        &pp_vampire_effector.color_gray.g, &pp_vampire_effector.color_gray.b);
    sscanf(pSettings->r_string(section, "color_add"), "%f,%f,%f", &pp_vampire_effector.color_add.r,
        &pp_vampire_effector.color_add.g, &pp_vampire_effector.color_add.b);
}

void CZombie::BoneCallback(CBoneInstance *B)
{
	CZombie*	this_class = static_cast<CZombie*>(B->callback_param());

	START_PROFILE("Zombie/Bones Update");
	this_class->Bones.Update(B, Device.dwTimeGlobal);
	STOP_PROFILE("AI/Zombie/Bones Update");
}


void CZombie::vfAssignBones()
{
	// Установка callback на кости
	bone_spine =	&smart_cast<IKinematics*>(Visual())->LL_GetBoneInstance(smart_cast<IKinematics*>(Visual())->LL_BoneID("bip01_spine"));
	bone_head =		&smart_cast<IKinematics*>(Visual())->LL_GetBoneInstance(smart_cast<IKinematics*>(Visual())->LL_BoneID("bip01_head"));
	//if(!PPhysicsShell())//нельзя ставить колбеки, если создан физ шел - у него стоят свои колбеки!!!
	//{
		//bone_spine->set_callback(BoneCallback,this);
		//bone_head->set_callback(BoneCallback,this);
	//}

	// Bones settings
	Bones.Reset();
	Bones.AddBone(bone_spine, AXIS_Z);	Bones.AddBone(bone_spine, AXIS_Y); Bones.AddBone(bone_spine, AXIS_X);
	Bones.AddBone(bone_head, AXIS_Z);	Bones.AddBone(bone_head, AXIS_Y);
}

void CZombie::ActivateVampireEffector()
{
    Actor()->Cameras().AddCamEffector(
        new CVampireCameraEffector(6.0f, get_head_position(this), get_head_position(Actor())));
    Actor()->Cameras().AddPPEffector(new CVampirePPEffector(pp_vampire_effector, 6.0f));
}

bool CZombie::WantVampire() { return !!fsimilar(m_vampire_want_value, 1.f); }
void CZombie::SatisfyVampire()
{
    m_vampire_want_value = 0.f;
}

BOOL CZombie::net_Spawn (CSE_Abstract* DC) 
{
	if (!inherited::net_Spawn(DC))
		return(FALSE);

	vfAssignBones	();

	return(TRUE);
}

#define TIME_FAKE_DEATH			5000
#define TIME_RESURRECT_RESTORE	2000

//void CZombie::Hit(float P,Fvector &dir,CObject*who,s16 element,Fvector p_in_object_space,float impulse, ALife::EHitType hit_type)
void	CZombie::Hit								(SHit* pHDS)
{
//	inherited::Hit(P,dir,who,element,p_in_object_space,impulse,hit_type);
	inherited::Hit(pHDS);

	if (!g_Alive()) return;
	
	if ((pHDS->hit_type == ALife::eHitTypeFireWound) && (Device.dwFrame != last_hit_frame)) {
		if (!com_man().ta_is_active() && (time_resurrect + TIME_RESURRECT_RESTORE < Device.dwTimeGlobal) && (conditions().GetHealth() < health_death_threshold)) {
			if (conditions().GetHealth() < (health_death_threshold - float(fake_death_count - fake_death_left) * health_death_threshold / fake_death_count)) {
				active_triple_idx			= u8(Random.randI(FAKE_DEATH_TYPES_COUNT));
				com_man().ta_activate		(anim_triple_death[active_triple_idx]);
				move().stop					();
				time_dead_start				= Device.dwTimeGlobal;
				
				if (fake_death_left == 0)	fake_death_left = 1;
				fake_death_left--;
			}
		} 
	}

	last_hit_frame = Device.dwFrame;
}

void CZombie::UpdateCL()
{
    inherited::UpdateCL();
    CControlledActor::frame_update();

    if (g_Alive())
    {
        // update vampire need
        m_vampire_want_value += m_vampire_want_speed * client_update_fdelta();
        clamp(m_vampire_want_value, 0.f, 1.f);
    }
}

bool CZombie::done_enough_hits_before_vampire()
{
    return m_hits_before_vampire >= m_sufficient_hits_before_vampire + m_sufficient_hits_before_vampire_random;
}

void CZombie::on_attack_on_run_hit() { ++m_hits_before_vampire; }

void CZombie::shedule_Update(u32 dt)
{
	inherited::shedule_Update(dt);

	if (time_dead_start != 0) {
		if (time_dead_start + TIME_FAKE_DEATH < Device.dwTimeGlobal) {
			time_dead_start  = 0;

			com_man().ta_pointbreak();	

			time_resurrect = Device.dwTimeGlobal;
		}
	}
}


bool CZombie::fake_death_fall_down()
{
	if (com_man().ta_is_active()) return false;

	com_man().ta_activate		(anim_triple_death[u8(Random.randI(FAKE_DEATH_TYPES_COUNT))]);
	move().stop					();

	return true;
}

void CZombie::fake_death_stand_up()
{
	// check if state active
	bool active = false;
	for (u32 i=0; i<FAKE_DEATH_TYPES_COUNT; i++) {
		if (com_man().ta_is_active(anim_triple_death[i])) {
			active = true;
			break;
		}
	}
	if (!active) return;
	
	com_man().ta_pointbreak();
}


#ifdef _DEBUG
void CZombie::debug_on_key(int key)
{
	switch (key){
	case DIK_MINUS:
		{
			fake_death_fall_down();
		}
		break;
	case DIK_EQUALS:
		{
			fake_death_stand_up();
		}
		break;
	}
}
#endif
