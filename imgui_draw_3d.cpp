#include <GL/glew.h>

#include <glm/gtx/transform.hpp>
#include "glm/gtc/type_ptr.hpp"

#include <string>

#include "InitShader.h"
#include "imgui_impl_vr.h"
#include "imgui_draw_3d.h"
#include <openvr.h>

struct QuadData
{
   GLuint quad_vao;
   GLuint quad_vbo;
};
QuadData quad;


static const std::string gui_vertex_shader("gui_vs.glsl");
static const std::string gui_fragment_shader("gui_fs.glsl");
GLuint gui_shader_program = -1;

glm::vec4 BaseColor(1.0f);


static int tex_id = 0;

void SetQuadTexture(int i)
{
	tex_id = i;
	for(int tab=0; tab<4; tab++)
	{
		glm::mat4 M = glm::scale(glm::vec3(0.0f));
		if(tab==i)
		{
			M = glm::scale(glm::vec3(1.0f));	
		}
		ImGui_Impl_VR_SetGuiRotation(tab, M);
	}
}

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

void CreateQuad()
{
   glGenVertexArrays(1, &quad.quad_vao);
   glBindVertexArray(quad.quad_vao);

   float vertices[] = { 1.0f, 1.0f, 0.0f, 1.0f, -1.0f, 0.0f, -1.0f, 1.0f, 0.0f, -1.0f, -1.0f, 0.0f };

   //create vertex buffers for vertex coords
   glGenBuffers(1, &quad.quad_vbo);
   glBindBuffer(GL_ARRAY_BUFFER, quad.quad_vbo);
   glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
   int pos_loc = 0;
   glEnableVertexAttribArray(pos_loc);
   glVertexAttribPointer(pos_loc, 3, GL_FLOAT, false, 0, 0);

   glBindVertexArray(0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
}


void Draw3dGui(glm::mat4& Pvr, glm::mat4& Vvr, glm::mat4& Mvr)
{
	//Draw3dGuiQuad(Pvr, Vvr, Mvr);
	Draw3dGuiCube(Pvr, Vvr, Mvr);
}

void Draw3dGuiQuad(glm::mat4& Pvr, glm::mat4& Vvr, glm::mat4& Mvr)
{
	if(ImGui_Impl_VR_IsHidden() == true)
	{
		return;
	}

	//Fade if mouse is outside UI
	if(ImGui::IsMouseHoveringAnyWindow())
	{
		BaseColor = glm::vec4(1.0f);
	}
	else
	{
		BaseColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.5f);
	}
	
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glUseProgram(gui_shader_program);

	glm::mat4 PVM = Pvr*Vvr*Mvr*ImGui_Impl_VR_GetGuiPose()*ImGui_Impl_VR_GetGuiScale();
	glUniformMatrix4fv(0, 1, false, glm::value_ptr(PVM));
	glUniform4fv(2, 1, glm::value_ptr(BaseColor));
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, ImGui_Impl_VR_GetGuiTexture(tex_id));
	glUniform1f(1, 0);
	glBindVertexArray(quad.quad_vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);

}

void Draw3dGuiCube(glm::mat4& Pvr, glm::mat4& Vvr, glm::mat4& Mvr)
{
	if(ImGui_Impl_VR_IsHidden() == true)
	{
		return;
	}

	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glUseProgram(gui_shader_program);
	glActiveTexture(GL_TEXTURE0);
	glUniform1f(1, 0);
	glBindVertexArray(quad.quad_vao);

	//Fade if mouse is outside UI
	if(ImGui::IsMouseHoveringAnyWindow())
	{
		BaseColor = glm::vec4(1.0f);
	}
	else
	{
		BaseColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.5f);
	}

	const GLuint cullmodes[2] = {GL_FRONT, GL_BACK};
	const glm::vec4 cullcolors[2] = {glm::vec4(0.5f), glm::vec4(1.0f)};

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);

	static float rot_angle = 0.0f;
	static float last_drag_x = 0.0f;
	float current_drag_x = ImGui_Impl_VR_GetAxisDragDelta(0).x;

	if(current_drag_x==0.0f)
	{
		rot_angle += last_drag_x;
		last_drag_x = 0.0f;
	}
	else
	{
		last_drag_x = current_drag_x;
	}

	const float angle = rot_angle + current_drag_x;

	const glm::mat4 T = glm::translate(glm::vec3(0.0f, 0.0f, 1.0f));
	const glm::mat4 Tcube = glm::translate(glm::vec3(0.0f, 0.0f, -1.0f));

	for(int f=0; f<2; f++)
	{
		glCullFace(cullmodes[f]);
		for(int i=0; i<4; i++)
		{
			glm::mat4 R = glm::rotate(i*0.5f*3.14159268f + angle, glm::vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 Mcube = Tcube*R*T;
			ImGui_Impl_VR_SetGuiRotation(i, Mcube);

			glm::mat4 PVM = Pvr*Vvr*Mvr*ImGui_Impl_VR_GetGuiPose()*ImGui_Impl_VR_GetGuiScale()*Mcube;
			glUniformMatrix4fv(0, 1, false, glm::value_ptr(PVM));
			glUniform4fv(2, 1, glm::value_ptr(BaseColor*cullcolors[f]));
			glBindTexture(GL_TEXTURE_2D, ImGui_Impl_VR_GetGuiTexture(i));
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}
	}
	glBindVertexArray(0);
	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);

}

void Init3dGui()
{
	gui_shader_program = InitShader(gui_vertex_shader.c_str(), gui_fragment_shader.c_str());
	CreateQuad();
}