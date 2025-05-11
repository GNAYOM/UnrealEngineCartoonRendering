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
bool FToonPassMeshProcessor::Process(const FMeshBatch& MeshBatch
	, uint64 BatchElementMask
	, int32 StaticMeshID
	, const FPrimitiveSceneProxy* PrimitiveSceneProxy
	, const FMaterialRenderProxy& MaterialRenderProxy
	, const FMaterial& RESTRICT MaterialResource
	, ERasterizerFillMode MeshFillMode
	, ERasterizerCullMode MeshCullMode)
{
	const FVertexFactory* VertexFactory = MeshBatch.VertexFactory;

	TMeshProcessorShaders<FToonPassVS,FToonPassPS> ToonPassShader;
	{
		FMaterialShaderTypes ShaderTypes;
		ShaderTypes.AddShaderType<FToonPassVS>();
		ShaderTypes.AddShaderType<FToonPassPS>();

		const FVertexFactoryType* VertexFactoryType = VertexFactory -> GetType();

		FMaterialShaders Shaders;
		if(!MaterialResource.TryGetShaders(ShaderTypes,VertexFactoryType,Shaders))
		{
			return false;
		}

		Shaders.TryGetVertexShader(ToonPassShader.VertexShader);
		Shaders.TryGetPixelShader(ToonPassShader.PixelShader);	
	}
	
	FMeshMaterialShaderElementData ShaderElementData;
	ShaderElementData.InitializeMeshMaterialData(ViewIfDynamicMeshCommand,PrimitiveSceneProxy
		,MeshBatch,StaticMeshID,false);

	const FMeshDrawCommandSortKey SortKey = CalculateMeshStaticSortKey(ToonPassShader.VertexShader
		,ToonPassShader.PixelShader);
	PassDrawRenderState.SetDepthStencilState(TStaticDepthStencilState<false,CF_DepthNearOrEqual>().GetRHI());

	FMeshPassProcessorRenderState DrawRenderState(PassDrawRenderState);

	BuildMeshDrawCommands(MeshBatch
		,BatchElementMask
		,PrimitiveSceneProxy
		,MaterialRenderProxy
		,MaterialResource
		,DrawRenderState
		,ToonPassShader
		,MeshFillMode
		,MeshCullMode
		,SortKey
		,EMeshPassFeatures::Default
		,ShaderElementData);
	return true;
}

void SetupToonPassState(FMeshPassProcessorRenderState& DrawRenderState)
{
	DrawRenderState.SetDepthStencilState(TStaticDepthStencilState<false,CF_DepthNearOrEqual>::GetRHI());
}

FMeshPassProcessor* CreateToonPassProcessor(ERHIFeatureLevel::Type FeatureLevel
	,const FScene* Scene
	,const FSceneView* InViewIfDynamicMeshCommand
	,FMeshPassDrawListContext* InDrawListContext)
{
	FMeshPassProcessorRenderState ToonPassState;
	SetupToonPassState(ToonPassState);
	return new FToonPassMeshProcessor(Scene,FeatureLevel
		,InViewIfDynamicMeshCommand,ToonPassState,InDrawListContext);
}

FRegisterPassProcessorCreateFunction RegisterToonPass(&CreateToonPassProcessor
	,EShadingPath::Deferred,EMeshPass::ToonPass
	,EMeshPassFlags::CachedMeshCommands | EMeshPassFlags::MainView);

DECLARE_CYCLE_STAT(TEXT("ToonPass"),STAT_CLP_ToonPass,STATGROUP_ParallelCommandListMarkers);

BEGIN_SHADER_PARAMETER_STRUCT(FToonMeshPassParameters,)
	SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters,View)
	SHADER_PARAMETER_STRUCT_INCLUDE(FInstanceCullingDrawParams,InstanceCullingDrawParams)
	RENDER_TARGET_BINDING_SLOTS()
END_SHADER_PARAMETER_STRUCT()

FToonMeshPassParameters* GetToonPassParameters(FRDGBuilder& GraphBuilder
	,const FViewInfo& View
	,FSceneTextures& SceneTextures )
{
	FToonMeshPassParameters* PassParameters
	 = GraphBuilder.AllocParameters<FToonMeshPassParameters>();
	PassParameters -> View = View.ViewUniformBuffer;

	PassParameters -> RenderTargets[0] = FRenderTargetBinding(SceneTextures.Color.Target
		, ERenderTargetLoadAction::ELoad);
	return PassParameters;
}

void FDeferredShadingSceneRenderer::RenderToonPass(FRDGBuilder& GraphBuilder
	, FSceneTextures& SceneTextures)
{
	RDG_EVENT_SCOPE(GraphBuilder,"ToonPass");
	RDG_CSV_STAT_EXCLUSIVE_SCOPE(GraphBuilder, RenderToonPass);

	SCOPED_NAMED_EVENT(FDeferredShadingSceneRenderer_RenderToonPass,FColor::Emerald);
	for(int32 ViewIndex = 0;ViewIndex < Views.Num(); ++ViewIndex)
	{
		FViewInfo& View = Views[ViewIndex];
		RDG_GPU_MASK_SCOPE(GraphBuilder,View.GPUMask);
		RDG_EVENT_SCOPE_CONDITIONAL(GraphBuilder,Views.Num() > 1,"View%d",ViewIndex);

		const bool bShouldRenderView = View.ShouldRenderView();
		if(bShouldRenderView)
		{
			FToonMeshPassParameters* PassParameters
				= GetToonPassParameters(GraphBuilder,View,SceneTextures);

			View.ParallelMeshDrawCommandPasses[EMeshPass::ToonPass]
				.BuildRenderingCommands(GraphBuilder,Scene->GPUScene,PassParameters->InstanceCullingDrawParams);

			GraphBuilder.AddPass(RDG_EVENT_NAME("ToonPass")
				,PassParameters
				,ERDGPassFlags::Raster | ERDGPassFlags::SkipRenderPass
				,[this,&View,PassParameters](const FRDGPass* InPass
					,FRHICommandListImmediate& RHICmdList)
					{
						FRDGParallelCommandListSet ParallelCommandListSet(InPass,RHICmdList
							,GET_STATID(STAT_CLP_ToonPass),View,FParallelCommandListBindings(PassParameters));
						ParallelCommandListSet.SetHighPriority();
						View.ParallelMeshDrawCommandPasses[EMeshPass::ToonPass].DispatchDraw(&ParallelCommandListSet
							,RHICmdList,&PassParameters->InstanceCullingDrawParams);
					}
				);
		}
	}
}

