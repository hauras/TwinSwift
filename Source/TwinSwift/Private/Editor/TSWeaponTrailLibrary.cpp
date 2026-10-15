#include "Editor/TSWeaponTrailLibrary.h"
#if WITH_EDITOR
#include "NiagaraEditorUtilities.h"
#include "Editor.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraRibbonRendererProperties.h"
#include "NiagaraScript.h"
#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraSystem.h"
#include "NiagaraScriptSource.h"
#include "NiagaraGraph.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraNodeAssignment.h"
#include "ViewModels/Stack/NiagaraParameterHandle.h"
#include "ViewModels/Stack/NiagaraStackGraphUtilities.h"
#endif
FString UTSWeaponTrailLibrary::PrepareRibbon(UObject* SystemAsset, UObject* EmitterAsset, UMaterialInterface* Material)
{
#if WITH_EDITOR
    static bool bVerificationThrottleOverride = false;
    if (!Material) { bVerificationThrottleOverride = false; return TEXT("THROTTLE_OVERRIDE_OFF"); }
    static bool bDelegateAdded = false;
    if (GEditor && !bDelegateAdded)
    {
        GEditor->ShouldDisableCPUThrottlingDelegates.Add(UEditorEngine::FShouldDisableCPUThrottling::CreateLambda([](){ return bVerificationThrottleOverride; }));
        bDelegateAdded = true;
    }
    bVerificationThrottleOverride = true;
    auto* System = Cast<UNiagaraSystem>(SystemAsset);
    auto* Emitter = Cast<UNiagaraEmitter>(EmitterAsset);
    if (!System || !Emitter || !Material) return TEXT("ERROR: Missing asset");
    System->Modify();
    TSet<FGuid> OldIds;
    for (const auto& H : System->GetEmitterHandles()) OldIds.Add(H.GetId());
    System->RemoveEmitterHandlesById(OldIds);
    if (System->GetEmitterHandles().IsEmpty())
        FNiagaraEditorUtilities::AddEmitterToSystem(*System, *Emitter, Emitter->GetExposedVersion().VersionGuid);
    FString ParameterReport;
    int32 LifetimeEdits = 0;
    int32 WidthEdits = 0;
    for (auto& Handle : System->GetEmitterHandles())
    {
        auto* Data = Handle.GetInstance().GetEmitterData();
        if (!Data) continue;
        Data->bLocalSpace = false;
        TArray<UNiagaraRendererProperties*> OldRenderers = Data->GetRenderers();
        for (auto* R : OldRenderers) Handle.GetInstance().Emitter->RemoveRenderer(R, Handle.GetInstance().Version);
        auto* NewRibbon = NewObject<UNiagaraRibbonRendererProperties>(Handle.GetInstance().Emitter);
        NewRibbon->Material = Material;
        FNiagaraVariable WidthVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("User.TrailWidth"));
        System->GetExposedParameters().AddParameter(WidthVariable);
        System->GetExposedParameters().SetParameterValue<float>(110.f, WidthVariable);
        NewRibbon->RibbonWidthBinding.SetValue(TEXT("Particles.RibbonWidth"), Handle.GetInstance(), ENiagaraRendererSourceDataMode::Particles);
        NewRibbon->RibbonLinkOrderBinding.SetValue(TEXT("Particles.NormalizedAge"), Handle.GetInstance(), ENiagaraRendererSourceDataMode::Particles);
        Handle.GetInstance().Emitter->AddRenderer(NewRibbon, Handle.GetInstance().Version);
        for (auto* Renderer : Data->GetRenderers())
            if (auto* Ribbon = Cast<UNiagaraRibbonRendererProperties>(Renderer)) Ribbon->Material = Material;
        if (auto* Source = Cast<UNiagaraScriptSource>(Data->GraphSource))
        {
            for (UEdGraphNode* Node : Source->NodeGraph->Nodes)
            if (auto* Output = Cast<UNiagaraNodeOutput>(Node); Output && Output->GetUsage() == ENiagaraScriptUsage::ParticleSpawnScript)
            {
                TArray<FNiagaraVariable> Values = {
                    FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Particles.RibbonWidth")),
                    FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Particles.Lifetime")),
                    FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Particles.Velocity")) };
                FNiagaraStackGraphUtilities::AddParameterModuleToStack(Values, *Output, INDEX_NONE, { TEXT("110"), TEXT("0.15"), TEXT("0,0,0") });
                break;
            }
            for (UEdGraphNode* Node : Source->NodeGraph->Nodes)
            if (auto* Output = Cast<UNiagaraNodeOutput>(Node); Output && Output->GetUsage() == ENiagaraScriptUsage::ParticleUpdateScript)
            {
                FNiagaraStackGraphUtilities::AddParameterModuleToStack({ FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Particles.Velocity")) }, *Output, INDEX_NONE, { TEXT("0,0,0") });
                break;
            }
        }
        TArray<UNiagaraScript*> Scripts;
        Data->GetScripts(Scripts, false);
        for (auto* Script : Scripts)
        {
            if (!Script) continue;
            TArray<FNiagaraVariable> Variables;
            Script->RapidIterationParameters.GetParameters(Variables);
            for (const auto& Variable : Variables)
            {
                if (Variable.GetType() == FNiagaraTypeDefinition::GetVec3Def() && Variable.GetName().ToString().Contains(TEXT("Gravity")))
                    Script->RapidIterationParameters.SetParameterValue<FVector3f>(FVector3f::ZeroVector, Variable);
                if (Variable.GetType() != FNiagaraTypeDefinition::GetFloatDef()) continue;
                const FString Name = Variable.GetName().ToString();
                ParameterReport += Name + TEXT("=") + FString::SanitizeFloat(Script->RapidIterationParameters.GetParameterValue<float>(Variable)) + TEXT("\n");
                if (Name.Contains(TEXT("Lifetime")))
                {
                    Script->RapidIterationParameters.SetParameterValue<float>(0.15f, Variable);
                    ++LifetimeEdits;
                }
                else if (Name.Contains(TEXT("RibbonWidth")) || Name.Contains(TEXT("Ribbon Width")))
                {
                    Script->RapidIterationParameters.SetParameterValue<float>(110.f, Variable);
                    ++WidthEdits;
                }
                else if (Name.Contains(TEXT("Sphere Radius")) || Name.Contains(TEXT("Velocity Speed")))
                    Script->RapidIterationParameters.SetParameterValue<float>(0.f, Variable);
                else if (Name.Contains(TEXT("RandomRangeFloat")))
                    Script->RapidIterationParameters.SetParameterValue<float>(0.f, Variable);
                else if (Name.EndsWith(TEXT("SpawnRate.SpawnRate")))
                    Script->RapidIterationParameters.SetParameterValue<float>(120.f, Variable);
            }
        }
    }
    System->RequestCompile(false);
    System->WaitForCompilationComplete(true, false);
    System->MarkPackageDirty();
    return ParameterReport + FString::Printf(TEXT("VALID=%d LIFETIME=%d WIDTH=%d"),System->IsValid(),LifetimeEdits,WidthEdits);
#else
    return TEXT("ERROR: Editor only");
#endif
}

// SwordTrailVFX 16번의 재질과 소멸 연출은 유지하고 칼날 부착 기준만 보정한다.
FString UTSWeaponTrailLibrary::ConfigureBladeTrail(UObject* SystemAsset)
{
#if WITH_EDITOR
    auto* System = Cast<UNiagaraSystem>(SystemAsset);
    if (!System) return TEXT("ERROR: Missing Niagara system");
    System->Modify();
    const FNiagaraVariable Width(FNiagaraTypeDefinition::GetFloatDef(), TEXT("User.TSBladeWidth"));
    System->GetExposedParameters().AddParameter(Width);
    System->GetExposedParameters().SetParameterValue<float>(100.f, Width);
    int32 Ribbons = 0;
    int32 Offsets = 0;
    for (auto& Handle : System->GetEmitterHandles())
    {
        auto* Data = Handle.GetInstance().GetEmitterData();
        if (!Data) continue;
        Handle.GetInstance().Emitter->Modify();
        Data->bLocalSpace = false;
        for (auto* Renderer : Data->GetRenderers())
        {
            if (auto* Ribbon = Cast<UNiagaraRibbonRendererProperties>(Renderer))
            {
                Ribbon->Modify();
                Ribbon->FacingMode = ENiagaraRibbonFacingMode::CustomSideVector;
                Ribbon->RibbonWidthBinding.SetValue(
                    TEXT("Particles.RibbonWidth"), Handle.GetInstance(), ENiagaraRendererSourceDataMode::Particles);
                ++Ribbons;
            }
        }
        bool bHasRibbon = false;
        for (auto* Renderer : Data->GetRenderers())
            bHasRibbon |= Cast<UNiagaraRibbonRendererProperties>(Renderer) != nullptr;
        if (bHasRibbon)
        if (auto* Source = Cast<UNiagaraScriptSource>(Data->GraphSource))
        {
            // 폭을 입자 속성에 기록해야 리본 렌더러와 바운드 계산이 함께 사용한다.
            for (const auto Usage : { ENiagaraScriptUsage::ParticleSpawnScript, ENiagaraScriptUsage::ParticleUpdateScript })
            {
                const FString Marker = FString::Printf(TEXT("TSBladeWidth_%d"), static_cast<int32>(Usage));
                bool bExists = false;
                UNiagaraNodeOutput* Output = nullptr;
                for (UEdGraphNode* Node : Source->NodeGraph->Nodes)
                {
                    bExists |= Node->NodeComment == Marker;
                    if (auto* Candidate = Cast<UNiagaraNodeOutput>(Node); Candidate && Candidate->GetUsage() == Usage)
                        Output = Candidate;
                }
                if (!Output || bExists) continue;
                auto* Assignment = FNiagaraStackGraphUtilities::AddParameterModuleToStack(
                    { FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Particles.RibbonWidth")) },
                    *Output, INDEX_NONE, { TEXT("100") });
                Assignment->NodeComment = Marker;
                const auto Input = FNiagaraParameterHandle::CreateAliasedModuleParameterHandle(
                    FNiagaraParameterHandle(TEXT("Module.Particles.RibbonWidth")), Assignment);
                auto& Pin = FNiagaraStackGraphUtilities::GetOrCreateStackFunctionInputOverridePin(
                    *Assignment, Input, FNiagaraTypeDefinition::GetFloatDef(), FGuid(), FGuid());
                FNiagaraStackGraphUtilities::SetLinkedParameterValueForFunctionInput(
                    Pin, Width, TSet<FNiagaraVariableBase>{ Width });
            }
        }
        TArray<UNiagaraScript*> Scripts;
        Data->GetScripts(Scripts, false);
        for (auto* Script : Scripts)
        {
            if (!Script) continue;
            TArray<FNiagaraVariable> Variables;
            Script->RapidIterationParameters.GetParameters(Variables);
            for (const auto& Variable : Variables)
            {
                if (Variable.GetType() == FNiagaraTypeDefinition::GetVec3Def()
                    && Variable.GetName().ToString().EndsWith(TEXT("InitializeParticle.Position Offset")))
                {
                    Script->Modify();
                    Script->RapidIterationParameters.SetParameterValue<FVector3f>(FVector3f::ZeroVector, Variable);
                    ++Offsets;
                }
            }
        }
    }
    System->RequestCompile(false);
    System->WaitForCompilationComplete(true, false);
    System->MarkPackageDirty();
    return FString::Printf(TEXT("VALID=%d RIBBONS=%d OFFSETS=%d"), System->IsValid(), Ribbons, Offsets);
#else
    return TEXT("ERROR: Editor only");
#endif
}

// 팩 이펙트는 수명·생성 값이 모듈 안 숫자(RapidIterationParameters)로 들어 있음 → 이름으로 찾아 배율을 곱함
// 수명: 이름에 Lifetime이 들어간 값 × LifetimeScale
// 생성: SpawnRate × SpawnScale, Spawn Spacing(거리당 생성 간격) ÷ SpawnScale
FString UTSWeaponTrailLibrary::TuneTrailTiming(UObject* SystemAsset, float LifetimeScale, float SpawnScale)
{
#if WITH_EDITOR
    auto* System = Cast<UNiagaraSystem>(SystemAsset);
    if (!System) return TEXT("ERROR: Missing Niagara system");
    const bool bChange = !FMath::IsNearlyEqual(LifetimeScale, 1.f) || !FMath::IsNearlyEqual(SpawnScale, 1.f);
    if (bChange) System->Modify();

    FString Report;
    int32 LifetimeEdits = 0;
    int32 SpawnEdits = 0;
    for (auto& Handle : System->GetEmitterHandles())
    {
        auto* Data = Handle.GetInstance().GetEmitterData();
        if (!Data) continue;
        Report += TEXT("EMITTER ") + Handle.GetName().ToString() + TEXT("\n");

        TArray<UNiagaraScript*> Scripts;
        Data->GetScripts(Scripts, false);
        for (auto* Script : Scripts)
        {
            if (!Script) continue;
            TArray<FNiagaraVariable> Variables;
            Script->RapidIterationParameters.GetParameters(Variables);
            for (const auto& Variable : Variables)
            {
                if (Variable.GetType() != FNiagaraTypeDefinition::GetFloatDef()) continue;
                const FString Name = Variable.GetName().ToString();
                const float Value = Script->RapidIterationParameters.GetParameterValue<float>(Variable);
                float NewValue = Value;
                if (Name.Contains(TEXT("Lifetime")))
                {
                    NewValue = Value * LifetimeScale;
                    ++LifetimeEdits;
                }
                else if (Name.Contains(TEXT("SpawnRate")) || Name.Contains(TEXT("Spawn Rate")))
                {
                    NewValue = Value * SpawnScale;
                    ++SpawnEdits;
                }
                else if (Name.Contains(TEXT("Spawn Spacing")) && SpawnScale > 0.f)
                {
                    NewValue = Value / SpawnScale;
                    ++SpawnEdits;
                }
                Report += FString::Printf(TEXT("  %s = %g"), *Name, Value);
                if (bChange && NewValue != Value)
                {
                    Script->Modify();
                    Script->RapidIterationParameters.SetParameterValue<float>(NewValue, Variable);
                    Report += FString::Printf(TEXT(" -> %g"), NewValue);
                }
                Report += TEXT("\n");
            }
        }
    }
    if (bChange)
    {
        System->RequestCompile(false);
        System->WaitForCompilationComplete(true, false);
        System->MarkPackageDirty();
    }
    return Report + FString::Printf(TEXT("VALID=%d LIFETIME=%d SPAWN=%d"), System->IsValid(), LifetimeEdits, SpawnEdits);
#else
    return TEXT("ERROR: Editor only");
#endif
}
