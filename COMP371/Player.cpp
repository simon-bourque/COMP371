#include <iostream>

#include "Player.h"
#include "ChunkManager.h"
#include "AABBCollider.h"
#include "SphereCollider.h"
#include "InputManager.h"

glm::vec2 getMouseAxis();

Player::Player(Camera* camera)
	: m_camera(camera)
	, m_position(glm::vec3(0.0f))
	, m_forwardPressed(false)
	, m_backwardPressed(false)
	, m_leftPressed(false)
	, m_rightPressed(false)
	, m_jumpPressed(false)
	, m_rightMouseButtonPressed(false)
	, m_isJumping(false)
	, m_ready(false)
	, m_jumpedFrames(0)
	, m_collisionMode(CollisionMode::AABB)
{
	InputManager::instance()->registerKeyCallback(std::bind(&Player::onKey, this, std::placeholders::_1, std::placeholders::_2));
	InputManager::instance()->registerMouseBtnCallback(std::bind(&Player::onMouseButton, this, std::placeholders::_1, std::placeholders::_2));
}

Player::~Player() {}

void Player::update(float32 deltaSeconds)
{
	const static float32 SPEED = 100.0f;
	const static float32 ROTATE_SPEED = 0.01f;
	const static int MAX_JUMP_FRAMES = 15;

	static bool hasLanded = false;

	m_position = glm::vec3(transform.xPos, transform.yPos, transform.zPos);

	float32 dx = 0;
	float32 dy = 0;
	float32 dz = 0;

	if (m_forwardPressed) {
		--dz;
	}
	if (m_backwardPressed) {
		++dz;
	}
	if (m_leftPressed) {
		--dx;
	}
	if (m_rightPressed) {
		++dx;
	}
	if (m_jumpPressed) {
		if (!m_isJumping && hasLanded)
		{
			m_isJumping = true;
			hasLanded = false;
		}
	}

	if(m_ready && !m_isJumping)
		dy += GRAVITY * deltaSeconds;
	else if (m_ready && m_isJumping)
	{
		dy -= GRAVITY * deltaSeconds;
		m_jumpedFrames++;

		if (m_jumpedFrames > MAX_JUMP_FRAMES)
		{
			m_jumpedFrames = 0;
			m_isJumping = false;
		}
	}

	glm::vec3 deltaPos(dx, dy, dz);
	deltaPos = (glm::normalize(deltaPos) * SPEED * deltaSeconds);

	// if you are in the water (below it), move half as much per frame
	if (m_position.y < m_swimY)
		deltaPos = glm::clamp(deltaPos, -0.45f, 0.45f);
	else
		deltaPos = glm::clamp(deltaPos, -0.9f, 0.9f);

	//Check if the chunk you are in has changed
	checkChunk();

	glm::vec3 newPosition = glm::vec3(transform.xPos + deltaPos.x, transform.yPos + deltaPos.y, transform.zPos + deltaPos.z);

	Collision coll = checkForShubbery(newPosition);

	if (coll)
	{
		// congrats you hit a tree...
		switch (coll)
		{
		case(Collision::Colliding):
			hasLanded = true;
			break;
		case(Collision::CollidingXZ):
			if (dy != 0)
				transform.translateLocal(0.0f, deltaPos.y, 0.0f);
			break;
		}
	}
	else
	{
		coll = checkForSurroundingBlocks(newPosition);

		switch (coll)
		{
		case(Collision::Colliding):
			std::cout << "Colliding" << std::endl;
			hasLanded = true;
			break;
		case(Collision::NoCollision):
			if (dx != 0 || dy != 0 || dz != 0)
				transform.translateLocal(deltaPos.x, deltaPos.y, deltaPos.z);
			break;
		case(Collision::CollidingNotY):
			if (dx != 0 || dz != 0)
				transform.translateLocal(deltaPos.x, 0.0f, deltaPos.z);
			hasLanded = true;
			break;
		case(Collision::NoCollisionUpOne):
			if (dx != 0 || dz != 0)
				transform.translateLocal(deltaPos.x, 1.0f, deltaPos.z);
			hasLanded = true;
			break;
		}
	}

	// update the camera's transform
	m_camera->transform.xPos = transform.xPos;
	m_camera->transform.yPos = transform.yPos + 1.0f;
	m_camera->transform.zPos = transform.zPos;

	glm::vec2 mouseAxis = getMouseAxis() * ROTATE_SPEED;
	if (m_rightMouseButtonPressed) {

		if (abs(mouseAxis.x) > abs(mouseAxis.y)) {
			mouseAxis.y = 0;
		}
		else {
			mouseAxis.x = 0;
		}

		transform.rotate(0, -mouseAxis.x, 0);

		m_camera->transform.rotateLocal(mouseAxis.y, 0, 0);
		m_camera->transform.rotate(0, -mouseAxis.x, 0);
	}
}

void Player::checkChunk()
{
	glm::vec3 currentChunkPosition = ChunkManager::instance()->getCurrentChunk(m_position);

	if (currentChunkPosition != m_chunkPosition)
	{
		Chunk c;
		bool gotChunk = ChunkManager::instance()->getChunkHandle(currentChunkPosition, c);

		if (gotChunk)
		{
			m_chunkPosition = currentChunkPosition;
			m_chunkPositions = c.getBlockPositions();
			m_chunkPositionsFoliage = c.getFoliageBlockPositions();

			// If gravity was not enabled, well then enable it
			if(!m_ready)
				m_ready = true;
		}
	}
}

