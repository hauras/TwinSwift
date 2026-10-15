#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TSWidgetEditLibrary.generated.h"

class UWidget;

/**
 * 에디터 전용 위젯 블루프린트(WBP) 편집 도구 (Python에서 unreal.TSWidgetEditLibrary로 부름)
 * Python으로는 WBP의 위젯 트리(디자이너 화면)를 읽거나 고칠 수 없어서 C++로 둠
 * 위젯을 추가·감싸기만 하고, 글자·색·정렬 같은 속성은 돌려받은 위젯에 Python에서 설정
 * 다 고친 뒤 MarkModified → 컴파일 → 저장
 */
UCLASS()
class TWINSWIFT_API UTSWidgetEditLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// 위젯 트리를 들여쓴 글로 (이름 [종류])
	UFUNCTION(BlueprintCallable, Category = "Editor|Widget")
	static FString DumpWidgetTree(UObject* WidgetBlueprint);

	// 이름으로 위젯 찾기 (없으면 null)
	UFUNCTION(BlueprintCallable, Category = "Editor|Widget")
	static UWidget* FindWidget(UObject* WidgetBlueprint, FName WidgetName);

	// 패널(Overlay·Canvas·Box 등) ParentName 안에 새 위젯 추가. 같은 이름이 이미 있으면 그걸 돌려줌 (다시 실행해도 안전)
	UFUNCTION(BlueprintCallable, Category = "Editor|Widget")
	static UWidget* AddChildWidget(UObject* WidgetBlueprint, FName ParentName, TSubclassOf<UWidget> WidgetClass, FName NewName);

	// WidgetName을 새 Overlay로 감쌈 (원래 자리의 배치 설정은 Overlay가 이어받음). 이미 OverlayName이 있으면 그걸 돌려줌
	UFUNCTION(BlueprintCallable, Category = "Editor|Widget")
	static UWidget* WrapWithOverlay(UObject* WidgetBlueprint, FName WidgetName, FName OverlayName);

	// 구조를 바꿨다고 알림 (컴파일 전에 한 번)
	UFUNCTION(BlueprintCallable, Category = "Editor|Widget")
	static void MarkModified(UObject* WidgetBlueprint);
};
