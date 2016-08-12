// Modified version of ImGui SDL2 binding with OpenGL3.
// TODO: remove dependencies on SDL...



// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#include "imgui/imgui.h"
#include "imgui_impl_vr.h"
#include "imgui/imgui_internal.h"

// SDL,GLEW
#include <SDL.h>
#include <SDL_syswm.h>
#include <openvr.h>
#include <GL/glew.h>
#include <glm/gtx/intersect.hpp>
#include <glm/gtx/transform.hpp>

// Data
static vr::IVRSystem*		g_HMD;
static vr::IVRScreenshots*	g_pScreenShots;
static vr::IVROverlay*		g_pOverlay;

static double       g_Time = 0.0f;
static GLuint       g_FontTexture = 0;
static int          g_ShaderHandle = 0, g_VertHandle = 0, g_FragHandle = 0;
static int          g_AttribLocationTex = 0, g_AttribLocationProjMtx = 0;
static int          g_AttribLocationPosition = 0, g_AttribLocationUV = 0, g_AttribLocationColor = 0;
static unsigned int g_VboHandle = 0, g_VaoHandle = 0, g_ElementsHandle = 0;

static glm::mat4	g_Mgui = glm::scale(glm::vec3(1.0f)); //GUI controller pose
static glm::mat4	g_MguiScale = glm::scale(glm::vec3(1.0f)); //GUI 3d modeling matrix
static glm::mat4	g_MguiRotation[4] = {glm::scale(glm::vec3(1.0f)), glm::scale(glm::vec3(1.0f)), glm::scale(glm::vec3(1.0f)), glm::scale(glm::vec3(1.0f))}; //GUI 3d rotation matrix
static glm::mat4	g_Mmouse = glm::scale(glm::vec3(1.0f)); // Mouse controller pose


const int			g_MaxTextures = 4;
static GLuint		g_GuiTexture[g_MaxTextures] = {0,0,0,0};
static int			g_TexWidth = 0;
static int			g_TexHeight = 0;
static GLuint		g_GuiFBO = 0;
static int			g_LastViewport[4] = {0,0,0,0};
static bool			g_IsHidden = false;
static bool			g_Pinned = false;

static ImVec2		g_AxisPos[2] = {ImVec2(0.0f, 0.0f), ImVec2(0.0f, 0.0f)};
static ImVec2		g_AxisClickedPos[2] = {ImVec2(0.0f, 0.0f), ImVec2(0.0f, 0.0f)};
static int			g_ActiveTexture = -1;

static char* InputTextBuf = 0;
static size_t InputTextBufSize = 0;

static ImGuiContext* g_Context[g_MaxTextures];

ImVec2 ImGui_Impl_VR_GetAxisPos(int i)
{
	return g_AxisPos[i];
}

ImVec2 ImGui_Impl_VR_GetAxisDragDelta(int i)
{
	return ImVec2(g_AxisPos[i].x-g_AxisClickedPos[i].x, g_AxisPos[i].y-g_AxisClickedPos[i].y);  //where is operator- ?
}

void ImGui_Impl_VR_SetPinned(bool pinned)
{
	g_Pinned = pinned;
}

bool ImGui_Impl_VR_IsHidden()
{
	return g_IsHidden;
}

glm::mat4 ImGui_Impl_VR_GetGuiPose()
{
	return g_Mgui;
}

glm::mat4 ImGui_Impl_VR_GetGuiScale()
{
	return g_MguiScale;
}

 void ImGui_Impl_VR_SetGuiScale(float s)
{
	g_MguiScale = glm::scale(glm::vec3(s));
}

int  ImGui_Impl_VR_GetGuiTexture(int i)
{
	return g_GuiTexture[i];
}

void ImGui_Impl_VR_SetGuiRotation(int i, glm::mat4& R)
{
	if(i<0 || i>=4)
	{
		return;
	}
	g_MguiRotation[i] = R;
}

