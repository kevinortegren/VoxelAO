#define _USE_MATH_DEFINES
#include "Camera.h"
#include <cmath>

Camera::Camera()
	: up(Vector3(0, 1, 0)), right(Vector3(0, 0, 1)), facing(Vector3(-1, 0, 0)), position(Vector3(800, 400, 0)), camSpeed(300.0f)
{}

Camera::~Camera()
{

}

void Camera::Update()
{
	camData.viewMat = Matrix::CreateLookAt(position, position + facing, up);
	camData.viewProjMat = camData.viewMat * camData.projMat;

	//Frustum stuff 

	float hheight = (float)std::tan((float)M_PI_4 * 0.5f);
	float hwidth = hheight * (mWindowWidth / mWindowHeight);

	Vector3 rightpos		= hwidth * mFarPlane * right* 0.2f;
	Vector3 toppos			= hheight * mFarPlane * up* 0.2f;
	Vector3 farcenterpos	= position + mFarPlane * 0.2f * facing;
	Vector3 nearcenterpos	= position + mNearPlane * facing;

	Vector3 ftl = farcenterpos - rightpos + toppos;
	Vector3 ftr = farcenterpos + rightpos + toppos;
	Vector3 fbl = farcenterpos - rightpos - toppos;
	Vector3 fbr = farcenterpos + rightpos - toppos;

	// All normals pointing outwards
	frustumPlanes[0] = Plane(position, fbl, ftl);		//Left plane
	frustumPlanes[1] = Plane(position, ftl, ftr);		//Top plane
	frustumPlanes[2] = Plane(position, ftr, fbr);		//Right plane
	frustumPlanes[3] = Plane(position, fbr, fbl);		//Bottom plane
	frustumPlanes[4] = Plane(farcenterpos, facing);		//Far plane
	frustumPlanes[5] = Plane(nearcenterpos, -facing);	//Near plane
}

void Camera::SetLens(float fov, float nearPlane, float farPlane, int width, int height)
{
	mNearPlane = nearPlane;
	mFarPlane = farPlane;
	mWindowWidth = (float)width;
	mWindowHeight = (float)height;

	camData.projMat = Matrix::CreatePerspectiveFieldOfView(fov, mWindowWidth / mWindowHeight, mNearPlane, mFarPlane);
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

bool Camera::IsInFrustum(Vector3 point)
{
	int inside_num_planes = 0;
	
	for (int i = 0; i < 6; i++)
		if (frustumPlanes[i].DotCoordinate(point) < 0.0f)
			inside_num_planes++;

	return inside_num_planes == 6; 
}