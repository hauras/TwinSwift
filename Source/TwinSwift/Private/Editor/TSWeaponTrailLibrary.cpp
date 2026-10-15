#include "Editor/TSWeaponTrailLibrary.h"
#if WITH_EDITOR
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraScript.h"
#include "NiagaraSystem.h"
#endif

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