glm::mat4 ImGui_Impl_VR_GetGuiRotation(int i)
{
	return g_MguiRotation[i];
}

void ImGui_Impl_VR_TriggerHapticPulse(vr::TrackedDeviceIndex_t device)
{
	const int left_controller = g_HMD->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_LeftHand);
	const uint32_t axis = 0; //there is currently only one actuator
	g_HMD->TriggerHapticPulse(left_controller, axis, 500);
}

void ImGui_Impl_VR_GrabScreenshot()
{
	vr::ScreenshotHandle_t handle;
	g_pScreenShots->RequestScreenshot( &handle, vr::VRScreenshotType_Stereo, "preview_ss", "vr_ss" );
}

void ImGui_Impl_VR_ShowKeyboard()
{
	g_pOverlay->ShowKeyboard( vr::k_EGamepadTextInputModeNormal, vr::k_EGamepadTextInputLineModeSingleLine, "Sample Keyboard", 1024, "", false, 0 );
}

void ImGui_Impl_VR_HideKeyboard()
{
	g_pOverlay->HideKeyboard();
}


// This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
// If text or lines are blurry when integrating ImGui in your engine:
// - in your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)
void ImGui_Impl_VR_RenderDrawLists(ImDrawData* draw_data)
{
	// Backup GL state
	GLint last_program; glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
	GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
	GLint last_array_buffer; glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
	GLint last_element_array_buffer; glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
	GLint last_vertex_array; glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
	GLint last_blend_src; glGetIntegerv(GL_BLEND_SRC, &last_blend_src);
	GLint last_blend_dst; glGetIntegerv(GL_BLEND_DST, &last_blend_dst);
	GLint last_blend_equation_rgb; glGetIntegerv(GL_BLEND_EQUATION_RGB, &last_blend_equation_rgb);
	GLint last_blend_equation_alpha; glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &last_blend_equation_alpha);
    GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
	GLboolean last_enable_blend = glIsEnabled(GL_BLEND);
	GLboolean last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
	GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
	GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);

	// Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_SCISSOR_TEST);
	glActiveTexture(GL_TEXTURE0);

	// Handle cases of screen coordinates != from framebuffer coordinates (e.g. retina displays)
	ImGuiIO& io = ImGui::GetIO();
    int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
	draw_data->ScaleClipRects(io.DisplayFramebufferScale);

	// Setup orthographic projection matrix
    glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
	const float ortho_projection[4][4] =
	{
		{ 2.0f/io.DisplaySize.x, 0.0f,                   0.0f, 0.0f },
		{ 0.0f,                  2.0f/-io.DisplaySize.y, 0.0f, 0.0f },
		{ 0.0f,                  0.0f,                  -1.0f, 0.0f },
		{-1.0f,                  1.0f,                   0.0f, 1.0f },
	};
	glUseProgram(g_ShaderHandle);
	glUniform1i(g_AttribLocationTex, 0);
	glUniformMatrix4fv(g_AttribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);
	glBindVertexArray(g_VaoHandle);

	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		const ImDrawIdx* idx_buffer_offset = 0;

		glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.size() * sizeof(ImDrawVert), (GLvoid*)&cmd_list->VtxBuffer.front(), GL_STREAM_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ElementsHandle);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.size() * sizeof(ImDrawIdx), (GLvoid*)&cmd_list->IdxBuffer.front(), GL_STREAM_DRAW);

		for (const ImDrawCmd* pcmd = cmd_list->CmdBuffer.begin(); pcmd != cmd_list->CmdBuffer.end(); pcmd++)
		{
			if (pcmd->UserCallback)
			{
				pcmd->UserCallback(cmd_list, pcmd);
			}
			else
			{
				glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
				glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
				glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer_offset);
			}
			idx_buffer_offset += pcmd->ElemCount;
		}
	}

	// Restore modified GL state
	glUseProgram(last_program);
	glBindTexture(GL_TEXTURE_2D, last_texture);
	glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
	glBindVertexArray(last_vertex_array);
	glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
	glBlendFunc(last_blend_src, last_blend_dst);
	if (last_enable_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
	if (last_enable_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
	if (last_enable_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
	if (last_enable_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
}

static const char* ImGui_Impl_VR_GetClipboardText()
{
	//return SDL_GetClipboardText(); //any openvr equivalent here?
	return 0;
}

static void ImGui_Impl_VR_SetClipboardText(const char* text)
{
	//SDL_SetClipboardText(text);
}

#include <stdio.h>

float last_axis = 0.0f;
bool axis_dragging = false;

void ImGui_Impl_VR_ProcessEvent(vr::VREvent_t* event)
{
	if(g_ActiveTexture != -1)
	{
		ImGui::SetCurrentContext(g_Context[g_ActiveTexture]);		
	}

	const int left_controller = g_HMD->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_LeftHand);
	const int right_controller = g_HMD->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_RightHand);
	ImGuiIO& io = ImGui::GetIO();

	if(event->trackedDeviceIndex == left_controller) //right controller is mouse
	{
		const vr::VREvent_Controller_t controller = event->data.controller;
		if(controller.button == vr::k_EButton_Axis0)
		{

			vr::VRControllerState_t state;
			if( g_HMD->GetControllerState( left_controller, &state ) )
			{
				if(event->eventType == vr::VREvent_ButtonTouch)
				{
					g_AxisPos[0] = ImVec2(state.rAxis[0].x, state.rAxis[0].y);
					g_AxisClickedPos[0] = g_AxisPos[0];
				}
				else if(event->eventType == vr::VREvent_ButtonUntouch)
				{
					g_AxisPos[0] = ImVec2(0.0f, 0.0f);
					g_AxisClickedPos[0] = g_AxisPos[0];
				}

			}
		}
	}

	if(event->trackedDeviceIndex == right_controller) //right controller is mouse
	{
		const vr::VREvent_Controller_t controller = event->data.controller;
		if(controller.button == vr::k_EButton_SteamVR_Trigger)
		{
			if(event->eventType == vr::VREvent_ButtonPress)
			{
				io.MouseDown[0] = true;
			}

			if(event->eventType == vr::VREvent_ButtonUnpress)
			{
				io.MouseDown[0] = false;
			}
		}

		if(controller.button == vr::k_EButton_Axis0)
		{

			vr::VRControllerState_t state;
			if( g_HMD->GetControllerState( right_controller, &state ) )
			{

				if(event->eventType == vr::VREvent_ButtonTouch)
				{
					g_AxisPos[1] = ImVec2(state.rAxis[0].x, state.rAxis[0].y);
					g_AxisClickedPos[1] = g_AxisPos[1];
					axis_dragging = true;
					last_axis = state.rAxis[0].y;
				}
				else if(event->eventType == vr::VREvent_ButtonUntouch)
				{
					axis_dragging = false;
					g_AxisPos[1] = ImVec2(0.0f, 0.0f);
					g_AxisClickedPos[1] = g_AxisPos[1];
				}

			}
		}

		if(controller.button == vr::k_EButton_ApplicationMenu)
		{
			if(event->eventType == vr::VREvent_ButtonPress)
			{
				g_IsHidden = !g_IsHidden;	
			}
		}
	}

	if(event->eventType == vr::VREvent_KeyboardCharInput) 
	{
		for(int i=0; i<8; i++)
		{
			//io.AddInputCharacter((unsigned short)event->data.keyboard.cNewInput[i]); //no worky. Why?
		}
	}

	if((event->eventType == vr::VREvent_KeyboardDone) && (InputTextBuf != 0) && (InputTextBufSize > 0)) 
	{
		g_pOverlay->GetKeyboardText(InputTextBuf, InputTextBufSize);
		for(unsigned int i=0; i<InputTextBufSize; i++)
		{
			io.AddInputCharacter((unsigned short)InputTextBuf[i]);
		}
		InputTextBuf = 0;
		InputTextBufSize = 0;
	}

	if(event->eventType == vr::VREvent_KeyboardClosed) 
	{

	}
	
}

void ImGui_Impl_VR_ProcessController(vr::TrackedDeviceIndex_t device, vr::VRControllerState_t* state, glm::mat4& M)
{
	if(g_ActiveTexture >= 0 && g_ActiveTexture < 4)
	{
		ImGui::SetCurrentContext(g_Context[g_ActiveTexture]);
	}

	const int left_controller = g_HMD->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_LeftHand);
	const int right_controller = g_HMD->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_RightHand);

	if(device == left_controller)
	{
		if(g_Pinned == false)
		{
			g_Mgui = M;
		}
		g_AxisPos[0] = ImVec2(state->rAxis[0].x, state->rAxis[0].y);
	}

	if(device == right_controller)
	{
		g_Mmouse = M;

		if(state->ulButtonTouched & ButtonMaskFromId(vr::k_EButton_SteamVR_Touchpad))
		{
			float new_axis = state->rAxis[0].y;
			if(axis_dragging == true)
			{

				if(g_ActiveTexture >= 0 && g_ActiveTexture < 4)
				{
					ImGuiIO& io = ImGui::GetIO();
					io.MouseWheel += new_axis - last_axis;
				}
			}
			last_axis = new_axis;
			g_AxisPos[1] = ImVec2(state->rAxis[0].x, state->rAxis[0].y);
		}
	}
}

