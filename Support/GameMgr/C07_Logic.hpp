//========================================================================================================================================================
//
//  Campaign 07 Logic
//
//========================================================================================================================================================

#ifndef C07_LOGIC_HPP
#define C07_LOGIC_HPP

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

class c07_logic : public campaign_logic
{

//--------------------------------------------------------------------------------------------------------------------------------------------------------
//  Public Functions
//--------------------------------------------------------------------------------------------------------------------------------------------------------

public:

virtual void    BeginGame       ( void );
virtual void    EndGame         ( void );
virtual void    AdvanceTime     ( f32 DeltaTime );
virtual void    RegisterItem    ( object::id ItemID, const char* pTag );
virtual void    ItemDisabled    ( object::id ItemID, object::id OriginID );
virtual void    ItemRepaired    ( object::id ItemID, object::id OriginID, xbool Score );

//--------------------------------------------------------------------------------------------------------------------------------------------------------
//  Private Types
//--------------------------------------------------------------------------------------------------------------------------------------------------------

protected:

    enum game_status
    {
        STATE_IDLE,
        STATE_MOVING_IN,
        STATE_INFILTRATION,
    };
    
//--------------------------------------------------------------------------------------------------------------------------------------------------------
//  Private Functions
//--------------------------------------------------------------------------------------------------------------------------------------------------------

protected:
        
        void        MovingIn        ( void );
        void        Infiltration    ( void );

//--------------------------------------------------------------------------------------------------------------------------------------------------------
//  Private Data
//--------------------------------------------------------------------------------------------------------------------------------------------------------

protected:

    //
    // Objects
    //

    object::id      m_EnemyBase;
    object::id      m_GeneratorRoom;
    object::id      m_Generator1;
    object::id      m_Generator2;
    
    //
    // Flags
    //

    xbool           m_bMovingInInfo;
    xbool           m_bMovingInWarning;
    xbool           m_bInfiltrationWarning;
    xbool           m_bGeneratorsDisabled;
    xbool           m_bGenerator1Disabled;
    xbool           m_bGenerator2Disabled;
    xbool           m_bMopUp;
};

//========================================================================================================================================================
//  GLOBAL VARIABLES
//========================================================================================================================================================

extern c07_logic C07_Logic;

//========================================================================================================================================================
#endif
//========================================================================================================================================================
