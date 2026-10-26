

#include "Bullet/SBulletPatternData.h"

void USBulletPatternData::BuildShots(TArray<FSBulletShot>& OutShots) const
{
	OutShots.Reset();
	for (int32 EventIndex = 0; EventIndex < Events.Num(); ++EventIndex)
	{
		const FSBulletPatternEvent& Event = Events[EventIndex];
		for (int32 RepeatIndex = 0; RepeatIndex < Event.RepeatCount; ++RepeatIndex)
		{
			FSBulletShot& Shot = OutShots.AddDefaulted_GetRef();
			Shot.Time = Event.FireTime + RepeatIndex * Event.RepeatInterval;
			Shot.EventIndex = EventIndex;
			Shot.RepeatIndex = RepeatIndex;
		}
	}
	OutShots.StableSort([](const FSBulletShot& A, const FSBulletShot& B)
	{
		return A.Time < B.Time;
	});
}

float USBulletPatternData::GetLastShotTime() const
{
	float LastTime = 0.f;
	
	for (const FSBulletPatternEvent& Event : Events)
	{
		const float EventLastTime = Event.FireTime + (Event.RepeatCount - 1) * Event.RepeatInterval;
		LastTime = FMath::Max(LastTime, EventLastTime);
	}
	
	return LastTime; 
}

bool USBulletPatternData::HasRandomColor() const
{
	return Events.ContainsByPredicate([](const FSBulletPatternEvent& Event)
	{
		return Event.bRandomColor;
	});
}
