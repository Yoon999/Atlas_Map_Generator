// Copyrights Gaon.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MapGenerator.generated.h"

struct FBatchedLine;
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
	bool HasPoint(int32 Point) const;
	FString ToString() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 A = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 B = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 C = 0;

	bool bShouldBeRemoved = false;
};

USTRUCT(BlueprintType, Atomic)
struct FNodeLine
{
	GENERATED_BODY()
	
	FNodeLine(){}
	FNodeLine(int32 InA, int32 InB): A(InA), B(InB) {}

	bool operator==(const FNodeLine& Line) const
	{
		return IsSame(*this, Line);
	}

	static bool IsSame(const FNodeLine& Line1, const FNodeLine& Line2);
	static bool HasPoint(const FNodeLine& Line1, int32 Point);
	bool HasPoint(int32 Point) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 A = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 B = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Length = 0.f;

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
	UPROPERTY(EditInstanceOnly, Category = "Generator", DisplayName = "Path Anomaly Percentage", meta = (ClampMin = "0", ClampMax = "1"))
	float Editor_PathAnomalyPercentage = 0.08f;

	UPROPERTY(EditAnywhere, Category = "Generator", DisplayName = "Floor Mesh")
	UStaticMesh* FloorMesh;
	UPROPERTY(EditAnywhere, Category = "Generator", DisplayName = "Floor Material")
	UMaterialInterface* FloorMaterial;
	
	UPROPERTY(EditAnywhere, Category = "Generator", DisplayName = "Wall Mesh")
	UStaticMesh* WallMesh;
	UPROPERTY(EditAnywhere, Category = "Generator", DisplayName = "Wall Material")
	UMaterialInterface* WallMaterial;
	
protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere)
	USceneComponent* SceneRoot;

	UPROPERTY(EditAnywhere)
	UBoxComponent* PersonaBox;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Generator|container")
	TArray<UBoxComponent*> RoomContainer;

private:
	void SpawnRooms(const int32 RoomCount, const FVector2D& EllipseSize, const int32 UnitSize, const int32 MinRoomSize, const int32 MaxRoomSize);
	
	void ActivateConstraintTimer();
	void JitterPositionRecursive(UBoxComponent* Room, const int32 MaxRecursionCount = 0);

	void SelectMainRoom();
	void CalculateDelaunayTriangulation();
	
	///@param Result .Z: Radius.
	void CalculateCircumscribedCircle(FVector& Result, const FVector& A, const FVector& B, const FVector& C); 
	
	void CalculateMST();
	int32 UnionFind(const int32 NodeIndex);

	void MakeOrthogonalPath();
	void DrawPath(const TArray<FHitResult>& HitResults, TArray<FBatchedLine>& BatchedLines, const FVector& PathStart, const FVector& PathEnd) const;

	void SelectSubRoom();

	float AverageWidth;
	float AverageHeight;

	FTimerHandle PositionConstraintTimer;

	UPROPERTY()
	TSet<UBoxComponent*> BoxToIgnoreJitterSet;

	UPROPERTY()
	TArray<UBoxComponent*> MainRoomArray;
	UPROPERTY()
	TArray<UBoxComponent*> SubRoomArray;
	
	TArray<FNodeTriangle> NodeTriangleArray;
	TArray<FNodeLine> NodeLineArray;
	TMap<int32, int32> UnionRoot;

	static void ChangeToRectangularDirection(FVector& Vector);
	static void FloorByUnitSize(FVector2D& Point, const int32 UnitSize = 1);
	static FVector GetRandomPosition(const FVector2D& EllipseSize, const int32 UnitSize = 1);
	static FVector GetRandomRoomSize(const int32 Min, const int32 Max, const int32 UnitSize);
	static FVector2D GetRandomPointInEllipse(const float Width, const float Height);
	bool RoomTraceMultiByChannel(TArray<FHitResult>& TraceResults, const FVector& Start, const FVector& End, ECollisionChannel TraceChannel, TArray<UBoxComponent*>& OutRoomContainer, TArray<UBoxComponent*>& RoomsToIgnore) const;
	bool CheckPositionEmpty(const FVector& Position);
	
#define NODE_LOCATION(Node) Node->GetComponentLocation()
#define NODE_EXTENT(Node) Node->GetScaledBoxExtent()
#define NODE_HALF_WIDTH(Node) Node->GetScaledBoxExtent().X
#define NODE_HALF_HEIGHT(Node) Node->GetScaledBoxExtent().Y
#define TRACE_INSIDE_CIRCLE(PosAndRad, V1) ((FVector::DistSquaredXY(PosAndRad, V1)) < (PosAndRad.Z * PosAndRad.Z))
};