Collision Player::checkForShubbery(const glm::vec3& newPosition)
{
	glm::vec3 noXZ = glm::vec3(m_position.x, newPosition.y, m_position.y);

	if (m_collisionMode == CollisionMode::AABB)
	{
		AABBCollider me = AABBCollider::centeredOnPoint(newPosition, 1.2f);
		AABBCollider me2 = AABBCollider::centeredOnPoint(noXZ, 1.2f);

		if (m_chunkPositionsFoliage.size() > 0)
		{
			for (auto& it : m_chunkPositionsFoliage)
			{
				AABBCollider other = AABBCollider::centeredOnPoint(it, 1.2f);
				if (AABBCollider::checkCollision(me, other))
				{
					if (AABBCollider::checkCollision(me2, other))
					{
						std::cout << "almost hittin' them good'ol foliage-xz" << std::endl;
						return Collision::CollidingXZ;
					}
					std::cout << "hittin' them good'ol foliage" << std::endl;
					return Collision::Colliding;
				}
			}
		}
	}
	else
	{
		SphereCollider meAll = SphereCollider(newPosition, 1.0f);
		SphereCollider meNoXZ = SphereCollider(noXZ, 1.0f);

		if (m_chunkPositionsFoliage.size() > 0)
		{
			for (auto& it : m_chunkPositionsFoliage)
			{
				SphereCollider other = SphereCollider(it, 1.0f);
				if (SphereCollider::checkCollision(meAll, other))
				{
					if (SphereCollider::checkCollision(meNoXZ, other))
					{
						std::cout << "almost hittin' them good'ol foliage-xz" << std::endl;
						return Collision::CollidingXZ;
					}
					std::cout << "hittin' them good'ol foliage" << std::endl;
					return Collision::Colliding;
				}
			}
		}
	}

	return Collision::NoCollision;
}

Collision Player::checkForSurroundingBlocks(const glm::vec3& newPosition)
{
	glm::vec3 noY = glm::vec3(newPosition.x, m_position.y, newPosition.z);
	glm::vec3 upOne = glm::vec3(newPosition.x, m_position.y + 1.0f, newPosition.z);
	if (m_collisionMode == CollisionMode::AABB)
	{
		AABBCollider me = AABBCollider::centeredOnPoint(newPosition, 1.2f);
		AABBCollider me2 = AABBCollider::centeredOnPoint(noY, 1.2f);
		AABBCollider me3 = AABBCollider::centeredOnPoint(upOne, 1.2f);

		if (m_chunkPositions.size() > 0)
		{
			for (auto& it : m_chunkPositions)
			{
				AABBCollider other = AABBCollider::centeredOnPoint(it, 1.2f);
				if (AABBCollider::checkCollision(me, other))
				{
					if (AABBCollider::checkCollision(me2, other))
					{
						if (AABBCollider::checkCollision(me3, other))
							return Collision::Colliding;
						else
							return Collision::NoCollisionUpOne;
					}
					else
						return Collision::CollidingNotY;
				}
			}
		}
	}
	else
	{
		SphereCollider meAll = SphereCollider(newPosition, 1.0f);
		SphereCollider meNoY = SphereCollider(noY, 1.0f);
		SphereCollider meUpOne = SphereCollider(upOne, 1.0f);
		if (m_chunkPositions.size() > 0)
		{
			for (auto& it : m_chunkPositions)
			{
				SphereCollider other = SphereCollider(it, 1.0f);
				if (SphereCollider::checkCollision(meAll, other))
				{
					if (SphereCollider::checkCollision(meNoY, other))
					{
						if (SphereCollider::checkCollision(meUpOne, other))
							return Collision::Colliding;
						else
							return Collision::NoCollisionUpOne;
					}
					else
						return Collision::CollidingNotY;
				}
			}
		}
	}

	return Collision::NoCollision;
}

void Player::onKey(int32 key, int32 action)
{
	if (key == GLFW_KEY_W) {
		if (action == GLFW_PRESS) {
			m_forwardPressed = true;
		}
		else if (action == GLFW_RELEASE) {
			m_forwardPressed = false;
		}
	}
	if (key == GLFW_KEY_S) {
		if (action == GLFW_PRESS) {
			m_backwardPressed = true;
		}
		else if (action == GLFW_RELEASE) {
			m_backwardPressed = false;
		}
	}
	if (key == GLFW_KEY_A) {
		if (action == GLFW_PRESS) {
			m_leftPressed = true;
		}
		else if (action == GLFW_RELEASE) {
			m_leftPressed = false;
		}
	}
	if (key == GLFW_KEY_D) {
		if (action == GLFW_PRESS) {
			m_rightPressed = true;
		}
		else if (action == GLFW_RELEASE) {
			m_rightPressed = false;
		}
	}
	if (key == GLFW_KEY_SPACE) {
		if (action == GLFW_PRESS) {
			m_jumpPressed = true;
		}
		else if (action == GLFW_RELEASE) {
			m_jumpPressed = false;
		}
	}
}
void Player::onMouseButton(int32 button, int32 action)
{
	if (button == GLFW_MOUSE_BUTTON_RIGHT) {
		if (action == GLFW_PRESS) {
			m_rightMouseButtonPressed = true;
		}
		else if (action == GLFW_RELEASE) {
			m_rightMouseButtonPressed = false;
		}
	}
}