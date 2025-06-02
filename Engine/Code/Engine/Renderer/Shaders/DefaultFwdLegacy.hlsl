struct vs_input_t
{
    float3 localPosition : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

struct v2p_t
{
    float4 position : SV_Position;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

struct CameraConstants
{
    float4x4 ProjectionMatrix;
    float4x4 ViewMatrix;
};

struct ModelConstants
{
    float4x4 ModelMatrix;
    float4 ModelColor;
    float4 ModelPadding;
};


ConstantBuffer<CameraConstants> cameraCBuffers[] : register(b0, space0);
ConstantBuffer<ModelConstants> modelCBuffers[] : register(b0, space1);
cbuffer PerDrawConstants : register(b0, space2)
{
    uint CameraBufferInd;
    uint ModelBufferInd;
    uint TextureStart;
    uint Padding;
    uint3x4 ExtraPadding;
};

Texture2D textures[] : register(t0, space0);
SamplerState diffuseSampler : register(s0);

v2p_t VertexMain(vs_input_t input)
{
    ModelConstants modelConstants = modelCBuffers[ModelBufferInd];
    CameraConstants cameraConstants = cameraCBuffers[CameraBufferInd];
    
    v2p_t v2p;
    float4 position = float4(input.localPosition, 1);
    float4 modelTransform = mul(modelConstants.ModelMatrix, position);
    float4 modelToViewPos = mul(cameraConstants.ViewMatrix, modelTransform);
    v2p.position = mul(cameraConstants.ProjectionMatrix, modelToViewPos);

    v2p.color = input.color;
    v2p.uv = input.uv;
    return v2p;
}

float4 PixelMain(v2p_t input) : SV_Target0
{
    Texture2D diffuseTexture = textures[TextureStart];
    ModelConstants modelConstants = modelCBuffers[ModelBufferInd];
    
    float4 resultingColor = diffuseTexture.Sample(diffuseSampler, input.uv) * input.color * modelConstants.ModelColor;
    if (resultingColor.w == 0)
        discard;
    return resultingColor;
}


