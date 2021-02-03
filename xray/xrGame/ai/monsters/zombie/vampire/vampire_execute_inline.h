#pragma once

#include "../../../../Include/xrRender/KinematicsAnimated.h"
#include "../../../actor.h"
#include "../../../../xrEngine/CameraBase.h"

// #include "../../../../xrEngine/CameraBase.h"
//#include "../../../ActorCondition.h"
#include "../../../HudManager.h"
#include "ai/monsters/controlled_actor.h"
#include <luabindex/functor.hpp>
#include "script_engine.h"
#include "ai_space.h"

#define TEMPLATE_SPECIALIZATION template <typename _Object>

#define CStateZombieVampireExecuteAbstract CStateZombieVampireExecute<_Object>

#define VAMPIRE_TIME_HOLD		4000
#define VAMPIRE_HIT_IMPULSE		40.f
#define VAMPIRE_MIN_DIST		0.5f
#define VAMPIRE_MAX_DIST 1.5f

TEMPLATE_SPECIALIZATION
void CStateZombieVampireExecuteAbstract::initialize()
{
	inherited::initialize					();

    this->object->CControlledActor::install();

	look_head				();

	m_action				= eActionPrepare;
	time_vampire_started	= 0;


    this->object->m_hits_before_vampire = 0;
    this->object->m_sufficient_hits_before_vampire_random = -1 + (rand() % 3);

	luabindex::functor<void> functor;
	ai().script_engine().functor("xr_effects.disable_ui_anim",functor);
	functor(1);
	
    Actor()->TransferInfo("bs_qte", true);

	m_effector_activated			= false;
}

TEMPLATE_SPECIALIZATION
void CStateZombieVampireExecuteAbstract::execute()
{

    if (!m_effector_activated)
    {
        this->object->ActivateVampireEffector();
        m_effector_activated = true;
    }
	
	luabindex::functor<void> functor;
	ai().script_engine().functor("leer_scr.bloodsucker_qte_hud",functor);
	functor(1);

    look_head();

	switch (m_action) {
		case eActionPrepare:
			execute_vampire_prepare();
			m_action = eActionContinue;
			break;

		case eActionContinue:
			execute_vampire_continue();
			break;

		case eActionFire:
			execute_vampire_hit();
			m_action = eActionWaitTripleEnd;
			break;

    case eActionWaitTripleEnd:
        if (!this->object->com_man().ta_is_active())
        {
            m_action = eActionCompleted;
        }

		case eActionCompleted:
			break;
	}

    this->object->dir().face_target(this->object->EnemyMan.get_enemy());

    Fvector const enemy_to_self = this->object->EnemyMan.get_enemy()->Position() - this->object->Position();
    float const dist_to_enemy = magnitude(enemy_to_self);
    float const vampire_dist = this->object->get_vampire_distance();

    if (angle_between_vectors(this->object->Direction(), enemy_to_self) < deg2rad(20.f) && dist_to_enemy > vampire_dist)
    {
        this->object->set_action(ACT_RUN);
        this->object->anim().accel_activate(eAT_Aggressive);
        this->object->anim().accel_set_braking(false);

        u32 const target_vertex = this->object->EnemyMan.get_enemy()->ai_location().level_vertex_id();
        Fvector const target_pos = ai().level_graph().vertex_position(target_vertex);

        this->object->path().set_target_point(target_pos, target_vertex);
        this->object->path().set_rebuild_time(100);
        this->object->path().set_use_covers(false);
        this->object->path().set_distance_to_end(vampire_dist);
    }
    else
    {
        this->object->set_action(ACT_STAND_IDLE);
    }
}

TEMPLATE_SPECIALIZATION
void CStateZombieVampireExecuteAbstract::show_hud()
{
	HUD().SetRenderable(true);
	NET_Packet			P;

	Actor()->u_EventGen	(P, GEG_PLAYER_WEAPON_HIDE_STATE, Actor()->ID());
	P.w_u16				(INV_STATE_BLOCK_ALL);
	P.w_u8				(u8(false));
	Actor()->u_EventSend(P);
}

TEMPLATE_SPECIALIZATION
void CStateZombieVampireExecuteAbstract::cleanup()
{
	
    if (this->object->com_man().ta_is_active())
        this->object->com_man().ta_deactivate();

    if (this->object->CControlledActor::is_controlling())
        this->object->CControlledActor::release();
	
	this->object->update_vampire_pause_time();
	
	Actor()->OnDisableInfo("bs_qte");
	
	if (Actor()->HasInfo("bs_qte_die"))
	{
	Actor()->OnDisableInfo("bs_qte_die");
	luabindex::functor<void> functor;
	ai().script_engine().functor("xr_effects.kill_actor",functor);
	functor(1);
	}
	
	luabindex::functor<void> functor;
	ai().script_engine().functor("leer_scr.bloodsucker_qte_hud_dis",functor);
	functor(1);
}

