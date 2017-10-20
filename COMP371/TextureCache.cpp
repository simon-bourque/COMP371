#include "TextureCache.h"

#include "Texture.h"

#define STB_IMAGE_STATIC
#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include "Types.h"

TextureCache::TextureCache() {}


TextureCache::~TextureCache() {
	for (const auto& pair : m_textures) {
		glDeleteTextures(1, &pair.second->m_texture);
		delete pair.second;
	}
	m_textures.clear();
}

Texture* TextureCache::loadTexture2D(const std::string& name, const std::string& imgPath) {
	// Load image
	int32 width = 0;
	int32 height = 0;
	int32 channels = 0;
	unsigned char* data = stbi_load(imgPath.c_str(), &width, &height, &channels, 4);

	if (!data) {
		const char* reason = stbi_failure_reason();
		throw std::runtime_error("Failed to load image \'" + imgPath + "\': " + reason);
	}

	GLuint texture = 0;
	glGenTextures(1, &texture);

	glBindTexture(GL_TEXTURE_2D, texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_2D, 0);

	stbi_image_free(data);

	m_textures[name] = new Texture(texture, Texture::Type::TEXTURE_2D);
	return m_textures[name];
}