void ImGui_Impl_VR_Poll3dMouse()
{
	//HACK: use ray/tri intersection tests
	const glm::vec3 p0(1.0f, 1.0f, 0.0f);
	const glm::vec3 p1(1.0f, -1.0f, 0.0f);
	const glm::vec3 p2(-1.0f, 1.0f, 0.0f);
	const glm::vec3 p3(-1.0f, -1.0f, 0.0f);

	for(int i=0; i<g_MaxTextures; i++)
	{
		ImGui::SetCurrentContext(g_Context[i]);

		const glm::mat4 M = glm::inverse(g_Mgui*g_MguiScale*g_MguiRotation[i])*g_Mmouse;
		const glm::vec3 start = glm::vec3(M * glm::vec4( 0, 0, -0.02f, 1 ));
		const glm::vec3 end = glm::vec3(M * glm::vec4( 0, 0, -39.f, 1 ));
		const glm::vec3 dir = end-start;

		glm::vec3 b, pt;
		bool intersect = glm::intersectRayTriangle(start, dir, p0, p2, p1, b);
		if(intersect == true)
		{
			pt = start + b.z*dir;
		}
		else if(intersect == false)
		{
			intersect = glm::intersectRayTriangle(start, dir, p3, p1, p2, b);
			if(intersect == true)
			{
				pt = start + b.z*dir;
			}
		}

		if(intersect==true)
		{
			ImGuiIO& io = ImGui::GetIO();
			io.MousePos = ImVec2(g_TexWidth*(0.5f*pt.x + 0.5f), g_TexHeight - g_TexHeight*(0.5f*pt.y + 0.5f)); 
			g_ActiveTexture = i;
		}
	}
}

