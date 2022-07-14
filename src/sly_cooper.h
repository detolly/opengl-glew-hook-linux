
#pragma once

/**
    *  RPCS3 Version: 0.0.23-13875-0c6df39a Alpha
*/
enum Sly1ShaderType : unsigned int
{
    SANZARU_VIDEO = 44,             // Renders sanzaru video.
    ALL_VIDEO = 45,                 // Renders all video.
    INTRO_UNKNOWN = 46,             // Is being called in intro, but not sure what this does.

    SKYBOX = 47,                    // Renders the skybox.
    SKYBOX_2 = 48,                  // Renders the skybox.

    MODEL_2 = 49,                   // Not sure.
    MODEL_3 = 50,                   // Not sure.
    
    ANIMATED_MODEL = 51,            // Models. Also the cannon in world 1. Not coins.
    STATIC_FADING = 52,             // Fading static objects.
    
    UNKNOWN_1 = 53,                 // Not sure.
    UNKNOWN_2 = 54,                 // Not sure.

    GUI_1 = 55,                     // Removes the GUI layer of the game. Bentley's head however is still on the cutscenes. Pretty weird. Does not remvoe map background.
    GUI_2 = 56,                     // Same as MENU_1.
    
    UNKNOWN_3 = 57,                   // Not sure if it was ever ran.
    
    ROLLING_TEXTURE = 58,           // Rolling textures.
    MAP_BACKGROUND = 59,            // Map Background.
    OUTLINES = 60,                  // Outlines (stencil testing?)
    STATIC_NONFADING = 61,          // Non-fading static objects.
    
    UNKNOWN_4 = 62,                 // Not sure if it was ever ran.
    UNKNOWN_5 = 63,                 // Not sure if it was ever ran.
    UNKNOWN_6 = 64,                 // Not sure.
    UNKNOWN_7 = 65,                 // Not sure
    UNKNOWN_8= 66,                 // Not sure.
};