#include "Level.hpp"
#include "Player.hpp"
#include "EndLevelMenu.hpp"
#include "Menu.hpp"
#include "MovingPlatform.hpp"

#include <map>

const Rectangle readUITexPosition(0, 512 - 174, 137, -205);

TMXTerrain* level::terrain;

static std::unique_ptr<TileSolidityMap> solidity;
static std::unique_ptr<TileSolidityMap> hasKeySolidity;
std::unique_ptr<TileSolidityMap> level::platformSolidity;

bool levelHasStar = false;
glm::vec2 starPosition;

bool starPickedUp = false;
glm::vec2 currentStarPosition;

glm::vec2 level::spawnPosition;

glm::vec2 flagPosition;

bool hasKey = false;
glm::vec2 keyPosition;

bool keyPickedUp = false;
bool hasDoor = false;
glm::vec2 doorPosition;

struct LevelInfo
{
	const char* name;
};

const LevelInfo levelInfo[] = 
{
	{ "Levels/Level1.tmx" },
	{ "Levels/Level2.tmx" },
	{ "Levels/Level3.tmx" },
	{ "Levels/Level4.tmx" },
	{ "Levels/Level5.tmx" },
	{ "Levels/Level6.tmx" },
	{ "Levels/Level7.tmx" },
};

bool hasStar[std::size(levelInfo)];
float bestTime[std::size(levelInfo)];

int level::numLevels = std::size(levelInfo);
int level::numCompleted = 0;

static Rectangle limitedCameraBounds;
static Rectangle fullCameraBounds;
static bool fadeToFullCameraBounds = false;
static float fullCameraBoundsProgress = 0;

bool isLevelCompleted = false;
float timeSinceCompletion = 0;
float levelElapsedTime = 0;
int deaths = 0;

float resetTime = 0;
bool canStartReset = true;
constexpr float RESET_HOLD_TIME = 0.5f;

glm::vec2 cameraTargetVel;
glm::vec2 cameraTarget;

glm::vec2 readTextPos;
float readTextShowProgress = 0;
const Texture2D* currentSignTexture;

std::vector<std::pair<std::string, std::string>> signTexts = 
{
	{ "basic-move", "Tex/SignBasicControls.png" },
	{ "climb", "Tex/SignClimb.png" },
	{ "look-around", "Tex/SignLookAround.png" },
	{ "walljump", "Tex/SignWalljump.png" },
	{ "key", "Tex/SignKey.png" },
};

std::vector<std::pair<Rectangle, std::string_view>> signActivators;

std::vector<MovingPlatform> movingPlatforms;

int level::currentLevelIdx;

void level::InitSave()
{
	numCompleted = 0;
	std::fill(std::begin(hasStar), std::end(hasStar), false);
	std::fill(std::begin(bestTime), std::end(bestTime), NAN);
	
	try
	{
		level::numCompleted = std::stoi(save::GetValue("levelsCompleted", "0"));
		for (size_t i = 0; i < std::size(levelInfo); i++)
		{
			std::string hasStarName = "hasStar" + std::to_string(i + 1);
			hasStar[i] = save::GetValue(hasStarName, "F") == "T";
			
			std::string bestTimeName = "bestTime" + std::to_string(i + 1);
			if (auto bestTimeVal = save::GetValue(bestTimeName))
				bestTime[i] = std::stof(*bestTimeVal);
		}
	}
	catch (...) { }
}

bool level::HasStar(int level)
{
	return hasStar[level - 1];
}

