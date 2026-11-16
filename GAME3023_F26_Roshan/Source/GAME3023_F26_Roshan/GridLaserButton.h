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
 // Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
 // Called every frame
	virtual void Tick(float DeltaTime) override;

};
