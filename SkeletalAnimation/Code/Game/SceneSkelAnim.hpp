#include "Scene.hpp"

#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Math/Transformation.hpp"

#include "Engine/Animation/Skeleton.hpp"

#include <vector>

class XboxController;
class SkeletalMesh;
class Animation;
class VertexBuffer;
class IndexBuffer;

class SceneSkelAnim : public Scene
{
public:
	SceneSkelAnim(Game* game);
	virtual ~SceneSkelAnim();

	virtual void Initialize() override;
	virtual void Shutdown() override;
	virtual void Update() override;
	virtual void UpdateCamera() override;
	virtual void RenderWorld() const override;
	virtual void RenderUI() const override;

	// model & animation
	void LoadModel(const char* name);
	void LoadAnimation(const char* name);

private:
	void RenderUILogoText() const;
	void HandleInput();

private:
	int         m_menuSelectionIdx                  = 0;
	bool        m_isQuitting                        = false;
	float       m_quitTimeCounter                   = 0.0f;
	XboxController* m_controller = nullptr;

	Transformation m_cameraPos;

	// skeletal mesh & animation
	SkeletalMesh* m_mesh = nullptr;
	Animation* m_animation = nullptr;
	mutable Pose* m_pose = nullptr;
	std::vector<VertexBuffer*> m_vbos;
	IndexBuffer* m_ibo = nullptr;

	BoneId m_highlightBone = 0;
	Vec3 m_effector;
	bool m_ikHead = true;
};

