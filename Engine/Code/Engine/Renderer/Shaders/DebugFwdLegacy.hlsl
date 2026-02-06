struct vs_input_t
{
    float3 localPosition : POSITION;
    float4 color : COLOR;
};

struct v2p_t
{
    float4 position : SV_Position;
    float4 color : COLOR;
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
cbuffer PerDrawConstants : register(b2, space3)
{
    uint CameraBufferInd;
    uint ModelBufferInd;
    uint TextureStart;
    uint Padding;
    uint3x4 ExtraPadding;
};

SamplerState diffuseSampler : register(s0);

v2p_t VertexMain(vs_input_t input)
{
    ModelConstants modelConstants = modelCBuffers[ModelBufferInd];
    CameraConstants cameraConstants = cameraCBuffers[CameraBufferInd];
    
    v2p_t v2p;
    float4 position = float4(input.localPosition, 1.0f);
    float4 modelTransform = mul(modelConstants.ModelMatrix, position);
    float4 modelToViewPos = mul(cameraConstants.ViewMatrix, modelTransform);
    v2p.position = mul(cameraConstants.ProjectionMatrix, modelToViewPos);

    v2p.color = input.color;
    return v2p;
}

float4 PixelMain(v2p_t input) : SV_Target0
{
    ModelConstants modelConstants = modelCBuffers[ModelBufferInd];
    
    float4 resultingColor = input.color * modelConstants.ModelColor;
    if (resultingColor.w == 0)
        discard;
    return resultingColor;
}