void ImGui_Impl_VR_CreateFontsTexture()
{
	// Build texture atlas
    ImGuiIO& io = ImGui::GetIO();
	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bits for OpenGL3 demo because it is more likely to be compatible with user's existing shader.

    // Upload texture to graphics system
    GLint last_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
	glGenTextures(1, &g_FontTexture);
	glBindTexture(GL_TEXTURE_2D, g_FontTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	// Store our identifier
	io.Fonts->TexID = (void *)(intptr_t)g_FontTexture;

    // Restore state
    glBindTexture(GL_TEXTURE_2D, last_texture);
}

bool ImGui_Impl_VR_CreateDeviceObjects()
{
	// Backup GL state
	GLint last_texture, last_array_buffer, last_vertex_array;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
	glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);

	const GLchar *vertex_shader =
		"#version 330\n"
		"uniform mat4 ProjMtx;\n"
		"in vec2 Position;\n"
		"in vec2 UV;\n"
		"in vec4 Color;\n"
		"out vec2 Frag_UV;\n"
		"out vec4 Frag_Color;\n"
		"void main()\n"
		"{\n"
		"	Frag_UV = UV;\n"
		"	Frag_Color = Color;\n"
		"	gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
		"}\n";

	const GLchar* fragment_shader =
		"#version 330\n"
		"uniform sampler2D Texture;\n"
		"in vec2 Frag_UV;\n"
		"in vec4 Frag_Color;\n"
		"out vec4 Out_Color;\n"
		"void main()\n"
		"{\n"
		"	Out_Color = Frag_Color * texture( Texture, Frag_UV.st);\n"
		"}\n";

	g_ShaderHandle = glCreateProgram();
	g_VertHandle = glCreateShader(GL_VERTEX_SHADER);
	g_FragHandle = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(g_VertHandle, 1, &vertex_shader, 0);
	glShaderSource(g_FragHandle, 1, &fragment_shader, 0);
	glCompileShader(g_VertHandle);
	glCompileShader(g_FragHandle);
	glAttachShader(g_ShaderHandle, g_VertHandle);
	glAttachShader(g_ShaderHandle, g_FragHandle);
	glLinkProgram(g_ShaderHandle);

	g_AttribLocationTex = glGetUniformLocation(g_ShaderHandle, "Texture");
	g_AttribLocationProjMtx = glGetUniformLocation(g_ShaderHandle, "ProjMtx");
	g_AttribLocationPosition = glGetAttribLocation(g_ShaderHandle, "Position");
	g_AttribLocationUV = glGetAttribLocation(g_ShaderHandle, "UV");
	g_AttribLocationColor = glGetAttribLocation(g_ShaderHandle, "Color");

	glGenBuffers(1, &g_VboHandle);
	glGenBuffers(1, &g_ElementsHandle);

	glGenVertexArrays(1, &g_VaoHandle);
	glBindVertexArray(g_VaoHandle);
	glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
	glEnableVertexAttribArray(g_AttribLocationPosition);
	glEnableVertexAttribArray(g_AttribLocationUV);
	glEnableVertexAttribArray(g_AttribLocationColor);

#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
	glVertexAttribPointer(g_AttribLocationPosition, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, pos));
	glVertexAttribPointer(g_AttribLocationUV, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, uv));
	glVertexAttribPointer(g_AttribLocationColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, col));
