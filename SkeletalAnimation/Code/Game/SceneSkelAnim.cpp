#include "SceneSkelAnim.hpp"

#include "App.hpp"
#include "GameCommon.hpp"
#include "RenderUtils.hpp"
#include "Game.hpp"
#include "SoundClip.hpp"
#include "RenderUtils.hpp"
#include "Networking.hpp"

#include "Engine/Animation/Animation.hpp"
#include "Engine/Animation/AssetImporter.hpp"
#include "Engine/Animation/SkeletalMesh.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/SimpleTriangleFont.hpp"
#include "Engine/Window/Window.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Core/EventSystem.hpp"

#include "Engine/Core/ByteBuffer.hpp"

#include <math.h>
#include <vector>


constexpr bool TEST_CONV_ANIM_TO_FRAMES = false;
constexpr bool TEST_MODEL_SERIALIZATION = true;


std::vector<VertexFormat> g_SkeletalShaderLayout;
Shader* g_SkeletalShader;

bool Command_Load(EventArgs& args)
{
	SceneSkelAnim* scene = dynamic_cast<SceneSkelAnim*>(g_theGame->GetCurrentScene());
	if (!scene)
	{
		g_theConsole->AddLine(DevConsole::LOG_INFO, "Cannot load in current scene!");
		return true;
	}

	std::string model = args.GetValue("model", "Swimming");
	std::string animation = args.GetValue("animation", model.c_str());

	g_theConsole->AddLine(DevConsole::LOG_INFO, Stringf("Loading model file %s...", model.c_str()));
	scene->LoadModel(model.c_str());
	g_theConsole->AddLine(DevConsole::LOG_INFO, Stringf("Loading animation file %s...", animation.c_str()));
	scene->LoadAnimation(animation.c_str());
	return true;
}

bool InitializeModelCommands()
{
	g_theEventSystem->SubscribeEventCallbackFunction("LoadModel", Command_Load);

	return true;
}

SceneSkelAnim::SceneSkelAnim(Game* game) : Scene(game)
{
	static bool commandLoad = InitializeModelCommands();

	auto& layout = g_SkeletalShaderLayout;
	auto& shader = g_SkeletalShader;

	if (!shader)
	{
		layout.resize(6);
		layout[0].AddElement(VertexFormatElement::POSITION3F);
		layout[1].AddElement(VertexFormatElement::NORMAL3F);
		layout[2].AddElement(VertexFormatElement::COLOR4UB);
		layout[3].AddElement(VertexFormatElement::TEXCOORD2F);
		layout[4].AddElement(VertexFormatElement("BONE_IDS", VertexFormatElement::Format::UB4, VertexFormatElement::Semantic::GENERIC));
		layout[5].AddElement(VertexFormatElement("BONE_WEIGHTS", VertexFormatElement::Format::FLOAT4, VertexFormatElement::Semantic::GENERIC));
		for (int i = 0; i < layout.size(); i++)
			layout[i].m_bufferSlot = i;

		std::string fileName = "Data/Shaders/SkeletalLit.hlsl";
		std::string source;
		if (FileReadToString(source, fileName) < 0)
			ERROR_AND_DIE(Stringf("Shader source file not found: ", fileName.c_str()));
		shader = g_theRenderer->CreateShader("SkeletalLit", source, layout);
		g_theRenderer->InitializeCustomConstantBuffer(4, sizeof(SkeletonConstants));
	}

	LoadModel("Swimming");
	LoadAnimation("Swimming");

	m_effector = Vec3(-20, -0, 160);
}

SceneSkelAnim::~SceneSkelAnim()
{
	delete m_animation;
	m_animation = nullptr;
	delete m_mesh;
	m_mesh = nullptr;
	for (auto* vbo : m_vbos)
		delete vbo;
	m_vbos.clear();
	delete m_ibo;
	m_ibo = nullptr;
}

void SceneSkelAnim::Initialize()
{
	g_theGame->PlayBGM(g_theGame->m_bgmAttract);
}

void SceneSkelAnim::Shutdown()
{
}

