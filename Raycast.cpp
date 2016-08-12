//Basic raycasting setup: render cube with ray start and end coords. Perform raycasting in the fragment shader.


#include "glm/gtc/type_ptr.hpp"
#include <GL/glew.h>
#include "glm/glm.hpp"
#include <glm/gtx/transform.hpp>
#include "imgui_impl_vr.h"
#include "imgui_draw_3d.h"

#include "InitShader.h"

#include <string>
#include <iostream>

static const std::string bkg_vertex_shader("ray_vs.glsl");
static const std::string bkg_fragment_shader("ray_fs.glsl");
GLuint bkg_shader_program = -1;

struct CubeData
{
   GLuint cube_vao;
   GLuint cube_vbo;
   GLuint cube_ibo;
};
CubeData cube;

//uniform variables that get sent to the shader
int shader_mode = 0;
float shader_params[10] = {0.0f};
glm::vec4 shader_color[4] = {glm::vec4(0.8, 0.7, 0.6, 1.0), glm::vec4(0.7, 0.8, 0.9, 1.0), glm::vec4(1.0, 1.0, 1.0, 1.0), glm::vec4(1.0, 1.0, 1.0, 1.0)};

void glError()
{
   GLenum errCode;
   if((errCode = glGetError()) != GL_NO_ERROR)
   {
	 std::cout << "OpenGL Error: " << errCode << std::endl;
   }
}

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

void CreateCube()
{
	glGenVertexArrays(1, &cube.cube_vao);
	glBindVertexArray(cube.cube_vao);

	using namespace glm;
	vec3 pos[8] = {vec3(-1.0f, -1.0f, -1.0f), vec3(+1.0f, -1.0f, -1.0f), vec3(+1.0f, +1.0f, -1.0f), vec3(-1.0f, +1.0f, -1.0f),
	vec3(-1.0f, -1.0f, +1.0f), vec3(+1.0f, -1.0f, +1.0f), vec3(+1.0f, +1.0f, +1.0f), vec3(-1.0f, +1.0f, +1.0f)};

	vec3 tex[8] = {vec3(-1.0f, -1.0f, -1.0f), vec3(+1.0f, -1.0f, -1.0f), vec3(+1.0f, +1.0f, -1.0f), vec3(-1.0f, +1.0f, -1.0f),
	vec3(-1.0f, -1.0f, +1.0f), vec3(+1.0f, -1.0f, +1.0f), vec3(+1.0f, +1.0f, +1.0f), vec3(-1.0f, +1.0f, +1.0f)};

	unsigned short idx[36] = { 0,2,1, 2,0,3, //bottom
	0,5,4, 5,0,1, //front
	1,6,5, 6,1,2, //right 
	2,7,6, 7,2,3, //back
	3,4,7, 4,3,0, //left
	4,5,6, 6,7,4};//top
	//Buffer vertices
	int datasize = sizeof(pos) + sizeof(tex);
	glGenBuffers(1, &cube.cube_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, cube.cube_vbo);
	glBufferData(GL_ARRAY_BUFFER, datasize, 0, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(pos), pos);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(pos), sizeof(tex), tex);
	

	//Buffer indices
	glGenBuffers(1, &cube.cube_ibo);
	int nIndices = 12;
	int indexsize = sizeof(idx);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube.cube_ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexsize, idx, GL_STATIC_DRAW);


	int pos_loc = 0;
	int tex_coord_loc = 2;
	glEnableVertexAttribArray(pos_loc);
	glEnableVertexAttribArray(tex_coord_loc);

	glVertexAttribPointer(pos_loc, 3, GL_FLOAT, false, 0, BUFFER_OFFSET(0));
	glVertexAttribPointer(tex_coord_loc, 3, GL_FLOAT, false, 0, BUFFER_OFFSET(8*3*sizeof(float)));

	glBindVertexArray(0);
	glError();
}

void DrawCube()
{
	glBindVertexArray(cube.cube_vao);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));
	glBindVertexArray(0);
}