#undef OFFSETOF

	ImGui_Impl_VR_CreateFontsTexture();

	glGenTextures(g_MaxTextures, g_GuiTexture);
	glActiveTexture(GL_TEXTURE0);
	for(int i=0; i<g_MaxTextures; i++)
	{
		glBindTexture(GL_TEXTURE_2D, g_GuiTexture[i]);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, g_TexWidth, g_TexHeight);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	glGenFramebuffers(1, &g_GuiFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, g_GuiFBO);
	for(int i=0; i<g_MaxTextures; i++)
	{
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+i, g_GuiTexture[i], 0);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Restore modified GL state
	glBindTexture(GL_TEXTURE_2D, last_texture);
	glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
	glBindVertexArray(last_vertex_array);

	return true;
}

void    ImGui_Impl_VR_InvalidateDeviceObjects()
{
    if (g_VaoHandle) glDeleteVertexArrays(1, &g_VaoHandle);
    if (g_VboHandle) glDeleteBuffers(1, &g_VboHandle);
    if (g_ElementsHandle) glDeleteBuffers(1, &g_ElementsHandle);
    g_VaoHandle = g_VboHandle = g_ElementsHandle = 0;

    glDetachShader(g_ShaderHandle, g_VertHandle);
    glDeleteShader(g_VertHandle);
    g_VertHandle = 0;

    glDetachShader(g_ShaderHandle, g_FragHandle);
    glDeleteShader(g_FragHandle);
    g_FragHandle = 0;

    glDeleteProgram(g_ShaderHandle);
    g_ShaderHandle = 0;

    if (g_FontTexture)
    {
        glDeleteTextures(1, &g_FontTexture);
        ImGui::GetIO().Fonts->TexID = 0;
        g_FontTexture = 0;
    }

	if(g_GuiTexture)
	{
		glDeleteTextures(g_MaxTextures, g_GuiTexture);
	}

	//TODO delete g_GuiFBO
}

