//Edited by T. McGraw (tmcgraw@purdue.edu)
//This class was stripped from openvr sample hellovr_opengl_main.cpp.

#ifndef __CGLRENDERMODEL_H__
#define __CGLRENDERMODEL_H__

#include <string>
#include <GL/glew.h>
#include <openvr.h>

class CGLRenderModel
{
public:
	CGLRenderModel( const std::string & sRenderModelName );
	~CGLRenderModel();

	bool BInit( const vr::RenderModel_t & vrModel, const vr::RenderModel_TextureMap_t & vrDiffuseTexture );
	void Cleanup();
	void Draw();
	const std::string & GetName() const { return m_sModelName; }

private:
	GLuint m_glVertBuffer;
	GLuint m_glIndexBuffer;
	GLuint m_glVertArray;
	GLuint m_glTexture;
	GLsizei m_unVertexCount;
	std::string m_sModelName;
};

#endif