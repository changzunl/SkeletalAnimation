#define ENGINE_SKEL_MAX_BONES 128
#define ENGINE_SKEL_MAX_BONE_WEIGHTS 4

struct vs_input_bone_lit
{
	float3 localPosition : POSITION;
	float3 localNormal   : NORMAL;
	float4 color         : COLOR;
	float2 uv            : TEXCOORD;
	int4   bones         : BONE_IDS;
	float4 weights       : BONE_WEIGHTS;
};

struct v2p_t_lit
{
	float4 position : SV_Position;
	float4 normal   : NORMAL;
	float4 color    : COLOR;
	float2 uv       : TEXCOORD;
};

Texture2D       diffuseTexture  : register(t0);
SamplerState    diffuseSampler  : register(s0);

cbuffer ModelConstants : register(b3)
{
	float4x4 ModelMatrix;
	float4   TintColor;
}

cbuffer CameraConstants : register(b2)
{
	float4x4 ProjectionMatrix;
	float4x4 ViewMatrix;
}

cbuffer LightingConstants : register(b1)
{
    float3 SunDirection;
    float  SunIntensity;
    float3 SunColor;
    float  AmbientIntensity;
}

cbuffer SkeletonConstants : register(b4)
{
	float4x4 Bone[ENGINE_SKEL_MAX_BONES];
}

float4 TransformLocalToSkinned(float4 local, int4 boneIds, float4 boneWeights)
{
	float4 skinned = { 0.0f, 0.0f, 0.0f, 0.0f };

	for (int i = 0; i < ENGINE_SKEL_MAX_BONE_WEIGHTS; i++)
	{
		if (boneWeights[i] == 0.0f)
			continue;
		float4 bonePartial = mul(Bone[boneIds[i]], local) * boneWeights[i];
		skinned += bonePartial;
	}

	return skinned;
}

v2p_t_lit VertexMain(vs_input_bone_lit input)
{
    float4 localPos = float4(input.localPosition, 1);
	float4 skinnedPos = TransformLocalToSkinned(localPos, input.bones, input.weights);
    float4 worldPos = mul(ModelMatrix, skinnedPos);
    float4 localNrm = float4(input.localNormal, 0);
	float4 skinnedNrm = TransformLocalToSkinned(localNrm, input.bones, input.weights);
	float4 worldNrm = mul(ModelMatrix, skinnedNrm);

	v2p_t_lit v2p;
	v2p.position = mul(ProjectionMatrix, mul(ViewMatrix, worldPos));
	v2p.normal   = worldNrm;
    v2p.color    = input.color;
	v2p.uv       = input.uv;
	
	return v2p;
}

float4 PixelMain(v2p_t_lit input) : SV_Target0
{
    float  ambient     = AmbientIntensity;
    float  directional = SunIntensity * saturate(dot(input.normal.xyz, -SunDirection));
    float4 light       = float4((ambient + directional).xxx, 1);

	float4 diffuse = diffuseTexture.Sample(diffuseSampler, input.uv);
	
	float4 color = TintColor * input.color * diffuse * light;

    return color;
}
