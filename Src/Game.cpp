#include "Level.hpp"
#include "Player.hpp"
#include "Common.hpp"
#include "Menu.hpp"

struct GMTK19Game : Game
{
	GMTK19Game()
	{
		SetInputBinding("act", { Button::E, Button::CtrlrX });
		SetInputBinding("jump", { Button::Space, Button::CtrlrA });
		SetInputBinding("climb", { Button::LeftShift, Button::RightShift, Button::CtrlrLeftShoulder, Button::CtrlrRightShoulder });
		SetInputBinding("restart", { Button::R, Button::CtrlrY });
		SetInputBinding("esc", { Button::Escape, Button::CtrlrStart });
		SetInputBinding("pan", { Button::Z, Button::CtrlrB });
		
		save::Init("eae", "onehop");
		level::InitSave();
		
		gfx2d = new Graphics2D();
		gfx2dOverlay = new Graphics2D();
		particleMan = new ParticleManager();
		gfx2dOverlay->roundCoordinates = true;
		gfx2dBackground = new Graphics2D();
		camera.rotationShakeMagnitude *= 0.35f;
		camera.constrainToBounds = true;
		
		InitAssetCallback<Texture2D>("Tex/Star.png", [](Texture2D& tex)
		{
			tex.SetWrapU(TextureWrapMode::ClampToEdge);
			tex.SetWrapV(TextureWrapMode::ClampToEdge);
		});
		
		InitAssetCallback<Texture2D>("Tex/SkyGradient.png", [](Texture2D& tex)
		{
			tex.SetWrapU(TextureWrapMode::ClampToEdge);
			tex.SetWrapV(TextureWrapMode::ClampToEdge);
			tex.SetMagFilter(Filter::Linear);
		});
	}
	
	~GMTK19Game()
	{
		delete gfx2d;
		delete gfx2dBackground;
		delete gfx2dOverlay;
		delete particleMan;
	}
	
	virtual void RunFrame(float dt) override
	{
		if (!inGame)
		{
			jm::UpdateListener(glm::vec2(0));
			
			menu::UpdateAndDraw();
		}
		else
		{
			animationTime += dt;
			
			level::Update(dt);
			
			level::Draw();
		}
	}
};

JM_ENTRY_POINT(GMTK19Game)
