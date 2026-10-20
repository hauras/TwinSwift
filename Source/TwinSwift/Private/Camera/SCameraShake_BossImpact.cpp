#include "Camera/SCameraShake_BossImpact.h"

#include "Shakes/WaveOscillatorCameraShakePattern.h"

USCameraShake_BossImpact::USCameraShake_BossImpact(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 두 번 치는 공격이어도 흔들림이 쌓이지 않고 처음부터 다시 시작
	bSingleInstance = true;

	UWaveOscillatorCameraShakePattern* Pattern = CreateDefaultSubobject<UWaveOscillatorCameraShakePattern>(TEXT("Pattern"));

	// 0.35초. 맞는 순간 바로 세게(0.02초), 뒤는 길게 잦아듦(0.2초)
	Pattern->Duration = 0.35f;
	Pattern->BlendInTime = 0.02f;
	Pattern->BlendOutTime = 0.2f;

	// 위치: 위아래 14cm 초당 18번 (산산조각보다 2배 넘게 크고 느림), 좌우는 절반 이하
	Pattern->X.Amplitude = 0.f;
	Pattern->Y.Amplitude = 6.f;
	Pattern->Y.Frequency = 14.f;
	Pattern->Z.Amplitude = 14.f;
	Pattern->Z.Frequency = 18.f;

	// 회전·시야각은 흔들지 않음
	Pattern->Pitch.Amplitude = 0.f;
	Pattern->Yaw.Amplitude = 0.f;
	Pattern->Roll.Amplitude = 0.f;
	Pattern->FOV.Amplitude = 0.f;

	SetRootShakePattern(Pattern);
}
