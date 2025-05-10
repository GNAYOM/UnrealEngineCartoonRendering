//ToonPassRendering 修改
#include "ToonPassRendering.h"

#include "ScenePrivate.h"
#include "MeshPassProcessor.h"
#include "SimpleMeshDrawCommandPass.h"
#include "StaticMeshBatch.h"
#include "DeferredShadingRenderer.h"
#include "Materials/MaterialRenderProxy.h"

IMPLEMENT_MATERIAL_SHADER_TYPE(, FToonPassVS
	, TEXT("/Engine/Shaders/Private/Toon/ToonPassShader.usf"), TEXT("MainVS"), SF_Vertex);
IMPLEMENT_MATERIAL_SHADER_TYPE(,FToonPassPS
	,TEXT("/Engine/Shaders/Private/Toon/ToonPassShader.usf"),TEXT("MainPS"),SF_Pixel);
//FToonVS FToonPS 绑定至 shader
FToonPassMeshProcessor::FToonPassMeshProcessor(const FScene* Scene
	, ERHIFeatureLevel::Type InFeatureLevel
	, const FSceneView* InViewIfDynamicMeshCommand
	, const FMeshPassProcessorRenderState& InPassDrawRenderState
	, FMeshPassDrawListContext* InDrawListContext)
: FMeshPassProcessor(Scene,Scene->GetFeatureLevel(),InViewIfDynamicMeshCommand,InDrawListContext)
,PassDrawRenderState(InPassDrawRenderState)//父类成员函数赋值
{
	if(PassDrawRenderState.GetDepthStencilState() == nullptr)
	{
		PassDrawRenderState.SetDepthStencilState(TStaticDepthStencilState<false
			,CF_DepthNearOrEqual>().GetRHI());
		
	}
	if (PassDrawRenderState.GetBlendState() == nullptr)
	{
		PassDrawRenderState.SetBlendState(TStaticBlendState<>().GetRHI());
	}
}

void FToonPassMeshProcessor::AddMeshBatch(const FMeshBatch& MeshBatch
	, uint64 BatchElementMask
	, const FPrimitiveSceneProxy* PrimitiveSceneProxy
	, int32 StaticMeshId)
{
	const FMaterialRenderProxy* MaterialRenderProxy = MeshBatch.MaterialRenderProxy;
	const FMaterial* Material = MaterialRenderProxy -> GetMaterialNoFallback(FeatureLevel);

	if (Material != nullptr && Material->GetRenderingThreadShaderMap())
	{
		const FMaterialShadingModelField ShadingModels = Material->GetShadingModels();
		if(ShadingModels.HasShadingModel(MSM_Toon) || ShadingModels.HasShadingModel(MSM_ToonFace))//为 shading models 开始渲染
		{
			const EBlendMode BlendMode = Material->GetBlendMode();

			bool bResult = true;
			if(BlendMode == BLEND_Opaque)
			{
				Process(MeshBatch
					,BatchElementMask
					,StaticMeshId
					,PrimitiveSceneProxy
					,*MaterialRenderProxy
					,*Material
					,FM_Solid
					,CM_CW);
			}
		}
	}
}
