// 

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "UnrealMathUtility.h"
#include "Containers/List.h"
#include "JCVTypes.h"
#include "JCVPolyIslandGenerator.generated.h"

struct FJCVPolyIslandParams
{
    FVector2D Size;
    FVector2D DisplacementRange;
    int32 SideCount = 3;
    int32 SubdivCount = 3;
    float SubdivLimit = .1f;
    float PolyScale = .985f;
    float PolyAngleOffset = 0.f;

    FJCVPolyIslandParams() = default;

    FJCVPolyIslandParams(
        const FVector2D& pSize,
        int32 pSideCount,
        int32 pSubdivCount,
        float pSubdivLimit,
        float pPolyScale,
        float pPolyAngleOffset,
        const FVector2D& pDisplacementRange
    ) {
        Set(pSize,
            pSideCount,
            pSubdivCount,
            pSubdivLimit,
            pPolyScale,
            pPolyAngleOffset,
            pDisplacementRange);
    }

    void Set(
        const FVector2D& pSize,
        int32 pSideCount,
        int32 pSubdivCount,
        float pSubdivLimit,
        float pPolyScale,
        float pPolyAngleOffset,
        const FVector2D& pDisplacementRange
    ) {
        Size = pSize;
        SideCount = pSideCount;
        SubdivCount = pSubdivCount;
        SubdivLimit = pSubdivLimit;
        PolyScale = pPolyScale;
        PolyAngleOffset = pPolyAngleOffset;
        DisplacementRange = pDisplacementRange;
    }

    FORCEINLINE bool IsValid() const
    {
        return SideCount >= 3
            && SubdivCount >= 0
            && SubdivLimit > 0.f
            && Size.GetMin() > KINDA_SMALL_NUMBER;
    }
};

USTRUCT(BlueprintType, meta = (DisplayName = "JCV Poly Island Params"))
struct FJCVPolyIslandParamsBP
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 RandomSeed = 1337;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FVector2D Size;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FVector2D DisplacementRange;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 SideCount = 3;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 SubdivCount = 3;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float SubdivLimit = .1f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float PolyScale = .985f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float PolyAngleOffset = 0.f;
};

class FJCVPolyIslandGenerator
{
    struct FRoughV2D
    {
        FVector2D Pos;
        float Balance;
        float MaxOffset;

        FORCEINLINE FRoughV2D() = default;

        FORCEINLINE FRoughV2D(float x, float y, float b, float m)
            : Pos(x, y)
            , Balance(b)
            , MaxOffset(m)
        {
        }

        FORCEINLINE FRoughV2D(float x, float y, FRandomStream& rand, float rmin, float rmax)
            : Pos(x, y)
        {
            GenerateBalance(rand, rmin, rmax);
        }

        FORCEINLINE FRoughV2D(const FVector2D& p, FRandomStream& rand, float rmin, float rmax)
            : Pos(p)
        {
            GenerateBalance(rand, rmin, rmax);
        }

        void GenerateBalance(FRandomStream& rand, float rmin, float rmax)
        {
            Balance = rand.GetFraction();
            MaxOffset = FMath::Max(.05f, rand.GetFraction()) * rand.FRandRange(rmin, rmax);
        }
    };

public:

    static bool GeneratePoly(const FJCVPolyIslandParams& Params, FRandomStream& Rand, TArray<FVector2D>& OutVerts)
    {
        typedef FPlatformMath FPM;
        typedef TDoubleLinkedList<FRoughV2D> TPolyList;
        typedef TPolyList::TDoubleLinkedListNode TPolyNode;

        // Input parameter invalid, abort
        if (! Params.IsValid())
        {
            return false;
        }

        const FVector2D& size( Params.Size );
        const FVector2D& displRange( Params.DisplacementRange );
        const int32 sideCount = Params.SideCount;
        const int32 subdivCount = Params.SubdivCount;
        const float subdivLimit = Params.SubdivLimit;
        const float polyScale = Params.PolyScale;
        const float polyAngleOffset = Params.PolyAngleOffset;

        const float displMin( displRange.X );
        const float displMax( displRange.Y );
        const FVector2D exts = size/2.f;

        FBox2D bounds;
        bounds.bIsValid = true;

        const float tau = 2*PI;
        const int32 sides = sideCount;

        const float sideAngle = tau/sides;
        float angle = (polyAngleOffset/360.f)*tau;

        TPolyList poly;

        // Generates initial poly verts
        for (int32 i=0; i<sides; ++i, angle+=sideAngle)
        {
            FRoughV2D pt(FPM::Cos(angle), FPM::Sin(angle), Rand, displMin, displMax);
            bounds += pt.Pos;
            poly.AddTail(pt);
        }
        poly.AddTail(poly.GetHead()->GetValue());

        // Subdivides poly edges
        for (int32 i=0; i<subdivCount; ++i)
        {
            int32 subdivPerformed = 0;
            TPolyNode* h( poly.GetHead() );
            TPolyNode* n( h->GetNextNode() );
            while (n)
            {
                const FRoughV2D& r0( n->GetPrevNode()->GetValue() );
                const FRoughV2D& r1( n->GetValue() );
                const FVector2D& v0( r0.Pos );
                const FVector2D& v1( r1.Pos );
                if (FVector2D::DistSquared(v0, v1) > subdivLimit)
                {
                    float mx = (v0.X+v1.X)*.5f;
                    float my = (v0.Y+v1.Y)*.5f;
                    float vx = -(v0.Y-v1.Y);
                    float vy = v0.X-v1.X;
                    float b = r1.Balance;
                    float o = r1.MaxOffset;
                    float d = (Rand.GetFraction()-b)*o;
                    FRoughV2D pt(mx+d*vx, my+d*vy, b, o);
                    poly.InsertNode(pt, n);
                    bounds += pt.Pos;
                    ++subdivPerformed;
                }
                n = n->GetNextNode();
            }
            if (subdivPerformed < 1)
            {
                break;
            }
        }
        // Fit points to bounds
        {
            const FVector2D& Unit( FVector2D::UnitVector );
            FVector2D offset( Unit-(Unit+bounds.Min) );
            FVector2D scaledOffset( (1.f-polyScale)*exts );
            float scale = ( size/bounds.GetSize() ).GetMin() * polyScale;

            OutVerts.Reset(poly.Num());

            for (const FRoughV2D& v : poly)
                OutVerts.Emplace(scaledOffset+(offset+v.Pos)*scale);
        }

        return true;
    }

};

UCLASS()
class JCVORONOIPLUGIN_API UJCVPolyIslandGeneratorBP : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

    UFUNCTION(BlueprintCallable, Category="JCV")
    static TArray<FVector2D> GeneratePoly(FJCVPolyIslandParamsBP Params)
    {
        FRandomStream rand(Params.RandomSeed);
        TArray<FVector2D> vertices;
        FJCVPolyIslandParams params;

        params.Set(
            Params.Size,
            Params.SideCount,
            Params.SubdivCount,
            Params.SubdivLimit,
            Params.PolyScale,
            Params.PolyAngleOffset,
            Params.DisplacementRange
        );

        FJCVPolyIslandGenerator::GeneratePoly(params, rand, vertices);

        return MoveTemp(vertices);
    }
};
