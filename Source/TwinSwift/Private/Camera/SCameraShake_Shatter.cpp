#include "Camera/SCameraShake_Shatter.h"

#include "Shakes/WaveOscillatorCameraShakePattern.h"

USCameraShake_Shatter::USCameraShake_Shatter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 연달아 깨도 흔들림이 쌓이지 않고 처음부터 다시 시작
	bSingleInstance = true;

	UWaveOscillatorCameraShakePattern* Pattern = CreateDefaultSubobject<UWaveOscillatorCameraShakePattern>(TEXT("Pattern"));

	// 0.2초. 맞는 순간 바로 세게(0.02초), 뒤는 천천히 잦아듦(0.1초)
	Pattern->Duration = 0.2f;
	Pattern->BlendInTime = 0.02f;
	Pattern->BlendOutTime = 0.1f;

	// 위치: 위아래 6cm 초당 30번, 좌우는 절반 세기 (위아래만 있으면 기계적)
	Pattern->X.Amplitude = 0.f;
	Pattern->Y.Amplitude = 3.f;
	Pattern->Y.Frequency = 25.f;
	Pattern->Z.Amplitude = 6.f;
	Pattern->Z.Frequency = 30.f;

	// 회전·시야각은 흔들지 않음
	Pattern->Pitch.Amplitude = 0.f;
	Pattern->Yaw.Amplitude = 0.f;
	Pattern->Roll.Amplitude = 0.f;
	Pattern->FOV.Amplitude = 0.f;

	SetRootShakePattern(Pattern);
}
