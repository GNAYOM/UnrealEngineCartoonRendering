//ToonPassRendering 修改
#pragma once

#include "DataDrivenShaderPlatformInfo.h"
#include "MeshPassProcessor.h"

#include "MeshMaterialShader.h"

class FToonPassMeshProcessor : public FMeshPassProcessor
{
public:
	FToonPassMeshProcessor(
			const FScene* Scene
			,ERHIFeatureLevel::Type InFeatureLevel
			,const FSceneView* InViewIfDynamicMeshCommand
			,const FMeshPassProcessorRenderState& InPassDrawRenderState
			,FMeshPassDrawListContext* InDrawListContext);

	virtual void AddMeshBatch(const FMeshBatch& RESTRICT MeshBatch
		, uint64 BatchElementMask
		, const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy
		, int32 StaticMeshId = -1) override final;
private:
	bool Process(
		const FMeshBatch& MeshBatch
		,uint64 BatchElementMask
		,int32 StaticMeshID
		,const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy
		,const FMaterialRenderProxy& RESTRICT MaterialRenderProxy
		,const FMaterial& RESTRICT MaterialResource
		,ERasterizerFillMode MeshFillMode
		,ERasterizerCullMode MeshCullMode);
	FMeshPassProcessorRenderState PassDrawRenderState;
};

class FToonPassVS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FToonPassVS,MeshMaterial);

public:
	FToonPassVS() = default;
	FToonPassVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer
		) : FMeshMaterialShader(Initializer)
	{
		
	}

	static void ModifyCompilationEnvironment(const FMaterialShaderPermutationParameters& Parameters
		,FShaderCompilerEnvironment& OutEnvironment)
	{}

	static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform,ERHIFeatureLevel::SM5) &&
			(Parameters.VertexFactoryType->GetFName() == FName(TEXT("FLocalVertexFactory")) ||
				Parameters.VertexFactoryType->GetFName() == FName(TEXT("TGPUSkinVertexFactoryDefault")));
	}

	void GetShaderBindings(
			const FScene* Scene
			,ERHIFeatureLevel::Type FeatureLevel
			,const FPrimitiveSceneProxy* PrimitiveSceneProxy
			,const FMaterialRenderProxy& MaterialRenderProxy
			,const FMaterial& Material
			,const FMeshPassProcessorRenderState& DrawRenderState
			,const FMeshMaterialShaderElementData& ShaderElementData
			,FMeshDrawSingleShaderBindings& ShaderBindings) const
	{
		FMeshMaterialShader::GetShaderBindings(Scene,FeatureLevel,PrimitiveSceneProxy,MaterialRenderProxy
				,Material,DrawRenderState,ShaderElementData,ShaderBindings);
	}
};

class FToonPassPS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FToonPassPS,MeshMaterial);
public:
	FToonPassPS() = default;
	FToonPassPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		:FMeshMaterialShader(Initializer)
	{
		InputColor.Bind(Initializer.ParameterMap,TEXT("InputColor"));
	}
	
	static void ModifyCompilationEnvironment(const FMaterialShaderPermutationParameters& Parameters
		,FShaderCompilerEnvironment& OutEnvironment)
	{}

	static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform,ERHIFeatureLevel::SM5) &&
			(Parameters.VertexFactoryType->GetFName() == FName(TEXT("FLocalVertexFactory"))||
				Parameters.VertexFactoryType->GetFName() == FName(TEXT("TGPUSkinVertexFactoryDefault")));
	}
	
	void GetShaderBindings(
		const FScene* Scene
		,ERHIFeatureLevel::Type FeatureLevel
		,const FPrimitiveSceneProxy* PrimitiveSceneProxy
		,const FMaterialRenderProxy& MaterialRenderProxy
		,const FMaterial& Material
		,const FMeshPassProcessorRenderState& DrawRenderState
		,const FMeshMaterialShaderElementData& ShaderElementData
		,FMeshDrawSingleShaderBindings& ShaderBindings) const//参数绑定着色器变量
	{
		FMeshMaterialShader::GetShaderBindings(Scene,FeatureLevel,PrimitiveSceneProxy,MaterialRenderProxy
				,Material,DrawRenderState,ShaderElementData,ShaderBindings);

		FVector3f Color(1.0,0.0,0.0);

		ShaderBindings.Add(InputColor,Color);
	}

	LAYOUT_FIELD(FShaderParameter,InputColor);
};