void SceneSkelAnim::Update()
{
	if (m_isQuitting)
	{
		m_quitTimeCounter -= (float)m_clock.GetDeltaTime();
		if (m_quitTimeCounter <= 0.0f)
		{
			g_theApp->Quit();
			return;
		}
	}
	else
	{
		HandleInput();
	}

	auto& pose = *m_pose;
	pose = m_mesh->m_skeleton.GetPose();
// 	{
// 		BoneId boneId = mesh->m_skeleton.FindBone("spine_01");
// 		pose.m_boneLocalPose[boneId].m_orientation.m_pitchDegrees = cosf(GetLifeTime() * 5.0f) * 3.0f * 1.0f;
// 	}
// 	{
// 		BoneId boneId = mesh->m_skeleton.FindBone("spine_02");
// 		pose.m_boneLocalPose[boneId].m_orientation.m_pitchDegrees = cosf(GetLifeTime() * 5.0f) * 3.0f * 2.0f;
// 	}
// 	{
// 		BoneId boneId = mesh->m_skeleton.FindBone("spine_03");
// 		pose.m_boneLocalPose[boneId].m_orientation.m_pitchDegrees = cosf(GetLifeTime() * 5.0f) * 3.0f * 3.0f;
// 	}
// 	{
// 		BoneId boneId = mesh->m_skeleton.FindBone("neck_01");
// 		pose.m_boneLocalPose[boneId].m_orientation.m_pitchDegrees = cosf(GetLifeTime() * 5.0f) * 3.0f * 4.0f;
// 	}
// 	{
// 		BoneId boneId = mesh->m_skeleton.FindBone("head");
// 		pose.m_boneLocalPose[boneId].m_orientation.m_pitchDegrees = cosf(GetLifeTime() * 5.0f) * 3.0f * 5.0f;
// 	}

	AnimationFrame frame = m_animation->Sample(GetLifeTime(), pose);
	frame.Apply(pose);
	pose.BakeLocalToComp();
	pose.BakeFromComp();
}

void SceneSkelAnim::UpdateCamera()
{
	m_worldCamera[0].SetPerspectiveView(float(g_theWindow->GetClientDimensions().x) / float(g_theWindow->GetClientDimensions().y), 60.0f, 0.1f, 10000.0f);

	// transform game conventions(x in y left z up) to dx conventions(x right y up z in)
	// dx.x = -game.y, dx.y = game.z, dx.z = game.x
	m_worldCamera[0].SetRenderTransform(Vec3(0, -1, 0), Vec3(0, 0, 1), Vec3(1, 0, 0));

	// transform unreal conventions(x right y in z up) to dx conventions(x right y up z in)
	// dx.x = game.x, dx.y = game.z, dx.z = game.y
	// m_worldCamera[0].SetRenderTransform(Vec3(1, 0, 0), Vec3(0, 0, 1), Vec3(0, 1, 0)); 
	m_worldCamera[0].SetViewTransform(m_cameraPos.m_position, m_cameraPos.m_orientation);

	m_screenCamera[0].SetOrthoView(g_theRenderer->GetViewport().m_mins, g_theRenderer->GetViewport().m_maxs);

	Scene::UpdateCamera();
}