void level::SetCurrent(int index)
{
	currentLevelIdx = index;
	index--;
	terrain = &GetAsset<TMXTerrain>(levelInfo[index].name);
	solidity = std::make_unique<TileSolidityMap>(terrain->MakeSolidityMap(5));
	platformSolidity = std::make_unique<TileSolidityMap>(terrain->MakeSolidityMap(2));
	hasKeySolidity = std::make_unique<TileSolidityMap>(terrain->MakeSolidityMap(1));
	
	signActivators.clear();
	for (std::pair<std::string, std::string>& sign : signTexts)
	{
		std::string fullName = "sign:" + sign.first;
		if (const TMXShape* signShape = terrain->GetShapeByName(fullName))
			signActivators.emplace_back(signShape->rect, sign.second);
	}
	
	movingPlatforms.clear();
	for (const TMXShape& platformShape : terrain->GetShapesByName("platform"))
	{
		MovingPlatform& platform = movingPlatforms.emplace_back();
		platform.start = platformShape.rect.Min();
		platform.end = platformShape.rect.Max();
		platform.waitTime = platformShape.properties.GetPropertyFloat("waitTime").value_or(1.0f);
		platform.moveTime = platformShape.properties.GetPropertyFloat("moveTime").value_or(1.0f);
		platform.timeOffset = platformShape.properties.GetPropertyFloat("timeOffset").value_or(0.0f);
		platform.Init();
	}
	
	flagPosition = terrain->GetShapeByName("flag")->rect.Min();
	
	hasKey = false;
	if (const TMXShape* key = terrain->GetShapeByName("key"))
	{
		keyPosition = key->rect.Min();
		hasKey = true;
	}
	
	keyPickedUp = false;
	hasDoor = false;
	if (const TMXShape* door = terrain->GetShapeByName("door"))
	{
		doorPosition = door->rect.Center();
		hasDoor = true;
	}
	
	if (const TMXShape* spawn = terrain->GetShapeByName("spawn"))
	{
		spawnPosition = spawn->rect.Center();
	}
	else
	{
		spawnPosition = { terrain->MapWidth() * terrain->TileWidth() * 0.5f, terrain->MapHeight() * terrain->TileHeight() };
	}
	
	levelHasStar = false;
	starPickedUp = false;
	if (const TMXShape* star = terrain->GetShapeByName("star"))
	{
		levelHasStar = true;
		currentStarPosition = starPosition = star->rect.Center();
	}
	
	fullCameraBounds = Rectangle(0, 0, terrain->MapWidth() * terrain->TileWidth(), terrain->MapHeight() * terrain->TileHeight()).Inflated(-4);
	
	limitedCameraBounds = fullCameraBounds;
	if (const TMXShape* boundsInitial = terrain->GetShapeByName("cameraBoundsInitial"))
	{
		limitedCameraBounds = boundsInitial->rect;
	}
	
	camera.bounds = limitedCameraBounds;
	fadeToFullCameraBounds = false;
	fullCameraBoundsProgress = 0;
	
	isLevelCompleted = false;
	timeSinceCompletion = 0;
	levelElapsedTime = 0;
	currentSignTexture = nullptr;
	readTextShowProgress = 0;
	cameraTarget = spawnPosition;
	
	deaths = 0;
	
	player.Reset();
}

constexpr float COMPLETE_MENU_DELAY = 0.5f;

