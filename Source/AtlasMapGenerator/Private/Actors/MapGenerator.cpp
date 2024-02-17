// Copyrights Gaon.


#include "Actors/MapGenerator.h"

#include "Components/BoxComponent.h"
#include "Components/LineBatchComponent.h"

bool FNodeTriangle::IsSame(const FNodeTriangle& Tri1, const FNodeTriangle& Tri2)
{
	if (Tri1.A != Tri2.A && Tri1.A != Tri2.B && Tri1.A != Tri2.C)
	{
		return false;
	}

	if (Tri1.B != Tri2.A && Tri1.B != Tri2.B && Tri1.B != Tri2.C)
	{
		return false;
	}

	if (Tri1.C != Tri2.A && Tri1.C != Tri2.B && Tri1.C != Tri2.C)
	{
		return false;
	}

	return true;
}

bool FNodeTriangle::HasSameLine(const FNodeTriangle& Tri1, const FNodeTriangle& Tri2)
{
	if (Tri1.A == Tri2.A || Tri1.A == Tri2.B || Tri1.A == Tri2.C)
	{
		if (Tri1.B == Tri2.A || Tri1.B == Tri2.B || Tri1.B == Tri2.C)
		{
			return true;
		}

		if (Tri1.C == Tri2.A || Tri1.C == Tri2.B || Tri1.C == Tri2.C)
		{
			return true;
		}
	}

	if (Tri1.B == Tri2.A || Tri1.B == Tri2.B || Tri1.B == Tri2.C)
	{
		if (Tri1.C == Tri2.A || Tri1.C == Tri2.B || Tri1.C == Tri2.C)
		{
			return true;
		}
	}

	return false;
}

bool FNodeTriangle::HasPoint(const FNodeTriangle& Tri1, int32 Point)
{
	if (Point == Tri1.A || Point == Tri1.B || Point == Tri1.C)
	{
		return true;
	}

	return false;
}

bool FNodeTriangle::HasPoint(int32 Point) const
{
	return HasPoint(*this, Point);
}

FString FNodeTriangle::ToString() const
{
	return FString::Printf(TEXT("[%d, %d, %d]"), A, B, C);
}

bool FNodeLine::IsSame(const FNodeLine& Line1, const FNodeLine& Line2)
{
	if (Line1.A != Line2.A && Line1.A != Line2.B)
	{
		return false;
	}

	if (Line1.B != Line2.A && Line1.B != Line2.B)
	{
		return false;
	}

	return true;
}

bool FNodeLine::HasPoint(const FNodeLine& Line1, int32 Point)
{
	if (Point == Line1.A || Point == Line1.B)
	{
		return true;
	}

	return false;
}

bool FNodeLine::HasPoint(int32 Point) const
{
	return HasPoint(*this, Point);
}

AMapGenerator::AMapGenerator()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>("Scene Root");
	SetRootComponent(SceneRoot);

	PersonaBox = CreateDefaultSubobject<UBoxComponent>("Persona Box");
	PersonaBox->SetupAttachment(GetRootComponent());
	PersonaBox->SetHiddenInGame(true);
	PersonaBox->SetLineThickness(Editor_BoxThickness);
	PersonaBox->ShapeColor = FColor::Red;

	PersonaBox->SetSimulatePhysics(false);
	PersonaBox->SetLinearDamping(10.f);
	PersonaBox->SetConstraintMode(EDOFMode::SixDOF);
	PersonaBox->BodyInstance.bLockZTranslation = true;
	PersonaBox->BodyInstance.bLockXRotation = true;
	PersonaBox->BodyInstance.bLockYRotation = true;
	PersonaBox->BodyInstance.bLockZRotation = true;
	PersonaBox->BodyInstance.bAutoWeld = false;

	PersonaBox->SetCollisionObjectType(ECC_WorldStatic);
	PersonaBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PersonaBox->SetCollisionResponseToAllChannels(ECR_Block);
}

void AMapGenerator::SpawnRooms()
{
	SpawnRooms(Editor_RoomCount, Editor_EllipseSize, Editor_UnitSize, Editor_MinRoomSize, Editor_MaxRoomSize);
}

