#include "Editor/TSWeaponTrailLibrary.h"
#if WITH_EDITOR
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraScript.h"
#include "NiagaraSystem.h"
#include "NiagaraDataInterfaceColorCurve.h"
#include "NiagaraDecalRendererProperties.h"
#include "NiagaraMeshRendererProperties.h"
#include "NiagaraRibbonRendererProperties.h"
#include "NiagaraSpriteRendererProperties.h"
#include "Materials/MaterialInterface.h"
#include "UObject/UObjectHash.h"
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

#if WITH_EDITOR
namespace
{
    // 채도가 이보다 낮으면 (흰색·회색) 색상을 돌리지 않음
    constexpr float MinSaturationToTint = 0.12f;

    // 색상만 TargetHue로 바꾼 색. 바뀌었으면 true
    bool RotateHue(FLinearColor& Color, float TargetHue)
    {
        // HDR(1보다 큰 값)이라 세기(가장 큰 채널)로 나눠 0~1에서 계산하고 다시 곱함
        const float Intensity = FMath::Max3(Color.R, Color.G, Color.B);
        if (Intensity <= KINDA_SMALL_NUMBER)
        {
            return false;
        }
        const FLinearColor Unit(Color.R / Intensity, Color.G / Intensity, Color.B / Intensity, Color.A);
        FLinearColor Hsv = Unit.LinearRGBToHSV();
        if (Hsv.G < MinSaturationToTint)
        {
            return false;
        }
        Hsv.R = TargetHue;
        const FLinearColor NewUnit = Hsv.HSVToLinearRGB();
        Color = FLinearColor(NewUnit.R * Intensity, NewUnit.G * Intensity, NewUnit.B * Intensity, Color.A);
        return true;
    }
}
#endif

FString UTSWeaponTrailLibrary::TintSystemHue(UObject* SystemAsset, float TargetHue, bool bApply)
{
#if WITH_EDITOR
    auto* System = Cast<UNiagaraSystem>(SystemAsset);
    if (!System) return TEXT("ERROR: Missing Niagara system");
    if (bApply) System->Modify();

    FString Report;
    int32 ColorEdits = 0;
    int32 CurveEdits = 0;

    // 1. 모듈 숫자로 들어 있는 색 (Initialize Particle의 Color, Scale Color 등)
    for (auto& Handle : System->GetEmitterHandles())
    {
        auto* Data = Handle.GetInstance().GetEmitterData();
        if (!Data) continue;
        TArray<UNiagaraScript*> Scripts;
        Data->GetScripts(Scripts, false);
        for (auto* Script : Scripts)
        {
            if (!Script) continue;
            TArray<FNiagaraVariable> Variables;
            Script->RapidIterationParameters.GetParameters(Variables);
            for (const auto& Variable : Variables)
            {
                if (Variable.GetType() != FNiagaraTypeDefinition::GetColorDef()) continue;
                FLinearColor Color = Script->RapidIterationParameters.GetParameterValue<FLinearColor>(Variable);
                const FLinearColor Before = Color;
                const bool bTinted = RotateHue(Color, TargetHue);
                Report += FString::Printf(TEXT("COLOR %s/%s = %s"), *Handle.GetName().ToString(), *Variable.GetName().ToString(), *Before.ToString());
                if (bApply && bTinted)
                {
                    Script->Modify();
                    Script->RapidIterationParameters.SetParameterValue<FLinearColor>(Color, Variable);
                    Report += TEXT(" -> ") + Color.ToString();
                    ++ColorEdits;
                }
                Report += TEXT("\n");
            }
        }
    }

    // 2. 색 커브 (Color From Curve 등). 시스템 안에 들어 있는 모든 색 커브 데이터 인터페이스
    TArray<UObject*> Inner;
    GetObjectsWithOuter(System, Inner, true);
    for (UObject* Object : Inner)
    {
        auto* Curve = Cast<UNiagaraDataInterfaceColorCurve>(Object);
        if (!Curve) continue;

        // 채널마다 키 시간이 다를 수 있어서, 모든 키 시간에서 색을 읽어 다시 만듦
        TSet<float> TimeSet;
        for (const FRichCurve* Channel : { &Curve->RedCurve, &Curve->GreenCurve, &Curve->BlueCurve })
        {
            for (const FRichCurveKey& Key : Channel->GetConstRefOfKeys())
            {
                TimeSet.Add(Key.Time);
            }
        }
        TArray<float> Times = TimeSet.Array();
        Times.Sort();

        TArray<FLinearColor> Colors;
        bool bAnyTinted = false;
        for (float Time : Times)
        {
            FLinearColor Color(Curve->RedCurve.Eval(Time), Curve->GreenCurve.Eval(Time), Curve->BlueCurve.Eval(Time), 1.f);
            Report += FString::Printf(TEXT("CURVE %s t=%.2f %s"), *Curve->GetPathName(System), Time, *Color.ToString());
            bAnyTinted |= RotateHue(Color, TargetHue);
            Report += TEXT(" -> ") + Color.ToString() + TEXT("\n");
            Colors.Add(Color);
        }

        if (bApply && bAnyTinted)
        {
            Curve->Modify();
            Curve->RedCurve.Reset();
            Curve->GreenCurve.Reset();
            Curve->BlueCurve.Reset();
            for (int32 Index = 0; Index < Times.Num(); ++Index)
            {
                Curve->RedCurve.AddKey(Times[Index], Colors[Index].R);
                Curve->GreenCurve.AddKey(Times[Index], Colors[Index].G);
                Curve->BlueCurve.AddKey(Times[Index], Colors[Index].B);
            }
            Curve->UpdateLUT();
            ++CurveEdits;
        }
    }

    if (bApply)
    {
        System->RequestCompile(false);
        System->WaitForCompilationComplete(true, false);
        System->MarkPackageDirty();
    }
    return Report + FString::Printf(TEXT("VALID=%d COLORS=%d CURVES=%d"), System->IsValid(), ColorEdits, CurveEdits);
#else
    return TEXT("ERROR: Editor only");
#endif
}

