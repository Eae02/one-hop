#include <sstream>
#include "EndLevelMenu.hpp"
#include "Common.hpp"
#include "Menu.hpp"
#include "Level.hpp"

#include <iomanip>

const Rectangle levelClearTexArea(197, 211, 291, 55);

const Rectangle timeTexArea(299, 280, 93, 30);
const Rectangle bestTimeTexArea(219, 314, 173, 32);
const Rectangle bestTimeNATexArea(219, 314, 249, 32);
const Rectangle deathsTexArea(266, 350, 126, 32);

const Rectangle nextLevelTexArea(254, 384, 177, 33);
const Rectangle mainMenuTexArea(261, 417, 167, 32);

std::string FormatTime(float time)
{
	std::ostringstream stream;
	int min = (int)(time / 60);
	int sec = (int)std::round(time - (float)min * 60);
	stream << min << ":" << std::setfill('0') << std::setw(2) << sec;
	return stream.str();
}

void DrawAndUpdateEndLevelMenu(int deaths, float elapsedTime, float bestTime)
{
	Texture2D& uiTex = GetAsset<Texture2D>("Tex/UI.png");
	auto FlipUISrcRect = [&] (const Rectangle& r) { return Rectangle(r.x, uiTex.Height() - r.y, r.w, -r.h); };
	
	const float midX = CurrentRTWidth() * 0.5f;
	constexpr float BTN_STEP_Y = 20;
	
	bool clicked = IsButtonDown(Button::MouseLeft) && !WasButtonDown(Button::MouseLeft);
	
	float yPos = CurrentRTHeight() * 0.5f + 130;
	auto Button = [&](const Rectangle& r)
	{
		auto drawRect = Rectangle::CreateCentered(midX, yPos, r.w, r.h);
		bool hover = drawRect.Contains(CursorX(), CurrentRTHeight() - CursorY());
		if (hover)
			drawRect.x += 2;
		gfx2dOverlay->Sprite(uiTex, drawRect, glm::vec4(1, 1, 1, hover ? 0.5f : 1.0f), FlipUISrcRect(r));
		yPos -= drawRect.h + BTN_STEP_Y;
		return hover && clicked;
	};
	
	auto titleRect = Rectangle::CreateCentered(midX, yPos, levelClearTexArea.w, levelClearTexArea.h);
	gfx2dOverlay->Sprite(uiTex, titleRect, glm::vec4(1), FlipUISrcRect(levelClearTexArea));
	yPos -= titleRect.h + 30;
	
	
	//Time
	auto timeRect = Rectangle(midX - timeTexArea.w - 10, yPos, timeTexArea.w, timeTexArea.h);
	gfx2dOverlay->Sprite(uiTex, timeRect, glm::vec4(1), FlipUISrcRect(timeTexArea));
	std::string timeText = FormatTime(elapsedTime);
	DrawUIText(timeText, glm::vec2(midX, timeRect.y), glm::vec4(1), false, gfx2dOverlay);
	yPos -= timeRect.h + BTN_STEP_Y;
	
	
	//Best time
	auto _bestTimeTexArea = std::isnan(bestTime) ? bestTimeNATexArea : bestTimeTexArea;
	auto bestTimeRect = Rectangle(midX - bestTimeTexArea.w - 10, yPos, _bestTimeTexArea.w, _bestTimeTexArea.h);
	gfx2dOverlay->Sprite(uiTex, bestTimeRect, glm::vec4(1), FlipUISrcRect(_bestTimeTexArea));
	if (!std::isnan(bestTime))
	{
		std::string bestTimeText = FormatTime(bestTime);
		DrawUIText(bestTimeText, glm::vec2(midX, bestTimeRect.y), glm::vec4(1), false, gfx2dOverlay);
	}
	yPos -= _bestTimeTexArea.h + BTN_STEP_Y;
	
	
	//Deaths
	auto deathsRect = Rectangle(midX - deathsTexArea.w - 10, yPos, deathsTexArea.w, deathsTexArea.h);
	gfx2dOverlay->Sprite(uiTex, deathsRect, glm::vec4(1), FlipUISrcRect(deathsTexArea));
	std::string deathsText = std::to_string(deaths);
	DrawUIText(deathsText, glm::vec2(midX, deathsRect.y), glm::vec4(1), false, gfx2dOverlay);
	yPos -= deathsRect.h + BTN_STEP_Y;
	
	
	yPos -= 20;
	
	if (level::currentLevelIdx < level::numLevels && Button(nextLevelTexArea))
	{
		level::SetCurrent(level::currentLevelIdx + 1);
	}
	
	if (Button(mainMenuTexArea))
	{
		inGame = false;
		menu::SetScreenMainMenu();
	}
}