void SceneSkelAnim::RenderWorld() const
{
	g_theRenderer->SetFillMode(FillMode::SOLID);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetDepthMask(false);
	g_theRenderer->SetDepthTest(DepthTest::ALWAYS);
	g_theRenderer->SetCullMode(CullMode::NONE);
	g_theRenderer->SetSamplerMode(SamplerMode::BILINEARWRAP);

//   m_game->RenderEntities(ENTITY_MASK_EFFECT);
//   m_game->RenderEntities(ENTITY_MASK_ENEMY);
	g_theRenderer->SetTintColor(Rgba8::WHITE);
	g_theRenderer->BindTexture(nullptr);


	g_theRenderer->SetFillMode(FillMode::WIREFRAME);
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetDepthMask(true);
	g_theRenderer->SetDepthTest(DepthTest::LESSEQUAL);
	g_theRenderer->SetTintColor(Rgba8::WHITE);
	g_theRenderer->BindTexture(nullptr);

	// transform unreal conventions(x right y in z up) to game conventions(x in y left z up)
	// game.x = assimp.y, game.y = -assimp.x, game.z = assimp.y
	Mat4x4 conv = Mat4x4(Vec3(0, 1, 0), Vec3(-1, 0, 0), Vec3(0, 0, 1), Vec3::ZERO).GetOrthonormalInverse();


	Transformation trans;
	trans.m_position = Vec3(200, 0, -100);
	trans.m_scale = Vec3(1.0f, 1.0f, 1.0f);
	trans.m_orientation.m_yawDegrees = 0.0f; //  +GetLifeTime() * 30.0f;
	trans.m_orientation.m_pitchDegrees = 0.0f; //  +GetLifeTime() * 30.0f;

	// IK
	Pose& pose = *m_pose;


	FABRIKSolver solver;
	solver.m_convMat = conv;
	solver.m_effector.m_position = m_effector;

	if (m_ikHead)
	{
		solver.m_rootBone = pose.m_skeleton->FindBone("spine_01");
		solver.m_targetBone = pose.m_skeleton->FindBone("head");
	}
	else
	{
		solver.m_rootBone = pose.m_skeleton->FindBone("lowerarm_r");
		solver.m_targetBone = pose.m_skeleton->FindBone("index_01_r");
	}

	std::deque<FABRIKNode> nodesInitial;
	std::deque<FABRIKNode> nodesAfter;
	solver.Solve(pose, &nodesInitial, &nodesAfter);

	g_theRenderer->SetModelMatrix(trans.GetMatrix() * conv);
	g_theRenderer->SetFillMode(FillMode::SOLID);
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetDepthMask(true);
	g_theRenderer->SetDepthTest(DepthTest::LESSEQUAL);
	g_theRenderer->SetCullMode(CullMode::BACK);
	g_theRenderer->SetSamplerMode(SamplerMode::BILINEARWRAP);

	g_theRenderer->BindShader(g_SkeletalShader);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetCustomConstantBuffer(4, pose.m_bakedPose.GetBuffer());
	g_theRenderer->DrawIndexedVertexBuffer(m_ibo, (int)m_vbos.size(), (VertexBuffer**)m_vbos.data(), (int)m_mesh->m_indices.size());
	g_theRenderer->BindShader(nullptr);

	{
		// convention transform
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->SetDepthMask(false);
		g_theRenderer->SetDepthTest(DepthTest::ALWAYS);

		static VertexList verts1;
		static VertexList verts2;
		static VertexList verts3;
		verts1.clear();
		verts2.clear();
		verts3.clear();
		AddVertsForXCone(verts3, Vec3(), 1.0f, 1.0f, Rgba8(255, 255, 255, 60));

		auto& skel = m_mesh->m_skeleton;

		for (auto& bone : skel)
		{
			if (bone.m_parentId == INVALID_BONE_ID)
				continue;

			TransformQuat boneTrans = pose.m_boneCompPose[bone.m_id];
			TransformQuat boneTransP = pose.m_boneCompPose[bone.m_parentId];

			Vec3 offset = boneTrans.m_position - boneTransP.m_position;

			Mat4x4 mat;
			mat.AppendTranslation3D(boneTransP.m_position);
			mat.Append(DirectionToRotation(offset).GetMatrix_XFwd_YLeft_ZUp());
			mat.AppendScaleNonUniform3D(Vec3(offset.GetLength(), 1.0f, 1.0f));

			verts2 = verts3;
			TransformVertexArray(mat, verts2);
			SetColorForVertices((int)verts2.size(), verts2.data(), bone.m_id == m_highlightBone ? Rgba8(0, 255, 0, 127) : Rgba8(255, 255, 255, 60));
			for (auto& val : verts2)
				verts1.push_back(val);

			mat = Mat4x4::IDENTITY;
			mat.AppendTranslation3D(boneTrans.m_position);
			mat.Append(boneTrans.m_rotation.GetMatrix());
			mat.AppendScaleNonUniform3D(Vec3(2.0f, 0.5f, 0.5f));

			verts2 = verts3;
			TransformVertexArray(mat, verts2);
			SetColorForVertices((int)verts2.size(), verts2.data(), bone.m_id == m_highlightBone ? Rgba8(255, 0, 0, 127) : Rgba8(255, 255, 0, 60));
			for (auto& val : verts2)
				verts1.push_back(val);
		}

		g_theRenderer->DrawVertexArray(verts1);

		AABB2 uv = AABB2(0.0f, 0.0f, 1.0f, 1.0f);
		verts1.clear();
		verts1.reserve(1024 * 1024);
		for (auto& node : nodesInitial)
		{
			AddVertsForSphere(verts1, node.m_position,  0.3f, Rgba8::WHITE, uv, 8);
			AddVertsForSphere(verts1, node.GetOrigin(), 0.3f, Rgba8::GREEN, uv, 8);
		}

		for (auto& node : nodesAfter)
		{
			AddVertsForSphere(verts1, node.m_position,  0.3f, Rgba8::RED, uv, 8);
			AddVertsForSphere(verts1, node.GetOrigin(), 0.3f, Rgba8(255, 255, 0), uv, 8);
		}

		g_theRenderer->DrawVertexArray(verts1);

		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->SetDepthMask(true);
		g_theRenderer->SetDepthTest(DepthTest::LESSEQUAL);
	}

	DebugAddMessage("WASD/QE = move camera, IJKL/UO = move IK effector, R = slow, F/G = change bone highlight, H = change IK target bone", 0.0f, Rgba8::WHITE, Rgba8::WHITE);

	auto* hlBone = m_mesh->m_skeleton.FindBone(m_highlightBone);
	auto* ikRoot = m_mesh->m_skeleton.FindBone(solver.m_rootBone);
	auto* ikBone = m_mesh->m_skeleton.FindBone(solver.m_targetBone);

	std::string msg = Stringf("Current bone: %s, Current IK target: %s -> %s", hlBone->m_name.c_str(), ikRoot->m_name.c_str(), ikBone->m_name.c_str());
	DebugAddMessage(msg, 0.0f, Rgba8::WHITE, Rgba8::WHITE);
}

