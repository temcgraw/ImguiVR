# ImguiVR
Demo code for using Imgui with OpenVR

ImguiVR uses OpenVR, OpenGL and ImGui to implement 3D user interfaces for virtual reality applications. The current sample code demonstrates

A simple UI on a quad that is attached to the controller (but can be pinned in 3d space) and multiple tabs. The tabs are implemented by creating multiple ImGui contexts and rendering each to a separate texture.

A TiltBrush-like UI on a cube. The left controller rotates the cube, and the right controller is a pointer.

Features that are currently working: 
Haptic feedback: vibration when mousing over UI items 
InputText: summons a virtual keyboard (must be running SteamVR in the background) 
Scrolling: right controller emulates mouse scroll wheel 
Screenshots : get written to the SteamVR user data directory