TEMPLATE_SPECIALIZATION
void CStateZombieVampireExecuteAbstract::finalize()
{
	inherited::finalize();
	cleanup();
}

TEMPLATE_SPECIALIZATION
void CStateZombieVampireExecuteAbstract::critical_finalize()
{
	inherited::critical_finalize();
	cleanup();
}

TEMPLATE_SPECIALIZATION
bool CStateZombieVampireExecuteAbstract::check_start_conditions()
{
    const CEntityAlive* enemy = this->object->EnemyMan.get_enemy();

    u32 const vertex_id = ai().level_graph().check_position_in_direction(
        this->object->ai_location().level_vertex_id(), this->object->Position(), enemy->Position());
    if (!ai().level_graph().valid_vertex_id(vertex_id))
        return false;

	if (this->object->get_vampire_chance() == 0)
	return false;

    if (!this->object->MeleeChecker.can_start_melee(enemy))
        return false;

    // проверить направление на врага
    if (!this->object->control().direction().is_face_target(enemy, PI_DIV_2))
        return false;

	//if ( !object->WantVampire() ) 
	//	return false;
	
	// является ли враг актером
	if ( !smart_cast<CActor const*>(enemy) )
		return false;

    if (this->object->CControlledActor::is_controlling())
        return false;

	const CActor *actor = smart_cast<const CActor *>(enemy);
	
	VERIFY(actor);

	if ( actor->input_external_handler_installed() )
        return false;
	
	if (Device.dwTimeGlobal <= this->object->get_vampire_pause_time())
	{
		return false;
	}
	else if ((rand() & 10) > this->object->get_vampire_chance())
	{
		this->object->update_vampire_pause_time();
		return false;
	}

	return true;
}

TEMPLATE_SPECIALIZATION
bool CStateZombieVampireExecuteAbstract::check_completion() 
{
	return (m_action == eActionCompleted);
}

//////////////////////////////////////////////////////////////////////////

TEMPLATE_SPECIALIZATION
void CStateZombieVampireExecuteAbstract::execute_vampire_prepare()
{
    this->object->com_man().ta_activate(this->object->anim_triple_vampire);
    time_vampire_started = Device.dwTimeGlobal + VAMPIRE_TIME_HOLD;

    this->object->sound().play(CZombie::eVampireGrasp);
}

TEMPLATE_SPECIALIZATION
void CStateZombieVampireExecuteAbstract::execute_vampire_continue()
{
    const CEntityAlive* enemy = this->object->EnemyMan.get_enemy();
	
    if ( !this->object->MeleeChecker.can_start_melee(enemy) )
    {
        this->object->com_man().ta_deactivate();
        m_action = eActionCompleted;
        return;
    }
	
	this->object->sound().play(CZombie::eVampireSucking);
	// проверить на грави удар
	if (!Actor()->HasInfo("bs_qte_done") && time_vampire_started < Device.dwTimeGlobal) {
		Actor()->TransferInfo("bs_qte_die", true);
		m_action = eActionFire;
	}
	
	if (Actor()->HasInfo("bs_qte_done") && time_vampire_started < Device.dwTimeGlobal) {
		cleanup();
		
		luabindex::functor<void> functor;
		ai().script_engine().functor("leer_scr.quick_kk",functor);
		functor(1);
		
		this->object->conditions().SetHealth(0.01f);
		
       		 m_action = eActionCompleted;
		
		Actor()->OnDisableInfo("bs_qte_done");
	}
    
}

TEMPLATE_SPECIALIZATION
void CStateZombieVampireExecuteAbstract::execute_vampire_hit()
{
    this->object->com_man().ta_pointbreak();
    this->object->sound().play(CZombie::eVampireHit);
    this->object->SatisfyVampire();
}

//////////////////////////////////////////////////////////////////////////

TEMPLATE_SPECIALIZATION
void CStateZombieVampireExecuteAbstract::look_head()
{
    IKinematics* pK = smart_cast<IKinematics*>(this->object->Visual());
    Fmatrix bone_transform = pK->LL_GetTransform(pK->LL_BoneID("bip01_neck"));

    Fmatrix global_transform;
    global_transform.mul_43(this->object->XFORM(), bone_transform);

    this->object->CControlledActor::look_point(global_transform.c);
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateZombieVampireExecuteAbstract

