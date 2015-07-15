#pragma once
#include <SimpleMath.h>
using namespace DirectX::SimpleMath;

struct CameraData
{
	Matrix viewMat;
	Matrix projMat;
	Matrix viewProjMat;
};

class Camera
{
	

public:

	Camera();
	~Camera();

	void Update();
	void SetLens(float fov, float nearPlane, float farPlane, int width, int height);
	void Pitch(float angle);
	void Yaw(float angle);
	const CameraData& GetCamData();
	void MoveForward(float dt);
	void MoveBackward(float dt);
	void MoveUp(float dt);
	void MoveDown(float dt);
	void StrafeLeft(float dt);
	void StrafeRight(float dt);

	Vector3 up;
	Vector3 right;
	Vector3 facing;
	Vector3 position;
	float camSpeed;
private:

	

	CameraData camData;

	
};