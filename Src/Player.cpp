#include "Player.hpp"
#include "Level.hpp"
#include "Common.hpp"

constexpr float HIT_BOX_MARGIN_X = 0.3f;

//Constants related to player movement
constexpr float WALK_SPEED           = 100;   //Walking speed in pixels/second
constexpr float ACCEL_TIME           = 0.1f;   //The number of seconds it takes to accelerate to maximum speed
constexpr float DEACCEL_TIME         = 0.05f;   //The number of seconds it takes to stop after moving at maximum speed
constexpr float GRAVITY              = 500.0f;  //Acceleration due to gravity in pixels/seconds^2
constexpr float JUMP_HEIGHT          = 60;   //Jump height in tiles
constexpr float FALLING_GRAVITY_RAMP = 0.1f;   //Percentage to increase gravity by while falling
constexpr float MAX_VERTICAL_SPEED   = 1000.0f; //Maximum vertical speed in pixels/second

//Constants which derive from the previous values, don't edit directly
static const float JUMP_ACCEL = std::sqrt(2.0f * JUMP_HEIGHT * GRAVITY);
constexpr    float ACCEL_AMOUNT = WALK_SPEED / ACCEL_TIME;
constexpr    float DEACCEL_AMOUNT = WALK_SPEED / DEACCEL_TIME;

constexpr float CLIMB_ATTACH_DIST_X = 5;

constexpr int JUMP_INIT_FRAMES = 2;
constexpr int JUMP_LOOP_FRAMES = 4;
constexpr float JUMP_TIME_PER_FRAME = 0.2f;

Player player;

Player::Player()
{
	m_position = glm::vec2(300, 100);
}

void Player::Update(float dt, bool controllable)
{
	if (m_dirtEmitter == nullptr)
	{
		m_dirtEmitter = std::make_shared<ParticleEmitter>(GetAsset<ParticleEmitterType>("PlayerMove.ype"));
		m_dirtEmitter->enabled = false;
		particleMan->AddEmitter(m_dirtEmitter);
	}
	
#ifndef NDEBUG
	if (IsButtonDown(Button::F1))
		m_hasJumped = false;
#endif
	
	m_useClimbSprite = false;
	m_noClip = false;
	m_additionalMove = glm::vec2(0);
	
	if (controllable)
	{
		if (m_climbHoldDirection == 0)
		{
			UpdateNormal(dt);
		}
		
		if (m_climbHoldDirection != 0)
		{
			UpdateClimb(dt);
		}
	}
	
	glm::vec2 move = m_velocity * dt + m_additionalMove;
	
	if (std::abs(m_velocity.x) < 10)
	{
		constexpr float LEDGE_FALL_W = 0.25f * WIDTH;
		if (m_onGround &&
			!level::IntersectsSolid(Rectangle(m_position.x, m_position.y - 5, WIDTH - LEDGE_FALL_W, 5)))
			move.x -= LEDGE_FALL_W;
		if (m_onGround && !level::IntersectsSolid(Rectangle(m_position.x + LEDGE_FALL_W, m_position.y - 5, WIDTH - LEDGE_FALL_W, 5)))
			move.x += LEDGE_FALL_W;
	}
	
	m_onGround = false;
	
	bool xClipped = false;
	bool yClipped = false;
	bool platXClipped = false;
	bool platYClipped = false;
	
	if (!m_noClip)
	{
		move = level::ClipPlatforms(HitBox(), move, platXClipped, platYClipped);
		
		move = level::SolidityMap().Clip(HitBox(), move, xClipped, yClipped);
		xClipped |= platXClipped;
		yClipped |= platYClipped;
		
		if (move.y < 0)
		{
			auto [platformClippedY, platformMove] = level::platformSolidity->ClipY(HitBox(), move.y);
			if (platformClippedY)
			{
				yClipped = true;
				move.y = platformMove;
			}
		}
	}
	
	if (yClipped)
	{
		if (m_velocity.y < 0)
		{
			if (m_velocity.y < -50)
			{
				SoundEffectParams sfxParams;
				sfxParams.pitch = 0.9f;
				sfxParams.volume = 2.0f;
				PlaySoundEffect(GetAsset<AudioClip>("Audio/Walk.ogg"), player.Position(), sfxParams);
			}
			if (m_velocity.y < -300)
			{
				camera.Shake(0.5f, 0.5f);
			}
			m_onGround = true;
		}
		m_velocity.y = 0;
	}
	
	if (xClipped)
	{
		m_velocity.x = 0;
	}
	
	constexpr float MIN_RUN_SPEED = 10;
	m_useRunAnimation = std::abs(m_velocity.x) > MIN_RUN_SPEED;
	
	m_dirtEmitter->enabled = m_climbHoldDirection == 0 && m_useRunAnimation && m_onGround;
	
	m_position += move;
	
	if (m_climbHoldDirection == 0 && m_onGround && std::abs(m_velocity.x) > 10)
	{
		m_walkSfxPlayTime -= dt;
		if (m_walkSfxPlayTime < 0)
		{
			SoundEffectParams sfxParams;
			sfxParams.pitch = RandomFloat(0.9f, 1.1f);
			sfxParams.volume = 1.2f;
			PlaySoundEffect(GetAsset<AudioClip>("Audio/Walk.ogg"), player.Position(), sfxParams);
			m_walkSfxPlayTime = 0.25f;
		}
	}
	
	if (controllable)
	{
		//Checks for spikes
		bool dead = false;
		for (const TMXShape& spikeRect : level::terrain->GetShapesByName("spikes"))
		{
			if (spikeRect.rect.Intersects(HitBox()))
			{
				dead = true;
			}
		}
		
		if (Rect().MaxY() < 0)
			dead = true;
		
		if (dead)
		{
			Reset();
			level::OnPlayerDead();
		}
	}
	
	if (auto jumpEmitter = m_jumpEmitter.lock())
		jumpEmitter->position = m_position + glm::vec2(WIDTH / 2, 5);
	m_dirtEmitter->position = m_position + glm::vec2(WIDTH / 2, 0);
	
	m_animationTime += dt;
	if (m_useJumpSprite)
		m_jumpAnimationTime += dt;
	
	jm::UpdateListener(m_position);
}

