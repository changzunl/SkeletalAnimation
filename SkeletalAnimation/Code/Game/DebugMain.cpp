#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"

bool EpsilonEquals(float a, float b)
{
	// constexpr float epsilon = 1.192092896e-07f;
	constexpr float epsilon = 0.001f;

	return fabsf(a - b) < epsilon;
}

bool EpsilonEquals(const Vec3& a, const Vec3& b)
{
	return EpsilonEquals(a.x, b.x)
		&& EpsilonEquals(a.y, b.y)
		&& EpsilonEquals(a.z, b.z);
}

bool EpsilonEquals(const EulerAngles& a, const EulerAngles& b)
{
	return EpsilonEquals(a.m_yawDegrees  , b.m_yawDegrees  )
		&& EpsilonEquals(a.m_pitchDegrees, b.m_pitchDegrees)
		&& EpsilonEquals(a.m_rollDegrees , b.m_rollDegrees );
}

void DebugEulerToVec()
{
	// 	DebuggerPrintf("Test for euler angles conversion:\n");
	// 
	// 	EulerAngles rotation(360, 0, 0);
	// 
	// 	EulerAngles rotation2 = DirectionToRotation(rotation.GetVectorXForward());
	// 
	// 	DebuggerPrintf(EpsilonEquals(rotation.GetVectorXForward(), rotation2.GetVectorXForward()) ? "true" : "false");
	// 	DebuggerPrintf("\n");
}

#include "Networking.hpp"

void DebugBuffer()
{
// 	constexpr UINT bufferSize = 64;
// 
// 	PacketBuffer buffer;
// 
// 	buffer.Write(123321123321ui64);
// 
// 	PacketBufferObject clientBuffer(bufferSize);
// 
// 	HPKT pkt0 = clientBuffer.AllocPacket(buffer.GetSize(), buffer.GetData());
// 
// 	UCHAR(*pData)[bufferSize] = (UCHAR(*)[bufferSize]) clientBuffer.ResolveData(pkt0);
// 
// 	clientBuffer.FreePacket(pkt0);
// 
// 	buffer.Write(321123ui32);
// 	HPKT pkt1 = clientBuffer.AllocPacket(buffer.GetSize(), buffer.GetData());
// 	buffer.Write(12345ui16);
// 	HPKT pkt2 = clientBuffer.AllocPacket(buffer.GetSize(), buffer.GetData());
// 	buffer.Write(132ui8);
// 	HPKT pkt3 = clientBuffer.AllocPacket(buffer.GetSize(), buffer.GetData());
// 	buffer.Write(Rgba8(123, 231, 213, 132));
// 	HPKT pkt4 = clientBuffer.AllocPacket(buffer.GetSize(), buffer.GetData()); // full error
// 	clientBuffer.FreePacket(pkt1);
// 	clientBuffer.FreePacket(pkt2);
// 	clientBuffer.CleanAndRoll(); // gc
// 	clientBuffer.FreePacket(pkt3);
// 	clientBuffer.CleanAndRoll(); // gc
// 	HPKT pkt5 = clientBuffer.AllocPacket(buffer.GetSize(), buffer.GetData());
// 	buffer.Write(Vec3(1.0f, 12.0f, 123.0f));
// 	HPKT pkt6 = clientBuffer.AllocPacket(buffer.GetSize(), buffer.GetData()); // full error
// 
// 	UCHAR* p1 = clientBuffer.ResolveData(pkt1);
// 	UCHAR* p2 = clientBuffer.ResolveData(pkt2);
// 	UCHAR* p3 = clientBuffer.ResolveData(pkt3);
// 	UCHAR* p4 = clientBuffer.ResolveData(pkt4);
// 	UCHAR* p5 = clientBuffer.ResolveData(pkt5);
// 	UCHAR* p6 = clientBuffer.ResolveData(pkt6);
}

#include "Engine/Animation/AssetImporter.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"

#include "ThirdParty/assimp/scene.h"

#include "Engine/Animation/Mesh.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Vec4.hpp"
#include "Engine/Math/Mat4x4.hpp"

#include "Engine/Math/Transformation.hpp"
#include "Engine/Animation/Quaternion.hpp"

bool DebugMain()
{
// 	DebugEulerToVec();
// 	DebugBuffer();

// 	Transformation transform;
// 	transform.m_position = Vec3(1.23f, 4.56f, 7.89f);
// 	transform.m_scale = Vec3(-3.0f, -6.0f, -9.0f);
// 	transform.m_orientation = EulerAngles(14.7f, 25.8f, 36.9f);
// 
// 	Quaternion quat = Quaternion::FromAxisAndAngle(Vec3(0, 0, 1), PI / 2);
// 
// 	Mat4x4 mat;
// 	mat.AppendTranslation3D(transform.m_position);
// 	mat.Append(quat.GetMatrix());
// 	mat.AppendScaleNonUniform3D(transform.m_scale);
// 
// 	transform.m_orientation = EulerAngles::FromMatrix(quat.GetMatrix());
// 
// 	transform = Transformation::DecomposeAffineMatrix(mat);
// 
// 	transform.GetMatrix();

	EulerAngles rotation = EulerAngles(14.7f, 25.8f, 36.9f);
	Mat4x4 matRot = rotation.GetMatrix_XFwd_YLeft_ZUp();

	Quaternion quat = Quaternion::FromMatrix(Mat3x3::GetSubMatrix(matRot, 3, 3));

	Mat4x4 matRot2 = quat.GetMatrix();

	return false;
}