void level::Update(float dt)
{
	if (!isLevelCompleted)
	{
		levelElapsedTime += dt;
		
		if (!keyPickedUp && glm::distance(player.Rect().Center(), keyPosition) < 25.0f)
		{
			keyPickedUp = true;
			PlaySoundEffect(GetAsset<AudioClip>("Audio/Key.ogg"), player.Position());
		}
		
		if (TMXLayer* doorLayer = terrain->GetLayerByName("Door"))
			doorLayer->visible = !keyPickedUp;
		
		if (!starPickedUp && glm::distance(player.Rect().Center(), starPosition) < 25.0f)
		{
			starPickedUp = true;
		}
		
		if (Rectangle(flagPosition, 27, 34).Intersects(player.HitBox()))
		{
			isLevelCompleted = true;
			timeSinceCompletion = 0;
			player.Stop();
			
			auto emitter = std::make_shared<ParticleEmitter>(GetAsset<ParticleEmitterType>("LevelEnd.ype"));
			emitter->position = flagPosition;
			particleMan->AddEmitter(emitter);
			particleMan->KeepAlive(std::move(emitter), 0.1f);
			
			if (currentLevelIdx > numCompleted)
			{
				numCompleted = currentLevelIdx;
				save::SetValue("levelsCompleted", std::to_string(numCompleted));
			}
			if (!hasStar[currentLevelIdx - 1] && starPickedUp)
			{
				hasStar[currentLevelIdx - 1] = true;
				std::string hasStarName = "hasStar" + std::to_string(currentLevelIdx);
				save::SetValue(hasStarName, "T");
			}
			if (std::isnan(bestTime[currentLevelIdx - 1]) || levelElapsedTime < bestTime[currentLevelIdx - 1])
			{
				bestTime[currentLevelIdx - 1] = levelElapsedTime;
				std::string bestTimeName = "bestTime" + std::to_string(currentLevelIdx);
				save::SetValue(bestTimeName, std::to_string(levelElapsedTime));
			}
		}
		
		if (IsButtonDown(Button::R))
		{
			if (canStartReset)
			{
				resetTime += dt;
				if (!WasButtonDown(Button::R))
					camera.Shake(3.0f, RESET_HOLD_TIME);
				if (resetTime > RESET_HOLD_TIME)
				{
					player.Reset();
					level::OnPlayerDead();
					resetTime = 0;
					canStartReset = false;
				}
			}
		}
		else
		{
			canStartReset = true;
			resetTime = 0;
		}
	}
	else
	{
		timeSinceCompletion += dt;
		currentSignTexture = nullptr;
	}
	
	if (IsButtonDown(Button::Escape) && !WasButtonDown(Button::Escape))
	{
		if (currentSignTexture != nullptr)
		{
			currentSignTexture = nullptr;
		}
		else
		{
			inGame = false;
			menu::SetScreenMainMenu();
		}
	}
	
	for (MovingPlatform& platform : movingPlatforms)
	{
		platform.Update(dt);
	}
	
	bool isLookingAround = !isLevelCompleted && IsButtonDown(Button::Z);
	
	player.Update(dt, !isLevelCompleted && currentSignTexture == nullptr && !isLookingAround);
	
	if (isLookingAround)
	{
		glm::vec2 targetAccel;
		if (IsButtonDown(Button::W))
			targetAccel.y += 1;
		else if (IsButtonDown(Button::S))
			targetAccel.y -= 1;
		else
			cameraTargetVel.y -= cameraTargetVel.y * std::min(100 * dt, 1.0f);
		
		if (IsButtonDown(Button::A))
			targetAccel.x -= 1;
		else if (IsButtonDown(Button::D))
			targetAccel.x += 1;
		else
			cameraTargetVel.x -= cameraTargetVel.x * std::min(100 * dt, 1.0f);
		
		targetAccel *= 500;
		cameraTargetVel += targetAccel * dt;
		
		cameraTarget = camera.ConstrainTargetToBounds(cameraTarget, GetZoomLevel());
		cameraTarget += cameraTargetVel * dt;
	}
	else
	{
		cameraTargetVel = glm::vec2(0.0f);
		cameraTarget += (player.Position() - cameraTarget) * std::min(dt * 15, 1.0f);
	}
	
	constexpr float READ_SHOW_ANIM_TIME = 0.25f;
	bool anySign = false;
	if (!isLevelCompleted && currentSignTexture == nullptr)
	{
		for (const auto& sign : signActivators)
		{
			if (sign.first.Intersects(player.HitBox()))
			{
				readTextPos = sign.first.Center() - glm::vec2(0, 5);
				readTextShowProgress = std::min(readTextShowProgress + dt / READ_SHOW_ANIM_TIME, 1.0f);
				anySign = true;
				
				if (IsButtonDown(Button::E) && !WasButtonDown(Button::E))
				{
					currentSignTexture = &GetAsset<Texture2D>(sign.second);
				}
				
				break;
			}
		}
	}
	if (!anySign)
	{
		readTextShowProgress = std::max(readTextShowProgress - dt / READ_SHOW_ANIM_TIME, 0.0f);
	}
	
	glm::vec2 starTargetPos = starPosition;
	if (starPickedUp)
	{
		starTargetPos = player.Rect().Center() + glm::vec2(0, Player::HEIGHT * 0.5f + 10);
	}
	currentStarPosition += (starTargetPos - currentStarPosition) * std::min(dt * 12.0f, 1.0f);
	
	if (!limitedCameraBounds.Inflated(8).Contains(player.Rect()) && !fadeToFullCameraBounds)
	{
		fadeToFullCameraBounds = true;
		fullCameraBoundsProgress = 0;
	}
	
	if (fadeToFullCameraBounds && fullCameraBoundsProgress < 1)
	{
		const float a = glm::smoothstep(0.0f, 1.0f, fullCameraBoundsProgress);
		camera.bounds.x = glm::mix(limitedCameraBounds.x, fullCameraBounds.x, a);
		camera.bounds.y = glm::mix(limitedCameraBounds.y, fullCameraBounds.y, a);
		camera.bounds.w = glm::mix(limitedCameraBounds.w, fullCameraBounds.w, a);
		camera.bounds.h = glm::mix(limitedCameraBounds.h, fullCameraBounds.h, a);
		fullCameraBoundsProgress = std::min(fullCameraBoundsProgress + dt * 3.0f, 1.0f);
	}
	
	particleMan->Update(dt);
}

