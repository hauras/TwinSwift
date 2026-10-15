#include "Editor/TSWidgetEditLibrary.h"

#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Components/Overlay.h"
#include "Components/Widget.h"
#if WITH_EDITOR
#include "Kismet2/BlueprintEditorUtils.h"
#include "WidgetBlueprint.h"
#endif

#if WITH_EDITOR
namespace
{
	UWidgetTree* GetTree(UObject* WidgetBlueprint)
	{
		UWidgetBlueprint* WBP = Cast<UWidgetBlueprint>(WidgetBlueprint);
		return WBP ? WBP->WidgetTree.Get() : nullptr;
	}

	void DumpRecursive(UWidget* Widget, int32 Depth, FString& Out)
	{
		if (!Widget)
		{
			return;
		}
		Out += FString::ChrN(Depth * 2, TEXT(' '))
			+ FString::Printf(TEXT("%s [%s]\n"), *Widget->GetName(), *Widget->GetClass()->GetName());
		if (const UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
		{
			for (int32 Index = 0; Index < Panel->GetChildrenCount(); ++Index)
			{
				DumpRecursive(Panel->GetChildAt(Index), Depth + 1, Out);
			}
		}
	}
}
#endif

FString UTSWidgetEditLibrary::DumpWidgetTree(UObject* WidgetBlueprint)
{
#if WITH_EDITOR
	UWidgetTree* Tree = GetTree(WidgetBlueprint);
	if (!Tree)
	{
		return TEXT("ERROR: Not a widget blueprint");
	}
	FString Out;
	DumpRecursive(Tree->RootWidget, 0, Out);
	return Out;
#else
	return TEXT("ERROR: Editor only");
#endif
}

UWidget* UTSWidgetEditLibrary::FindWidget(UObject* WidgetBlueprint, FName WidgetName)
{
#if WITH_EDITOR
	UWidgetTree* Tree = GetTree(WidgetBlueprint);
	return Tree ? Tree->FindWidget(WidgetName) : nullptr;
#else
	return nullptr;
#endif
}

UWidget* UTSWidgetEditLibrary::AddChildWidget(UObject* WidgetBlueprint, FName ParentName, TSubclassOf<UWidget> WidgetClass, FName NewName)
{
#if WITH_EDITOR
	UWidgetTree* Tree = GetTree(WidgetBlueprint);
	if (!Tree || !WidgetClass)
	{
		return nullptr;
	}
	if (UWidget* Existing = Tree->FindWidget(NewName))
	{
		return Existing;
	}

	UPanelWidget* Parent = Cast<UPanelWidget>(Tree->FindWidget(ParentName));
	if (!Parent)
	{
		return nullptr;
	}

	WidgetBlueprint->Modify();
	UWidget* NewWidget = Tree->ConstructWidget<UWidget>(WidgetClass, NewName);
	// 코드가 이름으로 찾을 수 있게 변수로 (BindWidget)
	NewWidget->bIsVariable = true;
	Parent->AddChild(NewWidget);
	return NewWidget;
#else
	return nullptr;
#endif
}

UWidget* UTSWidgetEditLibrary::WrapWithOverlay(UObject* WidgetBlueprint, FName WidgetName, FName OverlayName)
{
#if WITH_EDITOR
	UWidgetTree* Tree = GetTree(WidgetBlueprint);
	if (!Tree)
	{
		return nullptr;
	}
	if (UWidget* Existing = Tree->FindWidget(OverlayName))
	{
		return Existing;
	}

	UWidget* Target = Tree->FindWidget(WidgetName);
	UPanelWidget* Parent = Target ? Target->GetParent() : nullptr;
	if (!Parent)
	{
		return nullptr;
	}

	WidgetBlueprint->Modify();
	UOverlay* Overlay = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), OverlayName);
	Overlay->bIsVariable = true;

	// 같은 칸(Slot)에 내용만 바꿔 끼움 → 위치·크기 등 배치 설정은 그대로 Overlay가 씀
	const int32 Index = Parent->GetChildIndex(Target);
	Parent->ReplaceChildAt(Index, Overlay);
	Overlay->AddChild(Target);
	return Overlay;
#else
	return nullptr;
#endif
}

void UTSWidgetEditLibrary::MarkModified(UObject* WidgetBlueprint)
{
#if WITH_EDITOR
	if (UBlueprint* Blueprint = Cast<UBlueprint>(WidgetBlueprint))
	{
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	}
#endif
}
