// Copyrights Gaon.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MapGenerator.generated.h"

class UBoxComponent;

USTRUCT(BlueprintType, Atomic)
struct FNodeTriangle
{
	GENERATED_BODY()
	
	FNodeTriangle(){}
	FNodeTriangle(int32 InA, int32 InB, int32 InC): A(InA), B(InB), C(InC) {}

	bool operator==(const FNodeTriangle& Triangle) const
	{
		return IsSame(*this, Triangle);
	}

	static bool IsSame(const FNodeTriangle& Tri1, const FNodeTriangle& Tri2);
	static bool HasSameLine(const FNodeTriangle& Tri1, const FNodeTriangle& Tri2);
	static bool HasPoint(const FNodeTriangle& Tri1, int32 Point);
	bool HasPoint(int32 Point) const;;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 A = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 B = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 C = 0;

	bool bShouldBeRemoved = false;
};

UCLASS()
class ATLASMAPGENERATOR_API AMapGenerator : public AActor
{
	GENERATED_BODY()
	
public:
	AMapGenerator();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Generator", Exec)
	void SpawnRooms();
	
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Generator", Exec)
	void ClearRooms();

	UPROPERTY(EditInstanceOnly, Category = "Generator", DisplayName = "Box Thickness")
	float Editor_BoxThickness = 30.f;
	UPROPERTY(EditInstanceOnly, Category = "Generator", DisplayName = "Room Count")
	int32 Editor_RoomCount = 100;
	UPROPERTY(EditInstanceOnly, Category = "Generator", DisplayName = "Ellipse Size")
	FVector2D Editor_EllipseSize = FVector2D(8000.f, 8000.f);
	UPROPERTY(EditInstanceOnly, Category = "Generator", DisplayName = "Unit Size")
	int32 Editor_UnitSize = 200;
	UPROPERTY(EditInstanceOnly, Category = "Generator", DisplayName = "Min Room Size")
	int32 Editor_MinRoomSize = 400;
	UPROPERTY(EditInstanceOnly, Category = "Generator", DisplayName = "Max Room Size")
	int32 Editor_MaxRoomSize = 1000;
	UPROPERTY(EditInstanceOnly, Category = "Generator", DisplayName = "Timer Interval")
	float Editor_TimerInterval = 0.1f;
	UPROPERTY(EditInstanceOnly, Category = "Generator", DisplayName = "Jitter Amount")
	float Editor_JitterAmount = 1.f;
	UPROPERTY(EditInstanceOnly, Category = "Generator", DisplayName = "Recursion Count")
	int32 Editor_RecursionCount = 16;
	UPROPERTY(EditInstanceOnly, Category = "Generator", DisplayName = "Main Room Selection Threshold")
	float Editor_MainRoomSelectionThreshold = 1.1f;
	
protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere)
	USceneComponent* SceneRoot;

	UPROPERTY(EditAnywhere)
	UBoxComponent* PersonaBox;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Generator|container")
	TArray<UBoxComponent*> RoomContainer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generator|container")
	TArray<FNodeTriangle> NodeTriangleArray;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generator|container")
	TArray<FNodeTriangle> TrianglesToIgnore;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generator|container")
	TArray<UBoxComponent*> MainRoomArray;
private:
	void SpawnRooms(const int32 RoomCount, const FVector2D& EllipseSize, const int32 UnitSize, const int32 MinRoomSize, const int32 MaxRoomSize);
	
	void ActivateConstraintTimer();
	void JitterPositionRecursive(UBoxComponent* Room, const int32 MaxRecursionCount = 0);

	void SelectMainRoom();
	void CalculateDelaunayTriangulation();
	void CalculateCircumscribedCircle(FVector& Result, const FVector& A, const FVector& B, const FVector& C); ///@param Result .Z: Radius.
	
	void CalculateMST()

	float AverageWidth;
	float AverageHeight;

	FTimerHandle PositionConstraintTimer;

	UPROPERTY()
	TSet<UBoxComponent*> BoxToIgnoreJitterSet;

	UPROPERTY()
	UBoxComponent* PrevOverlappingBox_Internal;
	int32 OverlappingCount_Internal;

	static void ChangeToRectangularDirection(FVector& Vector);
	static void FloorByUnitSize(FVector2D& Point, const int32 UnitSize = 1);
	static FVector GetRandomPosition(const FVector2D& EllipseSize, const int32 UnitSize = 1);
	static FVector GetRandomRoomSize(const int32 Min, const int32 Max, const int32 UnitSize);
	static FVector2D GetRandomPointInEllipse(const float Width, const float Height);
	bool CheckPositionEmpty(const FVector& Position);
	
#define NODE_LOCATION(X) X->GetComponentLocation()
#define TRACE_INSIDE_CIRCLE(PosAndRad, V1) ((FVector::DistSquaredXY(PosAndRad, V1)) < (PosAndRad.Z * PosAndRad.Z))
};