void level::Draw()
{
	gfx2dBackground->Begin();
	gfx2d->Begin();
	gfx2dOverlay->Begin();
	
	gfx2dBackground->Sprite(GetAsset<Texture2D>("Tex/SkyGradient.png"), fullCameraBounds, glm::vec4(1));
	
	if (levelHasStar)
	{
		constexpr int STAR_FRAME_WIDTH = 15;
		constexpr int STAR_FRAMES = 9;
		constexpr float TIME_PER_STAR_FRAME = 0.15f;
		int starFrame = (int)(animationTime / TIME_PER_STAR_FRAME) % STAR_FRAMES;
		
		auto& starTex = GetAsset<Texture2D>("Tex/Star.png");
		gfx2d->Sprite(starTex,
		              Rectangle::CreateCentered(currentStarPosition, STAR_FRAME_WIDTH, starTex.Height()), glm::vec4(1),
		              Rectangle(STAR_FRAME_WIDTH * starFrame, 0, STAR_FRAME_WIDTH, starTex.Height()), SpriteFlags::FlipY);
	}
	
	if (hasKey && !keyPickedUp)
	{
		auto& keyTex = GetAsset<Texture2D>("Tex/Key.png");
		gfx2d->Sprite(keyTex, Rectangle::CreateCentered(keyPosition, keyTex.Width(), keyTex.Height()), glm::vec4(1), SpriteFlags::FlipY);
	}
	
	if (hasDoor)
	{
		auto& doorTex = GetAsset<Texture2D>("Tex/Door.png");
		gfx2d->Sprite(doorTex, Rectangle(doorPosition.x - doorTex.Width() / 2, doorPosition.y, doorTex.Width(), doorTex.Height()), glm::vec4(1), SpriteFlags::FlipY);
	}
	
	auto& flagTex = GetAsset<Texture2D>("Tex/Flag.png");
	constexpr int FLAG_FRAME_WIDTH = 27;
	constexpr int FLAG_FRAMES = 4;
	constexpr float TIME_PER_FLAG_FRAME = 0.3f;
	int flagFrame = (int)(animationTime / TIME_PER_FLAG_FRAME) % FLAG_FRAMES;
	
	gfx2d->Sprite(flagTex, flagPosition, glm::vec4(1), Rectangle(flagFrame * FLAG_FRAME_WIDTH, 0, FLAG_FRAME_WIDTH, flagTex.Height()));
	
	for (MovingPlatform& platform : movingPlatforms)
	{
		platform.Draw();
	}
	
	player.Draw();
	
	float zoomLevel = GetZoomLevel();
	
	auto [viewMatrix, viewMatrixInv] = camera.ViewMatrixAndInverse(cameraTarget, zoomLevel);
	
	gfx2dBackground->End(camera.ViewMatrix(cameraTarget * 0.75f, zoomLevel));
	
	level::terrain->Draw(viewMatrix);
	
	if (readTextShowProgress > 0)
	{
		constexpr float READ_SHOW_ANIM_DY = 5.0f;
		glm::vec2 center = readTextPos - glm::vec2(0, (1 - readTextShowProgress) * READ_SHOW_ANIM_DY);
		gfx2d->Sprite(GetAsset<Texture2D>("Tex/UI.png"), Rectangle::CreateCentered(center, readUITexPosition.w / 4, readUITexPosition.h / -4),
			glm::vec4(1, 1, 1, readTextShowProgress * 0.8f), readUITexPosition);
	}
	
	if (currentSignTexture != nullptr)
	{
		gfx2dOverlay->Sprite(*currentSignTexture,
			Rectangle::CreateCentered(CurrentRTWidth() / 2, CurrentRTHeight() * 0.7f, currentSignTexture->Width(), currentSignTexture->Height()),
			glm::vec4(1), SpriteFlags::FlipY);
	}
	
	if (isLevelCompleted && timeSinceCompletion > COMPLETE_MENU_DELAY)
	{
		float opacity = std::min((timeSinceCompletion - COMPLETE_MENU_DELAY) * 4.0f, 1.0f);
		gfx2dOverlay->FilledRect(Rectangle(0, 0, CurrentRTWidth(), CurrentRTHeight()), glm::vec4(0, 0, 0, opacity * 0.3f));
		
		DrawAndUpdateEndLevelMenu(deaths, levelElapsedTime, bestTime[currentLevelIdx - 1]);
	}
	
	if (!isLevelCompleted && resetTime > 0)
	{
		gfx2dOverlay->FilledRect(Rectangle(0, 0, CurrentRTWidth(), CurrentRTHeight()), glm::vec4(0, 0, 0, 0.5f * resetTime / RESET_HOLD_TIME));
	}
	
	particleMan->Draw(viewMatrix);
	
	gfx2d->End(viewMatrix);
	
	gfx2dOverlay->End();
}