void AMapGenerator::ClearRooms()
{
	FlushPersistentDebugLines(GetWorld());
		
	for (UBoxComponent* BoxComponent : RoomContainer)
	{
		if (BoxComponent)
		{
			BoxComponent->DestroyComponent();
		}
	}

	RoomContainer.Empty(Editor_RoomCount);
	
	MainRoomArray.Empty();
	NodeTriangleArray.Empty();
	NodeLineArray.Empty();
	UnionRoot.Empty();
}

void AMapGenerator::BeginPlay()
{
	Super::BeginPlay();
	
	if (RoomContainer.Num() > 0)
	{
		ActivateConstraintTimer();
	}
}

void AMapGenerator::SpawnRooms(const int32 RoomCount, const FVector2D& EllipseSize, const int32 UnitSize, const int32 MinRoomSize, const int32 MaxRoomSize)
{
	ClearRooms();

	if (UnitSize <= 0 || RoomCount <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: \"UnitSize\" should be bigger than 0."), *FString(__FUNCTION__))
		return;
	}

	float WidthSum = 0.f;
	float HeightSum = 0.f;

	for (int32 i = 0; i < RoomCount; ++i)
	{
		UBoxComponent* Box = NewObject<UBoxComponent>(this);
		
		Box->RegisterComponent();
		Box->InitializeComponent();
		Box->AttachToComponent(GetRootComponent(),FAttachmentTransformRules::SnapToTargetIncludingScale);
		Box->SetHiddenInGame(false);
		Box->SetLineThickness(Editor_BoxThickness);
		Box->ShapeColor = PersonaBox->ShapeColor;
		Box->SetSimulatePhysics(true);
		Box->SetEnableGravity(false);
		Box->SetLinearDamping(PersonaBox->GetLinearDamping());
		Box->SetConstraintMode(EDOFMode::SixDOF);
		Box->BodyInstance.bLockZTranslation = true;
		Box->BodyInstance.bLockXRotation = true;
		Box->BodyInstance.bLockYRotation = true;
		Box->BodyInstance.bLockZRotation = true;
		Box->BodyInstance.bAutoWeld = false;

		Box->SetGenerateOverlapEvents(true);
		Box->SetCollisionObjectType(PersonaBox->GetCollisionObjectType());
		Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Box->SetCollisionResponseToAllChannels(PersonaBox->GetCollisionResponseToChannel(PersonaBox->GetCollisionObjectType()));

		FVector Position;
		do
		{
			Position = GetRandomPosition(EllipseSize, UnitSize);
		}
		while (!CheckPositionEmpty(Position));
		
		Box->SetRelativeLocation(Position);
		Box->SetBoxExtent(GetRandomRoomSize(MinRoomSize, MaxRoomSize, UnitSize));

		RoomContainer.Add(Box);

		WidthSum += Box->GetScaledBoxExtent().X;
		HeightSum += Box->GetScaledBoxExtent().Y;
	}

	AverageWidth = WidthSum / RoomCount;
	AverageHeight = HeightSum / RoomCount;
	
	ActivateConstraintTimer();
}

