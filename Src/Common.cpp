#include "Common.hpp"

#include <map>

Camera2D camera;

Graphics2D* gfx2d;
Graphics2D* gfx2dBackground;
Graphics2D* gfx2dOverlay;

ParticleManager* particleMan;

float animationTime = 0;

bool inGame = false;

std::map<char, Rectangle> letterRectangles = 
{
	{ ':', { 144, 68, 10, 29 } },
	{ '0', { 119, 68, 19, 29 } },
	{ '1', { 94, 102, 16, 29 } },
	{ '2', { 118, 102, 19, 29 } },
	{ '3', { 146, 102, 19, 29 } },
	{ '4', { 0, 137, 21, 27 } },
	{ '5', { 28, 137, 26, 29 } },
	{ '6', { 60, 137, 23, 29 } },
	{ '7', { 89, 137, 21, 29 } },
	{ '8', { 118, 137, 20, 29 } },
	{ '9', { 146, 137, 20, 29 } },
};

glm::vec2 DrawUIText(std::string_view text, glm::vec2 pos, const glm::vec4& color, bool centerY, Graphics2D* gfx)
{
	if (!gfx)
		gfx = gfx2d;
	auto& uiTex = GetAsset<Texture2D>("Tex/UI.png");
	
	auto FlipUISrcRect = [&] (const Rectangle& r) { return Rectangle(r.x, uiTex.Height() - r.y, r.w, -r.h); };
	
	for (char c : text)
	{
		if (c == ' ')
		{
			pos.x += 5;
			continue;
		}
		
		auto it = letterRectangles.find(c);
		if (it != letterRectangles.end())
		{
			glm::vec2 actualPos = pos;
			if (centerY)
				actualPos.y -= it->second.h / 2;
			gfx->Sprite(uiTex, { actualPos.x, actualPos.y, it->second.w, it->second.h }, color, FlipUISrcRect(it->second));
			pos.x += it->second.w;
		}
	}
	
	return pos;
}