void Player::UpdateNormal(float dt)
{
	const float moveLR = AxisValueLR();
	
	float accelX = 0;
	
	if (std::abs(moveLR) < 0.01f)
	{
		if (m_velocity.x < 0)
		{
			m_velocity.x += dt * DEACCEL_AMOUNT;
			if (m_velocity.x > 0)
				m_velocity.x = 0.0f;
		}
		if (m_velocity.x > 0)
		{
			m_velocity.x -= dt * DEACCEL_AMOUNT;
			if (m_velocity.x < 0)
				m_velocity.x = 0.0f;
		}
	}
	else
	{
		accelX += moveLR * ACCEL_AMOUNT;
		m_facingLeft = moveLR < 0;
	}
	
	if ((accelX < 0) != (m_velocity.x < 0))
		accelX *= 4.0f;
	
	m_velocity.x += accelX * dt * (m_onGround ? 1.0f : 0.8f);
	
	float speedLim = m_onGround ? WALK_SPEED : WALK_SPEED * 1.1f;
	float speedX = std::abs(m_velocity.x);
	if (speedX > speedLim)
	{
		m_velocity.x *= speedLim / speedX;
	}
	
	//Updates vertical velocity
	if (IsBindingDownNow("jump") && m_onGround)
	{
		if (MaybeJump())
		{
			m_onGround = false;
			m_velocity.y = JUMP_ACCEL;
		}
	}
	else
	{
		float gravity = GRAVITY;
		gravity *= 1.0f + FALLING_GRAVITY_RAMP * glm::clamp(-m_velocity.y, 0.0f, 1.0f);
		
		m_velocity.y = std::max(m_velocity.y - gravity * dt, -MAX_VERTICAL_SPEED);
	}
	
	if (!m_onGround)
	{
		m_timeSinceLeftGround += dt;
		if (m_timeSinceLeftGround > 0.05f && !m_useJumpSprite)
		{
			m_useJumpSprite = true;
			m_jumpAnimationTime = (JUMP_INIT_FRAMES + 0.1f) * JUMP_TIME_PER_FRAME;
		}
	}
	else
	{
		m_useJumpSprite = false;
		m_timeSinceLeftGround = 0;
	}
	
	//Starts climbing
	if (IsBindingDown("climb") && !level::IntersectsSolid(Rect().Inflated(-2)))
	{
		if (level::IntersectsSolid(ClimbLeftRectangle(0, 0.3f, 0.5f)))
		{
			m_climbHoldDirection = -1;
		}
		else if (level::IntersectsSolid(ClimbRightRectangle(0, 0.3f, 0.5f)))
		{
			m_climbHoldDirection = 1;
		}
		
		if ((m_climbedPlatform = level::IntersectMovingPlatform(ClimbLeftRectangle(0, 0.1f, 0.9f))))
		{
			m_climbHoldDirection = -1;
		}
		else if ((m_climbedPlatform = level::IntersectMovingPlatform(ClimbRightRectangle(0, 0.1f, 0.9f))))
		{
			m_climbHoldDirection = 1;
		}
		
		if (m_climbedPlatform != nullptr)
		{
			m_facingLeft = m_climbHoldDirection == -1;
			m_velocity = glm::vec2(0);
			m_inFinalClimb = false;
			m_position = glm::vec2(m_climbedPlatform->GetPosition().x - m_climbHoldDirection * 40, m_climbedPlatform->GetPosition().y - HEIGHT * 0.75f);
			if (m_climbHoldDirection == 1)
				m_position.x -= WIDTH;
		}
		else if (m_climbHoldDirection != 0)
		{
			constexpr float CLIP_CHECK_MARGIN = 5;
			
			auto[clipped, clipDist] = level::SolidityMap().ClipX(
				Rectangle(m_position.x + CLIP_CHECK_MARGIN, m_position.y + HEIGHT * 0.1f,
				          WIDTH - CLIP_CHECK_MARGIN * 2, HEIGHT * 0.8f),
				(float)m_climbHoldDirection * 10);
			
			if (clipped)
			{
				m_facingLeft = m_climbHoldDirection == -1;
				m_velocity = glm::vec2(0);
				m_position.x += clipDist - (CLIP_CHECK_MARGIN + 3) * (float)m_climbHoldDirection;
				m_inFinalClimb = false;
			}
			else
			{
				m_climbHoldDirection = 0;
			}
		}
	}
}