FString UTSWeaponTrailLibrary::SetEmittersLocalSpace(UObject* SystemAsset, const TArray<FString>& NameFilters, bool bApply)
{
#if WITH_EDITOR
    auto* System = Cast<UNiagaraSystem>(SystemAsset);
    if (!System) return TEXT("ERROR: Missing Niagara system");
    if (bApply) System->Modify();

    FString Report;
    int32 Edits = 0;
    for (auto& Handle : System->GetEmitterHandles())
    {
        auto* Data = Handle.GetInstance().GetEmitterData();
        if (!Data) continue;
        const FString Name = Handle.GetName().ToString();
        const bool bMatch = NameFilters.ContainsByPredicate([&Name](const FString& Filter) { return Name.Contains(Filter); });
        Report += FString::Printf(TEXT("EMITTER %s local=%d"), *Name, Data->bLocalSpace ? 1 : 0);
        if (bApply && bMatch && !Data->bLocalSpace)
        {
            Handle.GetInstance().Emitter->Modify();
            Data->bLocalSpace = true;
            Report += TEXT(" -> 1");
            ++Edits;
        }
        Report += TEXT("\n");
    }

    if (bApply && Edits > 0)
    {
        System->RequestCompile(false);
        System->WaitForCompilationComplete(true, false);
        System->MarkPackageDirty();
    }
    return Report + FString::Printf(TEXT("VALID=%d EDITS=%d"), System->IsValid(), Edits);
#else
    return TEXT("ERROR: Editor only");
#endif
}

FString UTSWeaponTrailLibrary::PrepareOrbEffect(UObject* SystemAsset, const TArray<FString>& KeepNames, float LifetimeCap)
{
#if WITH_EDITOR
    UNiagaraSystem* System = Cast<UNiagaraSystem>(SystemAsset);
    if (!System || LifetimeCap <= 0.f) return TEXT("ERROR: Invalid system or lifetime");
    System->Modify();
    int32 Enabled = 0;
    for (FNiagaraEmitterHandle& Handle : System->GetEmitterHandles())
    {
        const bool bKeep = KeepNames.Contains(Handle.GetName().ToString());
        Handle.SetIsEnabled(bKeep, *System, false);
        if (!bKeep) continue;
        ++Enabled;
        auto* Data = Handle.GetInstance().GetEmitterData();
        if (!Data) continue;
        TArray<UNiagaraScript*> Scripts;
        Data->GetScripts(Scripts, false);
        for (UNiagaraScript* Script : Scripts)
        {
            if (!Script) continue;
            TArray<FNiagaraVariable> Variables;
            Script->RapidIterationParameters.GetParameters(Variables);
            for (const FNiagaraVariable& Variable : Variables)
            {
                if (Variable.GetType() != FNiagaraTypeDefinition::GetFloatDef()
                    || !Variable.GetName().ToString().Contains(TEXT("Lifetime"))) continue;
                const float Value = Script->RapidIterationParameters.GetParameterValue<float>(Variable);
                if (Value > LifetimeCap)
                {
                    Script->Modify();
                    Script->RapidIterationParameters.SetParameterValue<float>(LifetimeCap, Variable);
                }
            }
        }
    }
    System->RequestCompile(false);
    System->WaitForCompilationComplete(true, false);
    System->MarkPackageDirty();
    return FString::Printf(TEXT("VALID=%d ENABLED=%d"), System->IsValid(), Enabled);
#else
    return TEXT("ERROR: Editor only");
#endif
}

