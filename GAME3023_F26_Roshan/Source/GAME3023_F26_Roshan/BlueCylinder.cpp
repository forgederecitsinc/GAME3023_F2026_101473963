// Fill out your copyright notice in the Description page of Project Settings.


#include "BlueCylinder.h"

// Sets default values
ABlueCylinder::ABlueCylinder()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ABlueCylinder::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ABlueCylinder::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