bool Player::MaybeJump()
{
	if (m_hasJumped)
	{
		camera.Shake(1.0f, 0.4f);
		PlaySoundEffect(GetAsset<AudioClip>("Audio/Hurt.ogg"), m_position);
		return false;
	}
	
	SoundEffectParams sfxParams;
	sfxParams.volume = 0.8f;
	PlaySoundEffect(GetAsset<AudioClip>("Audio/Jump1.ogg"), m_position);
	
	m_hasJumped = true;
	m_useJumpSprite = true;
	m_jumpAnimationTime = 0;
	
	auto emitter = std::make_shared<ParticleEmitter>(GetAsset<ParticleEmitterType>("PlayerJump.ype"));
	m_jumpEmitter = emitter;
	particleMan->AddEmitter(emitter);
	particleMan->KeepAlive(std::move(emitter), 0.5f);
	
	return true;
}

void Player::UpdateClimb(float dt)
{
	constexpr float CLIMB_DEACCEL_TIME = 0.05f;
	constexpr float CLIMB_ACCEL_TIME = 0.05f;
	constexpr float CLIMB_SPEED = 75;
	constexpr float CLIMB_ACCEL_AMOUNT = CLIMB_SPEED / CLIMB_ACCEL_TIME;
	constexpr float CLIMB_DEACCEL_AMOUNT = CLIMB_SPEED / CLIMB_DEACCEL_TIME;
	
	constexpr float CLIMB_LAST_PERCENT = 0.3f;
	constexpr float FINAL_CLIMB_X_DURATION = 0.15f;
	
	m_useClimbSprite = true;
	m_useJumpSprite = false;
	
	if (m_inFinalClimb)
	{
		m_additionalMove += glm::vec2(0, std::min(dt * CLIMB_SPEED, std::max(m_finalClimbMoveUp, 0.0f)));
		m_finalClimbMoveUp -= dt * CLIMB_SPEED;
		if (m_finalClimbMoveUp < 0)
		{
			m_additionalMove.x += m_climbHoldDirection * 100 * dt;
			m_finalClimbTime -= dt;
		}
		m_velocity = glm::vec2(0);
		m_useClimbSprite = false;
		
		if (m_finalClimbTime < 0)
		{
			m_climbHoldDirection = 0;
			m_inFinalClimb = false;
		}
		return;
	}
	
	if (!IsBindingDown("climb"))
	{
		m_climbHoldDirection = 0;
	}
	
	if (m_climbedPlatform)
	{
		m_additionalMove += m_climbedPlatform->GetPosition() - m_climbedPlatform->GetOldPosition();
		if ((m_climbHoldDirection == -1 && !level::IntersectMovingPlatform(ClimbLeftRectangle(0, 0.1f, 0.9f))) ||
			(m_climbHoldDirection == 1 && !level::IntersectMovingPlatform(ClimbRightRectangle(0, 0.1f, 0.9f))))
		{
			m_velocity = glm::vec2(0);
			m_climbHoldDirection = 0;
		}
		return;
	}
	
	if (m_climbHoldDirection == -1)
	{
		if (!level::IntersectsSolid(ClimbLeftRectangle(0, 0.1f, 0.5f)))
		{
			m_climbHoldDirection = 0;
		}
		else if (
			!level::IntersectsSolid(ClimbLeftRectangle(0, CLIMB_LAST_PERCENT, 1)) &&
			level::IntersectsSolid(ClimbLeftRectangle(0, 0, CLIMB_LAST_PERCENT)))
		{
			m_inFinalClimb = true;
			m_finalClimbTime = FINAL_CLIMB_X_DURATION;
			m_finalClimbMoveUp = HEIGHT * (CLIMB_LAST_PERCENT + 0.05f);
		}
	}
	
	if (m_climbHoldDirection == 1)
	{
		if (!level::IntersectsSolid(ClimbRightRectangle(0, 0.1f, 0.5f)))
		{
			m_climbHoldDirection = 0;
		}
		else if (
			!level::IntersectsSolid(ClimbRightRectangle(0, CLIMB_LAST_PERCENT, 1)) &&
			level::IntersectsSolid(ClimbRightRectangle(0, 0, CLIMB_LAST_PERCENT)))
		{
			m_inFinalClimb = true;
			m_finalClimbTime = FINAL_CLIMB_X_DURATION;
			m_finalClimbMoveUp = HEIGHT * (CLIMB_LAST_PERCENT + 0.05f);
		}
	}
	
	if (m_climbHoldDirection == 0)
	{
		m_velocity = glm::vec2(0);
		return;
	}
	
	if (IsBindingDownNow("jump"))
	{
		if (MaybeJump())
		{
			m_velocity = glm::vec2(-m_climbHoldDirection * JUMP_ACCEL * 2, JUMP_ACCEL);
			m_climbHoldDirection = 0;
			return;
		}
	}
	
	m_velocity.x = 0;
	m_onGround = false;
	
	const float moveUD = AxisValueUD();
	
	float accelY = 0;
	if (std::abs(moveUD) < 0.1f)
	{
		if (m_velocity.y < 0)
		{
			m_velocity.y += dt * CLIMB_DEACCEL_AMOUNT;
			if (m_velocity.y > 0)
				m_velocity.y = 0.0f;
		}
		if (m_velocity.y > 0)
		{
			m_velocity.y -= dt * CLIMB_DEACCEL_AMOUNT;
			if (m_velocity.y < 0)
				m_velocity.y = 0.0f;
		}
	}
	else
	{
		accelY += CLIMB_ACCEL_AMOUNT * moveUD;
	}
	
	if ((accelY < 0) != (m_velocity.y < 0))
		accelY *= 4.0f;
	
	m_velocity.y += accelY * dt;
	
	float speed = std::abs(m_velocity.y);
	if (speed > CLIMB_SPEED)
	{
		m_velocity.y *= CLIMB_SPEED / speed;
	}
	
	m_climbAnimationTime += speed * dt / CLIMB_SPEED;
}