void SceneSkelAnim::RenderUI() const
{
	g_theRenderer->SetFillMode(FillMode::SOLID);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetDepthMask(false);
	g_theRenderer->SetDepthTest(DepthTest::ALWAYS);
	g_theRenderer->SetCullMode(CullMode::NONE);
	g_theRenderer->SetSamplerMode(SamplerMode::POINTCLAMP);

	RenderUILogoText();

	DebugRenderScreen(GetScreenCamera());
}

void SceneSkelAnim::RenderUILogoText() const
{
	std::vector<Vertex_PCU> verts;

	// Logo
	Vec2 textPosition = g_theRenderer->GetViewport().GetPointAtUV(Vec2(0.5f, 0.9f));
	float fontSize = Clamp(GetLifeTime() * 150.0f, 0.0f, 50.0f);
	AddVertsForText(verts, "SKELETAL ANIMATION", 1.0f, 0.5f);
	RenderFontVertices((int) verts.size(), &verts[0], textPosition, Rgba8(), fontSize, true);
	verts.clear();

	// Propaganda
	textPosition = g_theRenderer->GetViewport().GetPointAtUV(Vec2(0.5f, 0.5f));
	fontSize = 80.0f;
	AddVertsForText(verts, "TEST SCENE", 2.0f, 0.25f);
	// RenderFontVertices((int)verts.size(), &verts[0], textPosition, Rgba8(127, 127, 127, 127), fontSize, false);
	verts.clear();
}

