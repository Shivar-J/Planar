#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

enum class Camera_Movement {
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	ROLL_LEFT,
	ROLL_RIGHT,
	UP,
	DOWN
};

const float SPEED = 10.0f;
const float ROLL_SPEED = 7.5f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f;

class Camera {
public:
	glm::vec3 Position;
	glm::quat Orientation;
	float RightAngle;
	float UpAngle;
	float RollAngle;

	float MovementSpeed;
	float MouseSensitivity;
	float Zoom;
	float RollSpeed;

	Camera(glm::vec3 position) : MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM), RollSpeed(ROLL_SPEED) {
		Position = position;
		Orientation = glm::quat(0, 0, 0, -1);
		RightAngle = 0.0f;
		UpAngle = 0.0f;
		RollAngle = 0.0f;
		updateCameraVectors();
	}

	Camera(float posX, float posY, float posZ) : MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM) {
		Position = glm::vec3(posX, posY, posZ);
		Orientation = glm::quat(0, 0, 0, -1);
		RightAngle = 0.0f;
		UpAngle = 0.0f;
		RollAngle = 0.0f;
		updateCameraVectors();
	}

	glm::mat4 GetViewMatrix() {
		glm::quat reverseOrientation = glm::conjugate(Orientation);
		glm::mat4 rotation = glm::mat4_cast(reverseOrientation);
		glm::mat4 translation = glm::translate(glm::mat4(1.0), -Position);

		return rotation * translation;
	}

	void ProcessKeyboard(Camera_Movement direction, float deltaTime) {
		float velocity = MovementSpeed * deltaTime;

		glm::quat qF = Orientation * glm::quat(0, 0, 0, -1) * glm::conjugate(Orientation);
		glm::vec3 Front = { qF.x, qF.y, qF.z };
		glm::vec3 Right = glm::normalize(glm::cross(Front, glm::vec3(0, 1, 0)));
		glm::vec3 Up = glm::normalize(glm::cross(Right, Front));
		
		if (direction == Camera_Movement::FORWARD)
			Position += Front * velocity;
		if (direction == Camera_Movement::BACKWARD)
			Position -= Front * velocity;
		if (direction == Camera_Movement::LEFT)
			Position -= Right * velocity;
		if (direction == Camera_Movement::RIGHT)
			Position += Right * velocity;
		if (direction == Camera_Movement::UP)
			Position += Up * velocity;
		if (direction == Camera_Movement::DOWN)
			Position -= Up * velocity;

		if (direction == Camera_Movement::ROLL_LEFT) {
			RollAngle -= deltaTime * ROLL_SPEED;
		}
		if (direction == Camera_Movement::ROLL_RIGHT) {
			RollAngle += deltaTime * ROLL_SPEED;
		}

		updateCameraVectors();
	}

	void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true) {
		xoffset *= MouseSensitivity;
		yoffset *= MouseSensitivity;

		RightAngle += xoffset;
		UpAngle += yoffset;

		if (UpAngle >= 89.0f)
			UpAngle = 89.0f;
		if (UpAngle <= -89.0f)
			UpAngle = -89.0f;

		updateCameraVectors();
	}

	void ProcessMouseScroll(float yoffset) {
		if (Zoom >= 1.0f && Zoom <= 45.0f)
			Zoom -= yoffset;
		if (Zoom <= 1.0f)
			Zoom = 1.0f;
		if (Zoom >= 45.0f)
			Zoom = 45.0f;
	}

	void updateCameraVectors() {
		glm::quat aroundY = glm::angleAxis(glm::radians(-RightAngle), glm::vec3(0, 1, 0));
		glm::quat aroundX = glm::angleAxis(glm::radians(UpAngle), glm::vec3(1, 0, 0));
		glm::quat roll = glm::angleAxis(glm::radians(RollAngle), glm::vec3(0.0f, 0.0f, 1.0f));

		Orientation = roll * aroundY * aroundX;
		
		Orientation = glm::normalize(Orientation);
	}
};

#endif // !CAMERA_H
