/******************************************************************************/
/*!
\file   VideoRenderer.hpp
\author Team PASSTA
        Chew Bangxin Steven (banxginsteven.chew@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/09
\brief    Declares the video class

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef ENGINE_VIDEO_H
#define ENGINE_VIDEO_H

#include "Ecs/ecs.h"

struct VideoSystem : public ecs::SystemBase {
    void Update(ecs::world&);
    void FixedUpdate(ecs::world&);
};

#endif