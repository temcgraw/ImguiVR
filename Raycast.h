#ifndef __RAYCAST_H__
#define __RAYCAST_H__

#include "glm/glm.hpp"

void DrawRaycast(glm::mat4& Pvr, glm::mat4& Vvr, glm::mat4& Mvr);
void Draw2dGui();
void InitRaycast();
void ReloadShader();

#endif