//==============================================================================
//
//  ColliderCannon.hpp
//
//==============================================================================

#ifndef COLLIDER_CANNON_HPP
#define COLLIDER_CANNON_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include "e_View.hpp"

//==============================================================================
//  FUNCTIONS
//==============================================================================

void    cc_Aim      ( const vector3& Start, const vector3& Direction );
void    cc_Fire     ( void );
void    cc_Render   ( void );

xcolor  GetAmbientLighting( const vector3& Point, const vector3& RayDir, f32 AmbientHue, f32 AmbientIntensity );

//==============================================================================
#endif // COLLIDER_CANNON_HPP
//==============================================================================

