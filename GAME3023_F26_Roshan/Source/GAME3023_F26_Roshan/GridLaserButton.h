#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridLaserButton.generated.h"

UCLASS()
class GAME3023_F26_ROSHAN_API AGridLaserButton : public AActor
{
	GENERATED_BODY()
	
public:	
	AGridLaserButton();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

};
