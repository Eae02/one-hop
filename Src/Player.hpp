#pragma once

#include "Common.hpp"

class Player
{
public:
	Player();
	
	void Update(float dt, bool controllable);
	
	Rectangle Rect() const;
	
	Rectangle HitBox() const;
	
	const glm::vec2& Position() const
	{
		return m_position;
	}
	
	void SetPosition(const glm::vec2& position)
	{
		m_position = position;
	}
	
	static constexpr float HEIGHT = 35;
	static constexpr float WIDTH = 14;
	
	void Draw();
	
	void Reset();
	
	void Stop();
	
private:
	void UpdateNormal(float dt);
	void UpdateClimb(float dt);
	
	Rectangle ClimbLeftRectangle(float w = 0, float minYPC = 0.1f, float maxYPC = 0.9f) const;
	Rectangle ClimbRightRectangle(float w = 0, float minYPC = 0.1f, float maxYPC = 0.9f) const;
	
	bool MaybeJump();
	
	std::weak_ptr<ParticleEmitter> m_jumpEmitter;
	std::shared_ptr<ParticleEmitter> m_dirtEmitter;
	
	float m_walkSfxPlayTime = 0;
	
	bool m_noClip = false;
	bool m_hasJumped = false;
	
	int m_climbHoldDirection = 0;
	float m_climbAnimationTime = 0;
	bool m_inFinalClimb = false;
	float m_finalClimbMoveUp;
	float m_finalClimbTime;
	bool m_useClimbSprite = false;
	class MovingPlatform* m_climbedPlatform = nullptr;
	
	glm::vec2 m_additionalMove;
	
	glm::vec2 m_position;
	glm::vec2 m_velocity;
	bool m_onGround = false;
	
	bool m_useJumpSprite = false;
	bool m_useRunAnimation = false;
	float m_timeSinceLeftGround = 0;
	float m_jumpAnimationTime;
	
	bool m_facingLeft = false;
	
	float m_animationTime = 0;
};

extern Player player;
