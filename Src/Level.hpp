#pragma once

#include "MovingPlatform.hpp"

namespace level
{
	extern TMXTerrain* terrain;
	extern std::unique_ptr<TileSolidityMap> platformSolidity;
	
	extern int numLevels;
	extern int numCompleted;
	extern int currentLevelIdx;
	
	void InitSave();
	
	bool HasStar(int level);
	
	void OnPlayerDead();
	
	glm::vec2 ClipPlatforms(const Rectangle& origin, glm::vec2 move, bool& clippedX, bool& clippedY);
	
	TileSolidityMap& SolidityMap();
	
	extern glm::vec2 spawnPosition;
	
	void SetCurrent(int index);
	
	void Update(float dt);
	
	bool IntersectsSolid(const Rectangle& rectangle);
	
	MovingPlatform* IntersectMovingPlatform(const Rectangle& rectangle);
	
	void Draw();
	
	float GetZoomLevel();
}
