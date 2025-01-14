//========================================================================================================================================================
//
//  Campaign 11 Logic
//
//========================================================================================================================================================

#ifndef C11_LOGIC_HPP
#define C11_LOGIC_HPP

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

class c11_logic : public campaign_logic
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
virtual void    ItemEnabled     ( object::id ItemID, object::id OriginID );

//--------------------------------------------------------------------------------------------------------------------------------------------------------
//  Private Types
//--------------------------------------------------------------------------------------------------------------------------------------------------------

protected:

    enum game_status
    {
        STATE_IDLE,
        STATE_PREPARATION,
        STATE_FIGHT,
    };
    
//--------------------------------------------------------------------------------------------------------------------------------------------------------
//  Private Functions
//--------------------------------------------------------------------------------------------------------------------------------------------------------

protected:
        
        void        Preparation     ( void );
        void        Fight           ( void );
        void        StayNearGens    ( void );

//--------------------------------------------------------------------------------------------------------------------------------------------------------
//  Private Data
//--------------------------------------------------------------------------------------------------------------------------------------------------------

protected:

    //
    // Objects
    //

    object::id      m_Generator1;
    object::id      m_Generator2;
    object::id      m_TargetGenerator;
    
    //
    // Flags
    //

    xbool           m_bWarning1;
    xbool           m_bWarning2;
    xbool           m_bGenerator1Disabled;
    xbool           m_bGenerator2Disabled;
    xbool           m_bGenerator1Warning;
    xbool           m_bGenerator2Warning;
    xbool           m_bGenerator1Online;
    xbool           m_bGenerator2Online;
    xbool           m_bWave2Warning;
    xbool           m_bWave3Warning;
    xbool           m_bAttackWave1;
    xbool           m_bAttackWave2;
    xbool           m_bAttackWave3;

    //
    // Variables
    //
    
    f32             m_Time2;
};

//========================================================================================================================================================
//  GLOBAL VARIABLES
//========================================================================================================================================================

extern c11_logic C11_Logic;

//========================================================================================================================================================
#endif
//========================================================================================================================================================