Rectangle Player::ClimbLeftRectangle(float w, float minYPC, float maxYPC) const
{
	return Rectangle(
		m_position.x - CLIMB_ATTACH_DIST_X - w,
		m_position.y + minYPC * HEIGHT,
		CLIMB_ATTACH_DIST_X + w,
		HEIGHT * maxYPC
	);
}

Rectangle Player::ClimbRightRectangle(float w,  float minYPC, float maxYPC) const
{
	return Rectangle(
		m_position.x + WIDTH,
		m_position.y + minYPC * HEIGHT,
		CLIMB_ATTACH_DIST_X + w,
		HEIGHT * maxYPC
	);
}

Rectangle Player::Rect() const
{
	return Rectangle(m_position, WIDTH, HEIGHT);
}

Rectangle Player::HitBox() const
{
	return Rectangle(m_position.x + HIT_BOX_MARGIN_X, m_position.y, WIDTH - HIT_BOX_MARGIN_X * 2, HEIGHT);
}

void Player::Draw()
{
	SpriteFlags flags = SpriteFlags::None;
	if (m_facingLeft)
		flags |= SpriteFlags::FlipX;
	
	glm::vec2 climbPosOff(m_climbHoldDirection * 5 - 1, 0);
	
	if (m_useClimbSprite)
	{
		const Texture2D& climbTexture = GetAsset<Texture2D>(m_hasJumped ? "Tex/PlayerClimbR.png" : "Tex/PlayerClimb.png");
		
		constexpr uint32_t FRAME_WIDTH = 16;
		
		constexpr float CLIMB_TIME_PER_FRAME = 0.1f;
		int frameIdx = (int)std::floor(m_climbAnimationTime / CLIMB_TIME_PER_FRAME) % 8;
		
		gfx2d->Sprite(climbTexture, m_position + climbPosOff, glm::vec4(1),
		              Rectangle(frameIdx * FRAME_WIDTH, 0, FRAME_WIDTH, climbTexture.Height()), 1, flags);
	}
	else if (m_useJumpSprite)
	{
		const Texture2D& jumpTexture = GetAsset<Texture2D>(m_hasJumped ? "Tex/PlayerJumpR.png" : "Tex/PlayerJump.png");
		constexpr uint32_t FRAME_WIDTH = 18;
		
		int frameIdx = (int)std::floor(m_jumpAnimationTime / JUMP_TIME_PER_FRAME);
		if (frameIdx >= JUMP_INIT_FRAMES + JUMP_LOOP_FRAMES)
			frameIdx = (frameIdx - JUMP_INIT_FRAMES) % JUMP_LOOP_FRAMES + JUMP_INIT_FRAMES;
		
		gfx2d->Sprite(jumpTexture, m_position + glm::vec2(-2, 0), glm::vec4(1),
		              Rectangle(frameIdx * FRAME_WIDTH, 0, FRAME_WIDTH, jumpTexture.Height()), 1, flags);
	}
	else if (m_useRunAnimation)
	{
		const Texture2D& runTexture = GetAsset<Texture2D>(m_hasJumped ? "Tex/PlayerRunR.png" : "Tex/PlayerRun.png");
		constexpr uint32_t FRAME_WIDTH = 18;
		
		constexpr float RUN_TIME_PER_FRAME = 0.1f;
		int frameIdx = (int)std::floor(m_animationTime / RUN_TIME_PER_FRAME) % 8;
		
		gfx2d->Sprite(runTexture, m_position + glm::vec2(-2, 0), glm::vec4(1),
		              Rectangle(frameIdx * FRAME_WIDTH, 0, FRAME_WIDTH, runTexture.Height()), 1, flags);
	}
	else
	{
		const Texture2D& idleTexture = GetAsset<Texture2D>(m_hasJumped ? "Tex/PlayerIdleR.png" : "Tex/PlayerIdle.png");
		constexpr uint32_t FRAME_WIDTH = 18;
		
		constexpr float IDLE_TIME_PER_FRAME = 0.3f;
		int frameIdx = (int)std::floor(m_animationTime / IDLE_TIME_PER_FRAME) % 5;
		
		gfx2d->Sprite(idleTexture, m_position + glm::vec2(-2, 0), glm::vec4(1),
		              Rectangle(frameIdx * FRAME_WIDTH, 0, FRAME_WIDTH, idleTexture.Height()), 1, flags);
	}
}

void Player::Reset()
{
	m_position = level::spawnPosition;
	m_velocity = glm::vec2(0);
	m_climbHoldDirection = 0;
	m_onGround = false;
	m_facingLeft = false;
	m_hasJumped = false;
}

void Player::Stop()
{
	m_velocity.x = 0;
	m_climbHoldDirection = 0;
}
