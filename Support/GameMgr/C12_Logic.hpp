//========================================================================================================================================================
//
//  Campaign 12 Logic
//
//========================================================================================================================================================

#ifndef C12_LOGIC_HPP
#define C12_LOGIC_HPP

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

class c12_logic : public campaign_logic
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
        STATE_MISSION,
    };
    
//--------------------------------------------------------------------------------------------------------------------------------------------------------
//  Private Functions
//--------------------------------------------------------------------------------------------------------------------------------------------------------

protected:
        
        void        Mission         ( void );

//--------------------------------------------------------------------------------------------------------------------------------------------------------
//  Private Data
//--------------------------------------------------------------------------------------------------------------------------------------------------------

protected:

    //
    // Objects
    //
    
    object::id      m_StartPosition;
    object::id      m_EnemyPosition;

    //
    // Flags
    //

    xbool           m_bAlliesStarted;
    xbool           m_bAlliesWithinRange;
    xbool           m_bPlayerWithinRange;    
};

//========================================================================================================================================================
//  GLOBAL VARIABLES
//========================================================================================================================================================

extern c12_logic C12_Logic;

//========================================================================================================================================================
#endif
//========================================================================================================================================================

