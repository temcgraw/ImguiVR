//Draws a single quad ui using imgui_impl_vr.

#ifndef __IMGUI_DRAW_3D_H__
#define __IMGUI_DRAW_3D_H__

#include "glm/glm.hpp"

void SetQuadTexture(int i);
void Draw3dGui(glm::mat4& Pvr, glm::mat4& Vvr, glm::mat4& Mvr);
void Draw3dGuiQuad(glm::mat4& Pvr, glm::mat4& Vvr, glm::mat4& Mvr);
void Draw3dGuiCube(glm::mat4& Pvr, glm::mat4& Vvr, glm::mat4& Mvr);
void Tick(float deltaSeconds);
void Init3dGui();

#endif