void SceneSkelAnim::HandleInput()
{
	// input handle, return = handled one event


	// controller join/quit
	for (int i = 0; i < NUM_XBOX_CONTROLLERS; i++)
	{
		const auto& controller = g_theInput->GetXboxController(i);

		if (controller.IsConnected())
		{
			if (controller.WasButtonJustPressed(XboxButtonID::XBOXBTN_BACK))
			{
				g_theApp->Quit();
				return;
			}
		}
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_P))
	{
		m_clock.TogglePause();
	}

	m_clock.SetTimeDilation(g_theInput->IsKeyDown(KEYCODE_R) ? 0.1f : 1.0f);

	// keyboard quit
	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC))
	{
		g_theApp->Quit();
		return;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_T))
	{
		m_cameraPos = Transformation();
	}

	float cameraSpeed = 50.0f;

	if (g_theInput->IsKeyDown(KEYCODE_LSHIFT))
	{
		cameraSpeed *= 10.0f;
	}
	if (g_theInput->IsKeyDown(KEYCODE_LCTRL))
	{
		cameraSpeed *= .1f;
	}

	unsigned int hlBone = m_highlightBone;

	if (g_theInput->WasKeyJustPressed(KEYCODE_F))
	{
		hlBone += (int)m_mesh->m_skeleton.size() - 1;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_G))
	{
		hlBone++;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_H))
	{
		m_ikHead = !m_ikHead;
	}

	m_highlightBone = (BoneId)(hlBone % (int)m_mesh->m_skeleton.size());

	float deltaSeconds = (float)m_clock.GetDeltaTime();

	Vec3 iBasis;
	Vec3 jBasis;
	Vec3 kBasis;

	deltaSeconds *= 2.5f;

	cameraSpeed *= deltaSeconds / (float)m_clock.GetTimeDilation();

	constexpr float rotateSpeed = 0.25f;

	EulerAngles(m_cameraPos.m_orientation.m_yawDegrees, 0.0f, 0.0f).GetVectors_XFwd_YLeft_ZUp(iBasis, jBasis, kBasis);

	m_cameraPos.m_orientation.m_yawDegrees -= g_theInput->GetMouseClientDelta().x * rotateSpeed;
	m_cameraPos.m_orientation.m_pitchDegrees += g_theInput->GetMouseClientDelta().y * rotateSpeed;

	m_cameraPos.m_orientation.m_pitchDegrees = Clamp(m_cameraPos.m_orientation.m_pitchDegrees, -85.0f, 85.0f);

	if (g_theInput->IsKeyDown(KEYCODE_W))
	{
		m_cameraPos.m_position += iBasis * cameraSpeed;
	}
	if (g_theInput->IsKeyDown(KEYCODE_S))
	{
		m_cameraPos.m_position -= iBasis * cameraSpeed;
	}
	if (g_theInput->IsKeyDown(KEYCODE_A))
	{
		m_cameraPos.m_position += jBasis * cameraSpeed;
	}
	if (g_theInput->IsKeyDown(KEYCODE_D))
	{
		m_cameraPos.m_position -= jBasis * cameraSpeed;
	}
	if (g_theInput->IsKeyDown(KEYCODE_Q))
	{
		m_cameraPos.m_position.z -= cameraSpeed;
	}
	if (g_theInput->IsKeyDown(KEYCODE_E))
	{
		m_cameraPos.m_position.z += cameraSpeed;
	}

	cameraSpeed *= 0.1f;
	if (g_theInput->IsKeyDown(KEYCODE_I))
	{
		m_effector.x += cameraSpeed;
	}
	if (g_theInput->IsKeyDown(KEYCODE_K))
	{
		m_effector.x -= cameraSpeed;
	}
	if (g_theInput->IsKeyDown(KEYCODE_J))
	{
		m_effector.y += cameraSpeed;
	}
	if (g_theInput->IsKeyDown(KEYCODE_L))
	{
		m_effector.y -= cameraSpeed;
	}
	if (g_theInput->IsKeyDown(KEYCODE_U))
	{
		m_effector.z -= cameraSpeed;
	}
	if (g_theInput->IsKeyDown(KEYCODE_O))
	{
		m_effector.z += cameraSpeed;
	}

	// HandleMenuCursor();
	// HandleMenuSelection();

	g_theInput->SetMouseMode(true, true, true);
}