bool ImGui_Impl_VR_Init(vr::IVRSystem* hmd, int w, int h)
{
	g_HMD = hmd;
	g_TexWidth = w;
	g_TexHeight = h;

	vr::EVRInitError eError = vr::VRInitError_None;
	g_pScreenShots = ( vr::IVRScreenshots * )vr::VR_GetGenericInterface( vr::IVRScreenshots_Version, &eError );
	g_pOverlay = ( vr::IVROverlay * )vr::VR_GetGenericInterface( vr::IVROverlay_Version, &eError );

	const ImColor title_colors[g_MaxTextures] = {ImColor(1.0f, 0.0f, 0.0f, 1.0f), ImColor(0.0f, 1.0f, 0.0f, 1.0f), ImColor(0.0f, 0.0f, 1.0f, 1.0f), ImColor(1.0f, 0.0f, 1.0f, 1.0f)};
	const ImColor title_active_colors[g_MaxTextures] = {ImColor(0.7f, 0.0f, 0.0f, 1.0f), ImColor(0.0f, 0.7f, 0.0f, 1.0f), ImColor(0.0f, 0.0f, 0.7f, 1.0f), ImColor(0.7f, 0.0f, 0.7f, 1.0f)};

	for(int i=0; i<g_MaxTextures; i++)
	{
		if(i==0)
		{
			g_Context[0] = ImGui::GetCurrentContext();
		}
		else
		{
			g_Context[i] = ImGui::CreateContext();
			ImGui::SetCurrentContext(g_Context[i]);
		}

		ImGuiStyle& style = ImGui::GetStyle();
		style.Colors[ImGuiCol_TitleBg] = title_colors[i];
		style.Colors[ImGuiCol_TitleBgActive] = title_active_colors[i];

		ImGuiIO& io = ImGui::GetIO();
		io.KeyMap[ImGuiKey_Tab] = SDLK_TAB;                     // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
		io.KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
		io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
		io.KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
		io.KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
		io.KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
		io.KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
		io.KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
		io.KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
		io.KeyMap[ImGuiKey_Delete] = SDLK_DELETE;
		io.KeyMap[ImGuiKey_Backspace] = SDLK_BACKSPACE;
		io.KeyMap[ImGuiKey_Enter] = SDLK_RETURN;
		io.KeyMap[ImGuiKey_Escape] = SDLK_ESCAPE;
		io.KeyMap[ImGuiKey_A] = SDLK_a;
		io.KeyMap[ImGuiKey_C] = SDLK_c;
		io.KeyMap[ImGuiKey_V] = SDLK_v;
		io.KeyMap[ImGuiKey_X] = SDLK_x;
		io.KeyMap[ImGuiKey_Y] = SDLK_y;
		io.KeyMap[ImGuiKey_Z] = SDLK_z;

		io.RenderDrawListsFn = ImGui_Impl_VR_RenderDrawLists;   // Alternatively you can set this to NULL and call ImGui::GetDrawData() after ImGui::Render() to get the same ImDrawData pointer.
		io.SetClipboardTextFn = ImGui_Impl_VR_SetClipboardText;
		io.GetClipboardTextFn = ImGui_Impl_VR_GetClipboardText;

		io.MouseDrawCursor = true; // let imgui draw the cursor in the HMD
		io.MouseDoubleClickTime = 0.50f;        // Time for a double-click, in seconds.
		io.MouseDoubleClickMaxDist = 8.0f;
		g_Context[i]->MouseCursorData[0].Size = ImVec2(10.0f, 10.0f);
	}

	return true;
}