void Draw2dGui()
{
	if(ImGui_Impl_VR_IsHidden() == true)
	{
		return;
	}

	static int tab = 0;
	ImGui_Impl_VR_NewFrame(0);
	ImGui::Begin("Tab 0");
	bool t0 = ImGui::RadioButton("Tab 0", &tab, 0); ImGui::SameLine();
	bool t1 = ImGui::RadioButton("1", &tab, 1); ImGui::SameLine();
	bool t2 = ImGui::RadioButton("2", &tab, 2); ImGui::SameLine();
	bool t3 = ImGui::RadioButton("3", &tab, 3);

	if(t0 || t1 || t2 || t3)
	{
		SetQuadTexture(tab);  
	}


	static int e = 2;
	const float scales[4] = {0.25f, 0.5f, 1.0f, 1.5f};
	bool e0 = ImGui::RadioButton("Scale 0.25", &e, 0); ImGui::SameLine();
	bool e1 = ImGui::RadioButton("0.5", &e, 1); ImGui::SameLine();
	bool e2 = ImGui::RadioButton("1.0", &e, 2); ImGui::SameLine();
	bool e3 = ImGui::RadioButton("1.5", &e, 3);

	if(e0 || e1 || e2 || e3)
	{
		ImGui_Impl_VR_SetGuiScale(scales[e]);  
	}

	static bool pinned = false;
	if(ImGui::Checkbox("Pin", &pinned))
	{
		ImGui_Impl_VR_SetPinned(pinned);	
	}

	if(ImGui::Button("Screenshot"))
	{
		ImGui_Impl_VR_GrabScreenshot();	
	}

	static bool keyboard = false;
	if(ImGui::Checkbox("Show Keyboard", &keyboard))
	{
		if(keyboard == true)
		{
			ImGui_Impl_VR_ShowKeyboard();
		}
		else
		{
			ImGui_Impl_VR_HideKeyboard();
		}
	}

	//static bool test_opened = true;
	//ImGui::ShowTestWindow(&test_opened);
	ImGui::End();
	ImGui_Impl_VR_Render(0);

	ImGui_Impl_VR_NewFrame(1);
	ImGui::Begin("Tab 1");
	{
	bool t0 = ImGui::RadioButton("Tab 0", &tab, 0); ImGui::SameLine();
	bool t1 = ImGui::RadioButton("1", &tab, 1); ImGui::SameLine();
	bool t2 = ImGui::RadioButton("2", &tab, 2); ImGui::SameLine();
	bool t3 = ImGui::RadioButton("3", &tab, 3);

	if(t0 || t1 || t2 || t3)
	{
		SetQuadTexture(tab);  
	}
	}
	static char text1[1024];
	ImGui_Impl_VR_InputText("Sample text input 1", text1, 1024);

	static char text2[1024];
	ImGui_Impl_VR_InputText("Sample text input 2", text2, 1024);


	if(ImGui::SliderInt("shader mode", &shader_mode, 0, 10))
	{
      
	}
	ImGui::End();
	ImGui_Impl_VR_Render(1);

	ImGui_Impl_VR_NewFrame(2);
	ImGui::Begin("Tab 2");
	{
		bool t0 = ImGui::RadioButton("Tab 0", &tab, 0); ImGui::SameLine();
		bool t1 = ImGui::RadioButton("1", &tab, 1); ImGui::SameLine();
		bool t2 = ImGui::RadioButton("2", &tab, 2); ImGui::SameLine();
		bool t3 = ImGui::RadioButton("3", &tab, 3);

		if(t0 || t1 || t2 || t3)
		{
			SetQuadTexture(tab);  
		}
	}
	const int n_sliders = 10;
	for (int i = 0; i<n_sliders; i++)
	{
		std::string name = std::string("Slider ") + std::to_string(i);
		ImGui::SliderFloat(name.c_str(), &shader_params[i], 0.0f, +1.0f);
	}
	ImGui::End();
	ImGui_Impl_VR_Render(2);


	ImGui_Impl_VR_NewFrame(3);
	ImGui::Begin("Tab 3");
	{
		bool t0 = ImGui::RadioButton("Tab 0", &tab, 0); ImGui::SameLine();
		bool t1 = ImGui::RadioButton("1", &tab, 1); ImGui::SameLine();
		bool t2 = ImGui::RadioButton("2", &tab, 2); ImGui::SameLine();
		bool t3 = ImGui::RadioButton("3", &tab, 3);

		if(t0 || t1 || t2 || t3)
		{
			SetQuadTexture(tab);  
		}
	}
	for(int i=0; i<4; i++)
	{
		std::string name = std::string("Color ") + std::to_string(i);
		if(ImGui::ColorEdit4(name.c_str(), &shader_color[i].r, true))
		{

		}
	}
	ImGui::End();
	ImGui_Impl_VR_Render(3);

}


void DrawRaycast(glm::mat4& Pvr, glm::mat4& Vvr, glm::mat4& Mvr)
{

	float nearClip = 1.0f;
	float farClip = 5.0f;

	static float frame = 0.0f;
	static int frame_id = 0;
	frame += 1.0f/30.0f;
	
	frame_id++;

	glError();

	const glm::mat4 Q1 = glm::inverse(Vvr*Mvr)*glm::scale(glm::vec3(nearClip));
	const glm::mat4 Q2 = glm::inverse(Vvr*Mvr)*glm::scale(glm::vec3(farClip));

	glUseProgram(bkg_shader_program);
	glm::mat4 PVM = Pvr*Vvr*Mvr;
	glUniformMatrix4fv(0, 1, false, glm::value_ptr(PVM));
	glUniformMatrix4fv(1, 1, false, glm::value_ptr(Pvr));
	glUniformMatrix4fv(2, 1, false, glm::value_ptr(Q1));
	glUniformMatrix4fv(3, 1, false, glm::value_ptr(Q2));
	glUniform1f(4, frame);

	glUniform1i(8, shader_mode);
	glUniform1fv(9, 10, shader_params);
	glUniform4fv(19, 4, glm::value_ptr(shader_color[0]));

	DrawCube();
}

void ReloadShader()
{
   GLuint new_shader = InitShader(bkg_vertex_shader.c_str(), bkg_fragment_shader.c_str());
  
   if(new_shader == -1) // loading failed
   {
      glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
   }
   else
   {
      glClearColor(0.35f, 0.35f, 0.35f, 0.0f);

      if(bkg_shader_program != -1)
      {
         glDeleteProgram(bkg_shader_program);
      }
      bkg_shader_program = new_shader;
   }
}


void InitRaycast()
{
	bkg_shader_program = InitShader(bkg_vertex_shader.c_str(), bkg_fragment_shader.c_str());
	CreateCube();
	SetQuadTexture(0);
}