float level::GetZoomLevel()
{
	float zoomLevel = std::round(CurrentRTHeight() / 250.0f);
	return zoomLevel;
}

TileSolidityMap& level::SolidityMap()
{
	if (keyPickedUp)
		return *hasKeySolidity;
	return *solidity;
}

void level::OnPlayerDead()
{
	PlaySoundEffect(GetAsset<AudioClip>("Audio/Hurt.ogg"), player.Position());
	deaths++;
	keyPickedUp = false;
	starPickedUp = false;
	cameraTarget = player.Position();
}

glm::vec2 level::ClipPlatforms(const Rectangle& origin, glm::vec2 move, bool& clippedX, bool& clippedY)
{
	for (const MovingPlatform& platform : movingPlatforms)
	{
		glm::vec2 platformMove = platform.GetPosition() - platform.GetOldPosition();
		
		auto [playerToPlatCY, playerToPlatMY] = Rectangle::ClipY(origin, platform.GetOldRectangle(), move.y);
		if (playerToPlatCY)
		{
			clippedY = true;
			move.y = playerToPlatMY;
			move += platformMove;
		}
		else
		{
			auto [playerToPlatCY2, playerToPlatMY2] = Rectangle::ClipY(origin, platform.GetRectangle(), move.y);
			if (playerToPlatCY2)
			{
				clippedY = true;
				move.y = playerToPlatMY2;
			}
		}
	}
	
	return move;
}

MovingPlatform* level::IntersectMovingPlatform(const Rectangle& rectangle)
{
	for (MovingPlatform& platform : movingPlatforms)
	{
		if (platform.GetRectangle().Intersects(rectangle))
			return &platform;
	}
	return nullptr;
}

bool level::IntersectsSolid(const Rectangle& rectangle)
{
	return level::SolidityMap().IntersectsSolid(rectangle) || IntersectMovingPlatform(rectangle) != nullptr;
}
