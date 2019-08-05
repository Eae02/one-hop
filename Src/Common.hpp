#pragma once

extern Camera2D camera;

extern Graphics2D* gfx2dBackground;
extern Graphics2D* gfx2d;
extern Graphics2D* gfx2dOverlay;

extern ParticleManager* particleMan;

extern float animationTime;

extern bool inGame;

glm::vec2 DrawUIText(std::string_view text, glm::vec2 pos, const glm::vec4& color, bool centerY, Graphics2D* gfx = nullptr);
