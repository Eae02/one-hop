#pragma once

class MovingPlatform
{
public:
	
	float waitTime;
	float moveTime;
	float timeOffset;
	glm::vec2 start;
	glm::vec2 end;
	
	void Init();
	
	void Update(float dt);
	
	void Draw() const;
	
	glm::vec2 GetOldPosition() const
	{
		return m_oldPosition;
	}
	
	glm::vec2 GetPosition() const
	{
		return m_position;
	}
	
	Rectangle GetRectangle() const
	{
		return Rectangle(m_position.x - WIDTH / 2, m_position.y - 8, WIDTH, 8);
	}
	
	Rectangle GetOldRectangle() const
	{
		return Rectangle(m_oldPosition.x - WIDTH / 2, m_oldPosition.y - 8, WIDTH, 8);
	}
	
private:
	static constexpr float WIDTH = 5 * 16;
	
	float m_time = 0;
	glm::vec2 m_oldPosition;
	glm::vec2 m_position;
};
