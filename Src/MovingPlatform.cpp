#include "MovingPlatform.hpp"
#include "Common.hpp"

void MovingPlatform::Update(float dt)
{
	m_time += dt;
	
	m_oldPosition = m_position;
	
	const float t = std::fmod(m_time + timeOffset, (waitTime + moveTime) * 2);
	if (t < waitTime)
		m_position = start;
	else if (t < waitTime + moveTime)
		m_position = glm::mix(start, end, glm::smoothstep(waitTime, waitTime + moveTime, t));
	else if (t < waitTime * 2 + moveTime)
		m_position = end;
	else
		m_position = glm::mix(end, start, glm::smoothstep(waitTime * 2 + moveTime, (waitTime + moveTime) * 2, t));
}

void MovingPlatform::Draw() const
{
	gfx2d->Sprite(GetAsset<Texture2D>("Tex/Platform.png"), GetRectangle(), glm::vec4(1), SpriteFlags::FlipY);
}

void MovingPlatform::Init()
{
	m_position = start;
	m_oldPosition = start;
}
