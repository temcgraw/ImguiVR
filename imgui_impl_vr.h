// ImGui in OpenVR
// Modified from ImGui SDL2 binding with OpenGL3
// In this binding, ImTextureID is used to store an OpenGL 'GLuint' texture identifier. Read the FAQ about ImTextureID in imgui.cpp.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#include <imgui/imgui.h>
#include "glm/glm.hpp"

struct VREvent_t;

namespace vr
{
	class IVRSystem;
	struct VREvent_t;
	struct VRControllerState001_t;

	typedef VRControllerState001_t VRControllerState_t;
	typedef unsigned int TrackedDeviceIndex_t;
};



IMGUI_API bool        ImGui_Impl_VR_Init(vr::IVRSystem* hmd, int w, int h);
IMGUI_API void        ImGui_Impl_VR_Shutdown();
IMGUI_API void        ImGui_Impl_VR_NewFrame(int i=0);
IMGUI_API void        ImGui_Impl_VR_Render(int i=0);
IMGUI_API void        ImGui_Impl_VR_ProcessEvent(vr::VREvent_t* event);
IMGUI_API void        ImGui_Impl_VR_ProcessController(vr::TrackedDeviceIndex_t device, vr::VRControllerState_t* state, glm::mat4& M);
IMGUI_API void        ImGui_Impl_VR_Poll3dMouse();

IMGUI_API int         ImGui_Impl_VR_GetGuiTexture(int i=0);
IMGUI_API glm::mat4   ImGui_Impl_VR_GetGuiPose();
IMGUI_API bool        ImGui_Impl_VR_IsHidden();
IMGUI_API glm::mat4	  ImGui_Impl_VR_GetGuiScale();
IMGUI_API void		  ImGui_Impl_VR_SetGuiScale(float s);
IMGUI_API void		  ImGui_Impl_VR_SetGuiRotation(int i, glm::mat4& R);
IMGUI_API glm::mat4	  ImGui_Impl_VR_GetGuiRotation(int i);
IMGUI_API void		  ImGui_Impl_VR_SetPinned(bool pin);
IMGUI_API void		  ImGui_Impl_VR_TriggerHapticPulse(vr::TrackedDeviceIndex_t device);

IMGUI_API ImVec2	  ImGui_Impl_VR_GetAxisPos(int i);
IMGUI_API ImVec2	  ImGui_Impl_VR_GetAxisDragDelta(int i);

IMGUI_API void		  ImGui_Impl_VR_GrabScreenshot();
IMGUI_API void		  ImGui_Impl_VR_ShowKeyboard();
IMGUI_API void		  ImGui_Impl_VR_HideKeyboard();

IMGUI_API bool        ImGui_Impl_VR_InputText(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags = 0, ImGuiTextEditCallback callback = NULL, void* user_data = NULL);

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_API void        ImGui_Impl_VR_InvalidateDeviceObjects();
IMGUI_API bool        ImGui_Impl_VR_CreateDeviceObjects();