void ImGui_Impl_VR_Shutdown()
{
    ImGui_Impl_VR_InvalidateDeviceObjects();
	ImGui::Shutdown();
}

void ImGui_Impl_VR_NewFrame(int i)
{
	ImGui::SetCurrentContext(g_Context[i]);
	ImGuiIO& io = ImGui::GetIO();
	if (!g_FontTexture)
	{
		ImGui_Impl_VR_CreateDeviceObjects();
	}
	else
	{
		io.Fonts->TexID = (void *)(intptr_t)g_FontTexture; //TODO: still getting a square block mouse cursor
	}

	// Setup display size (every frame to accommodate for window resizing)
	io.DisplaySize = ImVec2((float)g_TexWidth, (float)g_TexHeight);
	io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

	// Setup time step
	Uint32	time = SDL_GetTicks();
	double current_time = time / 1000.0;
	io.DeltaTime = g_Time > 0.0 ? (float)(current_time - g_Time) : (float)(1.0f / 60.0f);
	g_Time = current_time;

	//setup for render-to-texture
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glGetIntegerv(GL_VIEWPORT, g_LastViewport);
	glViewport(0, 0, g_TexWidth, g_TexHeight); 

	glBindFramebuffer(GL_FRAMEBUFFER, g_GuiFBO); 

	glDrawBuffer(GL_COLOR_ATTACHMENT0+i);
	float clearcolor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	glClearBufferfv(GL_COLOR, 0, clearcolor);

	// Start the frame
	ImGui::NewFrame();

}

void PulseIfItemHovered()
{
	static bool anyItemHoveredLastFrame = false;
	static bool anyWindowHoveredLastFrame = false;

	bool anyItemHoveredThisFrame = ImGui::IsAnyItemHovered();
	bool anyWindowHoveredThisFrame = ImGui::IsMouseHoveringAnyWindow();

	if((anyItemHoveredLastFrame != anyItemHoveredThisFrame) || (anyWindowHoveredLastFrame != anyWindowHoveredThisFrame))
	{
		ImGui_Impl_VR_TriggerHapticPulse(1);
	}
	anyItemHoveredLastFrame = anyItemHoveredThisFrame;
	anyWindowHoveredLastFrame = anyWindowHoveredThisFrame;
}

void  ImGui_Impl_VR_Render(int i)
{
		ImGui::SetCurrentContext(g_Context[i]);

		if(g_ActiveTexture == i)
		{
			PulseIfItemHovered(); // haptic pulse if window or item is hovered
		}

		ImGui::Render();

		static bool WantTextInputLastFrame[g_MaxTextures] = {false, false, false, false};
		ImGuiIO& io = ImGui::GetIO();
		if((io.WantTextInput == true)&&(WantTextInputLastFrame[i] == false)&&(InputTextBufSize != 0) && (InputTextBuf != 0))
		{
			g_pOverlay->ShowKeyboard( vr::k_EGamepadTextInputModeNormal, vr::k_EGamepadTextInputLineModeSingleLine, "Virtual Keyboard", InputTextBufSize, InputTextBuf, false, 0 );
		}
		WantTextInputLastFrame[i] = io.WantTextInput;
	

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDrawBuffer(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glViewport(g_LastViewport[0], g_LastViewport[1], g_LastViewport[2], g_LastViewport[3]); 
}

bool ImGui_Impl_VR_InputText(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags, ImGuiTextEditCallback callback, void* user_data)
{
	bool retval = ImGui::InputText(label, buf, buf_size, flags, callback, user_data);

	if(ImGui::IsItemActive() == true)
	{
		InputTextBuf = buf;
		InputTextBufSize = buf_size;
	}

	if(retval==true)
	{
		printf("");
	}
	return retval;
}