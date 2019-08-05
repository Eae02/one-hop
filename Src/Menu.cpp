#include "Menu.hpp"
#include "Common.hpp"
#include "Level.hpp"

namespace menu
{
	enum class Screen
	{
		Main,
		LevelSelect,
		Options
	};
	
	Screen currentScreen = Screen::Main;
	
	const Rectangle titleTexArea(193, 9, 312, 150);
	const Rectangle levelSelectTexArea(204, 166, 282, 50);
	
	const Rectangle uiPlayTexArea(0, 0, 68, 32);
	const Rectangle uiOptionsTexArea(0, 33, 120, 34);
	const Rectangle uiExitTexArea(0, 69, 64, 28);
	
	const Rectangle uiLevelTexArea(0, 102, 93, 29);
	
	const Rectangle starTexArea(87, 73, 16, 17);
	const Rectangle starNoneTexArea(112, 73, 16, 17);
	
	void UpdateAndDraw()
	{
		ClearColor(glm::vec4(0, 120 / 255.0f, 190 / 255.0f, 1));
		
		Texture2D& uiTex = GetAsset<Texture2D>("Tex/UI.png");
		auto FlipUISrcRect = [&] (const Rectangle& r) { return Rectangle(r.x, uiTex.Height() - r.y, r.w, -r.h); };
		
		gfx2d->Begin();
		
		const float midX = CurrentRTWidth() * 0.5f;
		constexpr float BTN_STEP_Y = 20;
		
		bool clicked = IsButtonDown(Button::MouseLeft) && !WasButtonDown(Button::MouseLeft);
		
		glm::vec2 cursorPos(CursorX(), CurrentRTHeight() - CursorY());
		glm::vec2 oldCursorPos(PrevCursorX(), CurrentRTHeight() - PrevCursorY());
		
		SoundEffectParams selectSFXParams;
		selectSFXParams.volume = 0.5f;
		SoundEffectParams hoverSFXParams;
		hoverSFXParams.volume = 0.2f;
		
		auto MaybePlaySounds = [&] (const Rectangle& r)
		{
			if (r.Contains(cursorPos))
			{
				if (clicked)
					PlaySoundEffect(GetAsset<AudioClip>("Audio/Select.ogg"), glm::vec2(), selectSFXParams);
				else if (!r.Contains(oldCursorPos))
					PlaySoundEffect(GetAsset<AudioClip>("Audio/Hover.ogg"), glm::vec2(), hoverSFXParams);
			}
		};
		
		float yPos;
		auto Button = [&](const Rectangle& r)
		{
			auto drawRect = Rectangle::CreateCentered(midX, yPos, r.w, r.h);
			bool hover = drawRect.Contains(cursorPos);
			if (hover)
				drawRect.x += 2;
			MaybePlaySounds(drawRect);
			gfx2d->Sprite(uiTex, drawRect, glm::vec4(1, 1, 1, hover ? 0.5f : 1.0f), FlipUISrcRect(r));
			yPos -= drawRect.h + BTN_STEP_Y;
			return hover && clicked;
		};
		
		if (currentScreen == Screen::Main)
		{
			yPos = CurrentRTHeight() * 0.5f + 130;
			
			auto titleRect = Rectangle::CreateCentered(midX, yPos, titleTexArea.w, titleTexArea.h);
			gfx2d->Sprite(uiTex, titleRect, glm::vec4(1), FlipUISrcRect(titleTexArea));
			yPos -= titleRect.h + 20;
			
			if (Button(uiPlayTexArea))
				currentScreen = Screen::LevelSelect;
			Button(uiOptionsTexArea);
			if (Button(uiExitTexArea))
				QuitGame();
		}
		else if (currentScreen == Screen::LevelSelect)
		{
			yPos = CurrentRTHeight() * 0.5f + 130;
			
			auto titleRect = Rectangle::CreateCentered(midX, yPos, levelSelectTexArea.w, levelSelectTexArea.h);
			gfx2d->Sprite(uiTex, titleRect, glm::vec4(1), FlipUISrcRect(levelSelectTexArea));
			yPos -= titleRect.h + 20;
			
			for (int i = 1; i <= level::numLevels; i++)
			{
				auto drawRect = Rectangle::CreateCentered(midX, yPos, uiLevelTexArea.w + 20, uiLevelTexArea.h);
				bool hover = drawRect.Contains(CursorX(), CurrentRTHeight() - CursorY());
				
				glm::vec4 color(1);
				if (i > level::numCompleted + 1)
				{
					color = glm::vec4(0.7f, 0.7f, 0.7f, 1);
					hover = false;
				}
				else if (hover)
				{
					color.a = 0.5f;
				}
				
				MaybePlaySounds(drawRect);
				
				if (hover)
					drawRect.x += 2;
				
				gfx2d->Sprite(uiTex, { drawRect.x, drawRect.y, uiLevelTexArea.w, uiLevelTexArea.h }, color, FlipUISrcRect(uiLevelTexArea));
				
				std::string numStr = std::to_string(i);
				glm::vec2 endPos = DrawUIText(numStr, glm::vec2(drawRect.x + uiLevelTexArea.w, drawRect.y), color, false);
				
				if (level::HasStar(i))
				{
					gfx2d->Sprite(uiTex, Rectangle::CreateCentered(endPos.x + 20, drawRect.CenterY(), starTexArea.w, starTexArea.h),
					              color, FlipUISrcRect(starTexArea));
				}
				
				yPos -= drawRect.h + BTN_STEP_Y;
				
				if (hover && clicked)
				{
					inGame = true;
					level::SetCurrent(i);
				}
			}
		}
		
		gfx2d->End();
	}
	
	void SetScreenMainMenu()
	{
		currentScreen = Screen::Main;
	}
}