void AMapGenerator::ActivateConstraintTimer()
{
	if (!GetWorld())
	{
		return;
	}
	
	GetWorldTimerManager().ClearTimer(PositionConstraintTimer);
	GetWorldTimerManager().SetTimer(PositionConstraintTimer, FTimerDelegate::CreateLambda([this]()
	{
		for (UBoxComponent* Room : RoomContainer)
		{
			if (!Room)
			{
				return;
			}

			Room->SetLineThickness(Editor_BoxThickness);
			Room->ShapeColor = FColor::Yellow;
			JitterPositionRecursive(Room, Editor_RecursionCount);
				
			FVector2D XY{Room->GetRelativeLocation()};
			FloorByUnitSize(XY, Editor_UnitSize);
			Room->SetRelativeLocation(FVector(XY, 0.f));
		}

		for (UBoxComponent* Room : RoomContainer)
		{
			Room->SetCollisionResponseToChannel(Room->GetCollisionObjectType(), ECR_Overlap);
			TArray<FOverlapInfo> Infos = Room->GetOverlapInfos();
			if (Infos.Num() > 0)
			{
				for (const auto& Info : Infos)
				{
					UBoxComponent* OverlappingBox = Cast<UBoxComponent>(Info.OverlapInfo.GetComponent());
					const FVector& Direction = Room->GetRelativeLocation() - OverlappingBox->GetRelativeLocation();
					const FVector& ExtentSum = Room->GetScaledBoxExtent() + OverlappingBox->GetScaledBoxExtent();
					if (!FMath::IsNearlyEqual(ExtentSum.X * ExtentSum.X, Direction.X * Direction.X) && !FMath::IsNearlyEqual(ExtentSum.Y * ExtentSum.Y, Direction.Y * Direction.Y))
					{
						Room->SetCollisionResponseToChannel(Room->GetCollisionObjectType(), ECR_Block);
						return;
					}
				}
			}

			Room->SetLineThickness(Editor_BoxThickness * 2);
			Room->ShapeColor = FColor::Green;
		}

		UE_LOG(LogTemp, Warning, TEXT("Constraint Finished."))
		GetWorldTimerManager().ClearTimer(PositionConstraintTimer);

		for (UBoxComponent* Room : RoomContainer)
		{
			Room->SetSimulatePhysics(false);
			Room->SetCollisionResponseToChannel(Room->GetCollisionObjectType(), ECR_Ignore);
		}
		
		SelectMainRoom();
	}), Editor_TimerInterval, true);
}

void AMapGenerator::JitterPositionRecursive(UBoxComponent* Room, const int32 MaxRecursionCount)
{
	if (MaxRecursionCount < 0)
	{
		BoxToIgnoreJitterSet.Empty();
		return;
	}
	
	Room->SetCollisionResponseToChannel(Room->GetCollisionObjectType(), ECR_Overlap);
	TArray<FOverlapInfo> OverlapInfos = Room->GetOverlapInfos();
	
	if (OverlapInfos.Num() <= 0)
	{
		Room->SetCollisionResponseToChannel(Room->GetCollisionObjectType(), ECR_Block);
		BoxToIgnoreJitterSet.Empty();
		return;
	}
	
	UBoxComponent* OverlappingBox = nullptr;
	FVector Direction = FVector::ZeroVector;
	for (const FOverlapInfo& Info : OverlapInfos)
	{
		OverlappingBox = Cast<UBoxComponent>(Info.OverlapInfo.GetComponent());
		if (!BoxToIgnoreJitterSet.Contains(OverlappingBox))
		{
			Direction = Room->GetRelativeLocation() - OverlappingBox->GetRelativeLocation();
			const FVector& ExtentSum = Room->GetScaledBoxExtent() + OverlappingBox->GetScaledBoxExtent();
			if (FMath::IsNearlyEqual(ExtentSum.X * ExtentSum.X, Direction.X * Direction.X) || FMath::IsNearlyEqual(ExtentSum.Y * ExtentSum.Y, Direction.Y * Direction.Y))
			{
				OverlappingBox = nullptr;
				continue;
			}
				
			break;
		}
			
		OverlappingBox = nullptr;
	}

	if (!OverlappingBox)
	{
		Room->SetCollisionResponseToChannel(Room->GetCollisionObjectType(), ECR_Block);
		BoxToIgnoreJitterSet.Empty();
		return;
	}
		
	ChangeToRectangularDirection(Direction);

	Room->AddRelativeLocation(Direction * Editor_UnitSize * Editor_JitterAmount);
	Room->SetCollisionResponseToChannel(Room->GetCollisionObjectType(), ECR_Block);
		
	BoxToIgnoreJitterSet.Add(Room);
	JitterPositionRecursive(OverlappingBox, MaxRecursionCount - 1);
}

