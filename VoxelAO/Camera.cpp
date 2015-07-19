#include "Camera.h"

Camera::Camera()
	: up(Vector3(0, 1, 0)), right(Vector3(1, 0, 0)), facing(Vector3(0, 0, 1)), position(Vector3(0, 100, 0)), camSpeed(300.0f)
{}

Camera::~Camera()
{

}

void Camera::Update()
{
	camData.viewMat = Matrix::CreateLookAt(position, position + facing, up);
	camData.viewProjMat = camData.viewMat * camData.projMat;
}

void Camera::SetLens(float fov, float nearPlane, float farPlane, int width, int height)
{
	camData.projMat = Matrix::CreatePerspectiveFieldOfView(fov, (float)width / (float)height, nearPlane, farPlane);
}

void Camera::Pitch(float angle)
{
	Matrix rot = Matrix::CreateFromAxisAngle(right, angle);
	facing = Vector3::TransformNormal(facing, rot);
	up = Vector3::TransformNormal(up, rot);
}

void Camera::Yaw(float angle)
{
	Matrix rot = Matrix::CreateFromAxisAngle(Vector3(0, 1, 0), angle);
	facing = Vector3::TransformNormal(facing, rot);
	up = Vector3::TransformNormal(up, rot);
	right = Vector3::TransformNormal(right, rot);
}

const CameraData& Camera::GetCamData()
{
	return camData;
}

void Camera::MoveForward(float dt)
{
	position += camSpeed * dt * facing;
}

void Camera::MoveBackward(float dt)
{
	position -= camSpeed * dt * facing;
}

void Camera::MoveUp(float dt)
{
	position += camSpeed * dt * up;
}

void Camera::MoveDown(float dt)
{
	position -= camSpeed * dt * up;
}

void Camera::StrafeLeft(float dt)
{
	position += camSpeed * dt * right;
}

void Camera::StrafeRight(float dt)
{
	position -= camSpeed * dt * right;
}