// 렌더러 종류마다 재질 칸이 다름: 스프라이트·리본·데칼은 Material 하나, 메시는 덮어쓰기 목록(OverrideMaterials)
// 메시가 덮어쓰기를 안 쓰면 메시 자체 재질이라 시스템에서는 못 바꿈 → 목록에 "(메시 자체 재질)"로만 표시
FString UTSWeaponTrailLibrary::ReplaceSystemMaterials(
    UObject* SystemAsset,
    const TArray<UMaterialInterface*>& From,
    const TArray<UMaterialInterface*>& To,
    bool bApply)
{
#if WITH_EDITOR
    auto* System = Cast<UNiagaraSystem>(SystemAsset);
    if (!System) return TEXT("ERROR: Missing Niagara system");
    if (From.Num() != To.Num()) return TEXT("ERROR: From/To count mismatch");
    if (bApply) System->Modify();

    FString Report;
    int32 Edits = 0;

    // 재질 칸 하나를 보고, 바꿀 대상이면 바꿈. 바꿨으면 true
    auto SwapSlot = [&](UObject* Renderer, const FString& Label, TObjectPtr<UMaterialInterface>& Slot)
    {
        Report += FString::Printf(TEXT("MAT %s = %s"), *Label, Slot ? *Slot->GetName() : TEXT("None"));
        const int32 Index = From.IndexOfByKey(Slot.Get());
        if (bApply && Index != INDEX_NONE && To[Index])
        {
            Renderer->Modify();
            Slot = To[Index];
            Report += TEXT(" -> ") + Slot->GetName();
            ++Edits;
        }
        Report += TEXT("\n");
    };

    for (auto& Handle : System->GetEmitterHandles())
    {
        auto* Data = Handle.GetInstance().GetEmitterData();
        if (!Data) continue;

        const FString EmitterName = Handle.GetName().ToString();
        for (UNiagaraRendererProperties* Renderer : Data->GetRenderers())
        {
            if (!Renderer) continue;

            const int32 EditsBefore = Edits;
            const FString Label = EmitterName + TEXT("/") + Renderer->GetClass()->GetName();
            if (auto* Sprite = Cast<UNiagaraSpriteRendererProperties>(Renderer))
            {
                SwapSlot(Renderer, Label, Sprite->Material);
            }
            else if (auto* Ribbon = Cast<UNiagaraRibbonRendererProperties>(Renderer))
            {
                SwapSlot(Renderer, Label, Ribbon->Material);
            }
            else if (auto* Decal = Cast<UNiagaraDecalRendererProperties>(Renderer))
            {
                SwapSlot(Renderer, Label, Decal->Material);
            }
            else if (auto* Mesh = Cast<UNiagaraMeshRendererProperties>(Renderer))
            {
                if (!Mesh->bOverrideMaterials)
                {
                    Report += FString::Printf(TEXT("MAT %s = (메시 자체 재질)\n"), *Label);
                }
                for (FNiagaraMeshMaterialOverride& Override : Mesh->OverrideMaterials)
                {
                    SwapSlot(Renderer, Label, Override.ExplicitMat);
                }
            }

            // 바꾼 렌더러는 편집 알림 → 렌더러가 재질 캐시를 다시 만듦
            if (Edits != EditsBefore)
            {
                Renderer->PostEditChange();
            }
        }
    }

    if (bApply && Edits > 0)
    {
        System->RequestCompile(false);
        System->WaitForCompilationComplete(true, false);
        System->MarkPackageDirty();
    }
    return Report + FString::Printf(TEXT("VALID=%d EDITS=%d"), System->IsValid(), Edits);
#else
    return TEXT("ERROR: Editor only");
#endif
}
