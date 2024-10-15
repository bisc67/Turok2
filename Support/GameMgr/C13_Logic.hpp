//========================================================================================================================================================
//
//  Campaign 13 Logic
//
//========================================================================================================================================================

#ifndef C13_LOGIC_HPP
#define C13_LOGIC_HPP

//========================================================================================================================================================
//  INCLUDES
//========================================================================================================================================================

#include "Campaign_Logic.hpp"

//========================================================================================================================================================
//  DEFINES
//========================================================================================================================================================

//========================================================================================================================================================
//  TYPES
//========================================================================================================================================================

class c13_logic : public campaign_logic
{

//--------------------------------------------------------------------------------------------------------------------------------------------------------
//  Public Functions
//--------------------------------------------------------------------------------------------------------------------------------------------------------

public:

virtual void    BeginGame       ( void );
virtual void    EndGame         ( void );
virtual void    AdvanceTime     ( f32 DeltaTime );
virtual void    RegisterItem    ( object::id ItemID, const char* pTag );

//--------------------------------------------------------------------------------------------------------------------------------------------------------
//  Private Types
//--------------------------------------------------------------------------------------------------------------------------------------------------------

protected:

    enum game_status
    {
        STATE_IDLE,
        STATE_MOVING_OUT,
        STATE_BATTLE,
    };
    
//--------------------------------------------------------------------------------------------------------------------------------------------------------
//  Private Functions
//--------------------------------------------------------------------------------------------------------------------------------------------------------

protected:
        
        void        MovingOut       ( void );
        void        Battle          ( void );

//--------------------------------------------------------------------------------------------------------------------------------------------------------
//  Private Data
//--------------------------------------------------------------------------------------------------------------------------------------------------------

protected:

    //
    // Objects
    //
    
    object::id      m_StartPos;
    object::id      m_EnemyBase;
    object::id      m_Switch;
    
    //
    // Flags
    //

    xbool           m_bAlliesStarted;
    xbool           m_bMovingOutWarning1;
    xbool           m_bMovingOutWarning2;
    xbool           m_bHitSwitch;
    xbool           m_bGetEnemies;
    xbool           m_bGetSwitchBack;
};

//========================================================================================================================================================
//  GLOBAL VARIABLES
//========================================================================================================================================================

extern c13_logic C13_Logic;

//========================================================================================================================================================
#endif
//========================================================================================================================================================