void AMapGenerator::SelectMainRoom()
{
	UE_LOG(LogTemp, Warning, TEXT("startig Main Room Selection"))
	
	for (UBoxComponent* Room : RoomContainer)
	{
		if (Room->GetScaledBoxExtent().X > AverageWidth * Editor_MainRoomSelectionThreshold && Room->GetScaledBoxExtent().Y > AverageHeight * Editor_MainRoomSelectionThreshold)
		{
			Room->ShapeColor = FColor::Cyan;
			MainRoomArray.Add(Room);
		}
		else
		{
			Room->SetVisibility(false);
		}
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Main Room Selection Finished."))
	CalculateDelaunayTriangulation();
}

void AMapGenerator::CalculateDelaunayTriangulation()
{
	UE_LOG(LogTemp, Warning, TEXT("Starting Triangulation."))

	for(int32 TriA = 0; TriA < MainRoomArray.Num(); ++TriA)
	{
		for(int32 TriB = TriA + 1; TriB < MainRoomArray.Num(); ++TriB)
		{
			for(int32 TriC = TriB + 1; TriC < MainRoomArray.Num(); ++TriC)
			{
				NodeTriangleArray.Add({TriA, TriB, TriC});
			}
		}
	}
	
	for (int32 NodeIndex = 0; NodeIndex < MainRoomArray.Num(); ++NodeIndex)
	{
		for (auto& NodeTriangle : NodeTriangleArray)
		{
			if (NodeTriangle.HasPoint(NodeIndex))
			{
				continue;
			}
			
			FVector PosAndRad;
			CalculateCircumscribedCircle(PosAndRad, NODE_LOCATION(MainRoomArray[NodeTriangle.A]), NODE_LOCATION(MainRoomArray[NodeTriangle.B]), NODE_LOCATION(MainRoomArray[NodeTriangle.C]));
			if (TRACE_INSIDE_CIRCLE(PosAndRad, NODE_LOCATION(MainRoomArray[NodeIndex])))
			{
				NodeTriangle.bShouldBeRemoved = true;
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Creating Triangle %s"), *NodeTriangle.ToString())
			}
		}

		NodeTriangleArray.RemoveAllSwap([](const FNodeTriangle& Tri){ return Tri.bShouldBeRemoved; });
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Created Triangles: %d"), NodeTriangleArray.Num())

	if (ULineBatchComponent* const LineBatcher = GetWorld()->PersistentLineBatcher)
	{
		LineBatcher->Flush();
				
		TArray<FBatchedLine> BatchedLines;
		for (const FNodeTriangle& Triangle : NodeTriangleArray)
		{
			BatchedLines.Add({NODE_LOCATION(MainRoomArray[Triangle.A]), NODE_LOCATION(MainRoomArray[Triangle.B]), FColor::Magenta, -1.f, 60.f, 0, 1});
			BatchedLines.Add({NODE_LOCATION(MainRoomArray[Triangle.B]), NODE_LOCATION(MainRoomArray[Triangle.C]), FColor::Magenta, -1.f, 60.f, 0, 1});
			BatchedLines.Add({NODE_LOCATION(MainRoomArray[Triangle.C]), NODE_LOCATION(MainRoomArray[Triangle.A]), FColor::Magenta, -1.f, 60.f, 0, 1});
		}
					
		LineBatcher->DrawLines(BatchedLines);
	}

	UE_LOG(LogTemp, Warning, TEXT("Triangulation Finished."))
	CalculateMST();
}

void AMapGenerator::CalculateCircumscribedCircle(FVector& Result, const FVector& A, const FVector& B, const FVector& C)
{
	/*
	 * A(a1, b1), B(a2, b2), C(a3, b3)
	 * 
	 * |(a2 - a1) (b2 - b1)| |x| = 0.5 |(a2^2 - a1^2 + b2^2 - b1^2)|
	 * |(a3 - a2) (b3 - b2)| |y|       |(a3^2 - a2^2 + b3^2 - b2^2)|
	 *
	 * m1 = (a2 - a1), n1 = (b2 - b1), o1 = (a2^2 - a1^2 + b2^2 - b1^2)
	 * m2 = (a3 - a2), n2 = (a3 - a2), o2 = (a3^2 - a2^2 + b3^2 - b2^2)
	 * 
	 * m1 * x + n1 * y = o1
	 * m2 * x + n2 * y = o2
	 */

	const float& m1 = B.X - A.X, n1 = B.Y - A.Y, o1 = B.X * B.X - A.X * A.X  + B.Y * B.Y - A.Y * A.Y;
	const float& m2 = C.X - B.X, n2 = C.Y - B.Y, o2 = C.X * C.X - B.X * B.X  + C.Y * C.Y - B.Y * B.Y;

	if (FMath::IsNearlyZero(m1))
	{
		Result.Y = o1 / n1;
		Result.X = (o2 - n2 * Result.Y) / m2;
		Result.Z = FVector::DistXY(Result, A);
		
		return;
	}

	if (FMath::IsNearlyZero(m2))
	{
		Result.Y = o2 / n2;
		Result.X = (o1 - n1 * Result.Y) / m1;
		Result.Z = FVector::DistXY(Result, A);
		
		return;
	}
	
	if (FMath::IsNearlyZero(n1))
	{
		Result.X = o1 / m1;
		Result.Y = (o2 - m2 * Result.X) / n2;
		Result.Z = FVector::DistXY(Result, A);

		return;
	}

	if (FMath::IsNearlyZero(n2))
	{
		Result.X = o2 / n2;
		Result.Y = (o1 - m1 * Result.X) / n1;
		Result.Z = FVector::DistXY(Result, A);
		
		return;
	}

	Result.X = 0.5f * (o1 * n2 - o2 * n1) / (m1 * n2 - m2 * n1); 
	Result.Y = 0.5f * (o1 * m2 - o2 * m1) / (n1 * m2 - n2 * m1);
	Result.Z = FVector::DistXY(Result, A);
}

void AMapGenerator::CalculateMST()
{
	UE_LOG(LogTemp, Warning, TEXT("Starting MST."))

	for (const auto& Triangle : NodeTriangleArray)
	{
		NodeLineArray.AddUnique({Triangle.A, Triangle.B});
		NodeLineArray.AddUnique({Triangle.B, Triangle.C});
		NodeLineArray.AddUnique({Triangle.C, Triangle.A});
	}

	for (auto& Line : NodeLineArray)
	{
		Line.Length = FVector::DistSquaredXY(NODE_LOCATION(MainRoomArray[Line.A]), NODE_LOCATION(MainRoomArray[Line.B]));

		UnionRoot.Add(Line.A, Line.A);
		UnionRoot.Add(Line.B, Line.B);
	}

	NodeLineArray.Sort([](const FNodeLine& Line1, const FNodeLine& Line2){ return Line1.Length < Line2.Length; });

	for (auto& Line : NodeLineArray)
	{
		const int32& UnionRootA = UnionFind(Line.A);
		const int32& UnionRootB = UnionFind(Line.B);

		if (UnionRootA == UnionRootB)
		{
			//UE_LOG(LogTemp, Warning, TEXT("Compare Node Root %d(U: %d) == %d(U: %d)"), Line.A, UnionRootA, Line.B, UnionRootB)
			Line.bShouldBeRemoved = true;

			continue;
		}
		
		if (UnionRootA < UnionRootB)
		{
			UnionRoot[UnionRootB] = UnionRootA;
			UE_LOG(LogTemp, Warning, TEXT("Change Node Union %d(U: %d) : %d(U: %d) => Union %d"), Line.A, UnionRootA, Line.B, UnionRootB, UnionRootA)
		}
		else
		{
			UnionRoot[UnionRootA] = UnionRootB;
			UE_LOG(LogTemp, Warning, TEXT("Change Node Union %d(U: %d) : %d(U: %d) => Union %d"), Line.A, UnionRootA, Line.B, UnionRootB, UnionRootB)
		}
	}

	// restore some paths in the percentage
	for (auto& Line : NodeLineArray)
	{
		if (Line.bShouldBeRemoved && FMath::SRand() < Editor_PathAnomalyPercentage)
		{
			Line.bShouldBeRemoved = false;
		}
	}

	if (ULineBatchComponent* const LineBatcher = GetWorld()->PersistentLineBatcher)
	{
		LineBatcher->Flush();
				
		TArray<FBatchedLine> BatchedLines;
		for (const FNodeLine& Line : NodeLineArray)
		{
			if (Line.bShouldBeRemoved)
			{
				BatchedLines.Add({NODE_LOCATION(MainRoomArray[Line.A]), NODE_LOCATION(MainRoomArray[Line.B]), FColor{16,16,16}, -1.f, 30.f, 0, 1});
			}
			else
			{
				BatchedLines.Add({NODE_LOCATION(MainRoomArray[Line.A]), NODE_LOCATION(MainRoomArray[Line.B]), FColor::Magenta, -1.f, 60.f, 0, 1});
			}
		}
					
		LineBatcher->DrawLines(BatchedLines);
	}

	NodeLineArray.RemoveAllSwap([](const FNodeLine& Line){ return Line.bShouldBeRemoved; });

	UE_LOG(LogTemp, Warning, TEXT("MST Finished."))
	FTimerHandle Timer;
	GetWorldTimerManager().SetTimer(Timer, this, &AMapGenerator::MakeOrthogonalPath, 1.f);
	//SelectSubRoom();
}

int32 AMapGenerator::UnionFind(const int32 NodeIndex)
{
	if (NodeIndex == UnionRoot[NodeIndex])
	{
		return NodeIndex;
	}

	return UnionRoot[NodeIndex] = UnionFind(UnionRoot[NodeIndex]);
}

void AMapGenerator::MakeOrthogonalPath()
{
	UE_LOG(LogTemp, Warning, TEXT("startig Sub Room Selection"))

	TArray<FBatchedLine> BatchedLines;
	for (const auto& Line : NodeLineArray)
	{
		const UBoxComponent* NodeA = MainRoomArray[Line.A];
		const UBoxComponent* NodeB = MainRoomArray[Line.B];
		const FVector& ExtentA = NodeA->GetScaledBoxExtent();
		const FVector& ExtentB = NodeB->GetScaledBoxExtent();
		const FVector& ExtentSum = ExtentA + ExtentB;
		const FVector& LineDirection = NODE_LOCATION(NodeB) - NODE_LOCATION(NodeA);

		const FVector& SubRoomExtent = ExtentSum - LineDirection.GetAbs();

		FVector RectangularLineDirection = LineDirection;
		ChangeToRectangularDirection(RectangularLineDirection);

		if (SubRoomExtent.X > Editor_UnitSize * 3)
		{
			// draw a vertical line
			FVector LineStart = NODE_LOCATION(NodeA) + FVector{RectangularLineDirection.X * SubRoomExtent.X * 0.5f, 0.f, 0.f};
			FVector LineEnd = LineStart + FVector{0.f, LineDirection.Y, 0.f};
			
			LineStart.Y += RectangularLineDirection.Y * ExtentA.Y;
			LineEnd.Y -= RectangularLineDirection.Y * ExtentB.Y;
			
			BatchedLines.Add({LineStart, LineEnd, FColor::Orange, -1.f, 60.f, 0, 1});
			continue;
		}

		if (SubRoomExtent.Y > Editor_UnitSize * 3)
		{
			// draw a horizontal line
			FVector LineStart = NODE_LOCATION(NodeA) + FVector{0.f, RectangularLineDirection.Y * SubRoomExtent.Y * 0.5f, 0.f};
			FVector LineEnd = LineStart + FVector{LineDirection.X, 0.f, 0.f};
			
			LineStart.X += RectangularLineDirection.X * ExtentA.X;
			LineEnd.X -= RectangularLineDirection.X * ExtentB.X;
			
			BatchedLines.Add({LineStart, LineEnd, FColor::Orange, -1.f, 60.f, 0, 1});
			continue;
		}

		if (FMath::SRand() > 0.5f)
		{
			
			// X -> Y
			FVector LineStart = NODE_LOCATION(NodeA);
			LineStart.X += RectangularLineDirection.X * ExtentA.X;

			const FVector& LineCorner = NODE_LOCATION(NodeA) + FVector{LineDirection.X, 0.f, 0.f};
			
			FVector LineEnd = NODE_LOCATION(NodeB);
			LineEnd.Y -= RectangularLineDirection.Y * ExtentB.Y;
			
			BatchedLines.Add({LineStart, LineCorner, FColor::Orange, -1.f, 60.f, 0, 1});
			BatchedLines.Add({LineCorner, LineEnd, FColor::Orange, -1.f, 60.f, 0, 1});
		}
		else
		{
			// Y -> X
			FVector LineStart = NODE_LOCATION(NodeA);
			LineStart.Y += RectangularLineDirection.Y * ExtentA.Y;

			const FVector& LineCorner = NODE_LOCATION(NodeA) + FVector{0.f, LineDirection.Y, 0.f};
			
			FVector LineEnd = NODE_LOCATION(NodeB);
			LineEnd.X -= RectangularLineDirection.X * ExtentB.X;
			
			BatchedLines.Add({LineStart, LineCorner, FColor::Orange, -1.f, 60.f, 0, 1});
			BatchedLines.Add({LineCorner, LineEnd, FColor::Orange, -1.f, 60.f, 0, 1});
		}
	}

	if (ULineBatchComponent* const LineBatcher = GetWorld()->PersistentLineBatcher)
	{
		LineBatcher->Flush();
		LineBatcher->DrawLines(BatchedLines);
	}

	
	UE_LOG(LogTemp, Warning, TEXT("Sub Room Selection Finished."))
}

void AMapGenerator::ChangeToRectangularDirection(FVector& Vector)
{
	if (Vector.X > 1E-08)
	{
		Vector.X = 1;
	}
	else if (-Vector.X > 1E-08)
	{
		Vector.X = -1;
	}
	else
	{
		Vector.X = 0;
	}

	if (Vector.Y > 1E-08)
	{
		Vector.Y = 1;
	}
	else if (-Vector.Y > 1E-08)
	{
		Vector.Y = -1;
	}
	else
	{
		Vector.Y = 0;
	}

	if (Vector.Z > 1E-08)
	{
		Vector.Z = 1;
	}
	else if (-Vector.Z > 1E-08)
	{
		Vector.Z = -1;
	}
	else
	{
		Vector.Z = 0;
	}
}

void AMapGenerator::FloorByUnitSize(FVector2D& Point, const int32 UnitSize)
{
	if (UnitSize <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: \"UnitSize\" should be bigger than 0."), *FString(__FUNCTION__))
		return;
	}
	
	Point.X = FMath::Floor((Point.X + UnitSize - 1) / UnitSize) * UnitSize;
	Point.Y = FMath::Floor((Point.Y + UnitSize - 1) / UnitSize) * UnitSize;
}

FVector AMapGenerator::GetRandomPosition(const FVector2D& EllipseSize, const int32 UnitSize)
{
	FVector2D Point = GetRandomPointInEllipse(EllipseSize.X, EllipseSize.Y);
	FloorByUnitSize(Point, UnitSize);

	return FVector(Point, 0.f);
}

FVector AMapGenerator::GetRandomRoomSize(const int32 Min, const int32 Max, const int32 UnitSize)
{
	float X = Min + FMath::SRand() * (Max - Min);
	float Y = Min + FMath::SRand() * (Max - Min);
	
	FVector2D Point{X, Y};
	FloorByUnitSize(Point, UnitSize);

	return FVector(Point, 100.f);
}

FVector2D AMapGenerator::GetRandomPointInEllipse(const float Width, const float Height)
{
	FVector2D Point = FMath::RandPointInCircle(1.f);

	Point.X = Point.X * Width;
	Point.Y = Point.Y * Height;
	
	return Point;
}

bool AMapGenerator::CheckPositionEmpty(const FVector& Position)
{
	for (const UBoxComponent* Room : RoomContainer)
	{
		if (Room->GetRelativeLocation().Equals(Position))
		{
			return false;
		}
	}

	return true;
}