void SceneSkelAnim::LoadModel(const char* name)
{
	auto& layout = g_SkeletalShaderLayout;

	AssimpRes aiRes(Stringf("Data/Models/%s.FBX", name).c_str());
	aiRes.SetSpaceConventions(Mat4x4(Vec3(1, 0, 0), Vec3(0, 1, 0), Vec3(0, 0, 1), Vec3::ZERO)); // no conversion

	// load mesh
	{
		delete m_mesh;
		delete m_pose;

		auto meshes = aiRes.LoadMesh();
		ASSERT_OR_DIE(meshes.size(), "No mesh in model file!");

		m_mesh = meshes[0];
		m_pose = new Pose(m_mesh->m_skeleton.GetPose());

		for (auto& mesh : meshes)
		{
			if (mesh != m_mesh)
				delete mesh;
		}
	}
	
	if (TEST_MODEL_SERIALIZATION)
	{
		ByteBuffer buffer;
		uint64_t endian = 0x01020304CAFEBADEULL;
		buffer.Write(endian); // endianness identifier
		m_mesh->WriteBytes(&buffer);
		FileWriteFromBuffer(buffer, Stringf("Content/Cooked/Assets/SKEL_%s.asset", m_mesh->m_name.c_str()));
		buffer.ResetRead();

		auto* nmesh = new SkeletalMesh();
		buffer.Read(endian);
		nmesh->ReadBytes(&buffer);

		delete m_mesh;
		delete m_pose;
		m_mesh = nmesh;
		m_pose = new Pose(m_mesh->m_skeleton.GetPose());
	}

	// load mesh into GPU
	{
		delete m_ibo;
		for (auto& vbo : m_vbos)
			delete vbo;
		m_vbos.clear();

		auto& mesh = m_mesh;
		auto& vboList = m_vbos;
		VertexBuffer* vbo;

		// vbo position
		vbo = g_theRenderer->CreateVertexBuffer(mesh->m_vertices.size() * layout[0].GetVertexStride(), &layout[0]);
		g_theRenderer->CopyCPUToGPU(mesh->m_vertices.data(), mesh->m_vertices.size() * layout[0].GetVertexStride(), vbo);
		vboList.push_back(vbo);

		// vbo normal
		vbo = g_theRenderer->CreateVertexBuffer(mesh->m_normals.size() * layout[1].GetVertexStride(), &layout[1]);
		g_theRenderer->CopyCPUToGPU(mesh->m_normals.data(), mesh->m_normals.size() * layout[1].GetVertexStride(), vbo);
		vboList.push_back(vbo);

		// vbo color
		std::vector<Rgba8> colors;
		colors.resize(mesh->m_vertices.size());
		vbo = g_theRenderer->CreateVertexBuffer(colors.size() * layout[2].GetVertexStride(), &layout[2]);
		g_theRenderer->CopyCPUToGPU(colors.data(), colors.size() * layout[2].GetVertexStride(), vbo);
		vboList.push_back(vbo);

		// vbo texcoords
		vbo = g_theRenderer->CreateVertexBuffer(mesh->m_uvs[0].size() * layout[3].GetVertexStride(), &layout[3]);
		g_theRenderer->CopyCPUToGPU(mesh->m_uvs[0].data(), mesh->m_uvs[0].size() * layout[3].GetVertexStride(), vbo);
		vboList.push_back(vbo);

		// vbo boneIds
		vbo = g_theRenderer->CreateVertexBuffer(mesh->m_boneIndices.size() * layout[4].GetVertexStride(), &layout[4]);
		g_theRenderer->CopyCPUToGPU(mesh->m_boneIndices.data(), mesh->m_boneIndices.size() * layout[4].GetVertexStride(), vbo);
		vboList.push_back(vbo);

		// vbo boneWeights
		vbo = g_theRenderer->CreateVertexBuffer(mesh->m_boneWeights.size() * layout[5].GetVertexStride(), &layout[5]);
		g_theRenderer->CopyCPUToGPU(mesh->m_boneWeights.data(), mesh->m_boneWeights.size() * layout[5].GetVertexStride(), vbo);
		vboList.push_back(vbo);

		// ibo
		m_ibo = g_theRenderer->CreateIndexBuffer(sizeof(int) * mesh->m_indices.size());
		g_theRenderer->CopyCPUToGPU(mesh->m_indices.data(), sizeof(int) * mesh->m_indices.size(), m_ibo);
	}
}

void SceneSkelAnim::LoadAnimation(const char* name)
{
	AssimpRes aiRes(Stringf("Data/Models/%s.FBX", name).c_str());
	aiRes.SetSpaceConventions(Mat4x4(Vec3(1, 0, 0), Vec3(0, 1, 0), Vec3(0, 0, 1), Vec3::ZERO)); // no conversion

	// load mesh
	{
		delete m_animation;

		auto animations = aiRes.LoadAnimation(m_mesh->m_skeleton);
		ASSERT_OR_DIE(animations.size(), "No animation in model file!");
		m_animation = animations[0];

		for (auto& anim : animations)
		{
			if (anim != m_animation)
				delete anim;
		}
	}

	if (TEST_MODEL_SERIALIZATION)
	{
		ByteBuffer buffer;
		uint64_t endian = 0x01020304CAFEBADEULL;
		buffer.Write(endian); // endianness identifier
		m_animation->WriteBytes(&buffer);
		FileWriteFromBuffer(buffer, Stringf("Content/Cooked/Assets/ANIM_%s.asset", m_animation->m_name.c_str()));
		buffer.ResetRead();

		Animation* nanim = new CurveAnimation();
		buffer.Read(endian);
		nanim->ReadBytes(&buffer);

		delete m_animation;
		m_animation = nanim;
	}

	// load animation

	if (TEST_CONV_ANIM_TO_FRAMES)
	{
		FrameAnimation* fanim = new FrameAnimation();
		fanim->m_tps = 60.f;
		fanim->BakeFrom(*m_animation, m_mesh->m_skeleton.GetPose());
		fanim->m_name = m_animation->m_name + "_baked_60fps";
		delete m_animation;
		m_animation = fanim;
	}

}

