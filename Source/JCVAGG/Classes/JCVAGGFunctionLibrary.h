////////////////////////////////////////////////////////////////////////////////
//
// MIT License
// 
// Copyright (c) 2018-2019 Nuraga Wiswakarma
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////
// 

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "AGGPathController.h"
#include "AGGRendererObject.h"
#include "JCVTypes.h"
#include "JCVDiagramAccessor.h"
#include "JCVAGGFunctionLibrary.generated.h"

USTRUCT(BlueprintType, Blueprintable)
struct JCVAGG_API FJCVAGGFeatureDrawParams
{
	GENERATED_BODY()

    /**
     * Source Feature Type
     */
	UPROPERTY(Category="Preview Settings", BlueprintReadWrite, EditAnywhere)
    uint8 FeatureType = EJCVCellFeature::ONE;

    /**
     * Edge Type:
     * 0. Curve3
     * 1. Curve4
     * 2. Linear
     */
	UPROPERTY(Category = "Preview Settings", BlueprintReadWrite, EditAnywhere)
    uint8 EdgeType = 0;

    /**
     * Draw scale
     */
	UPROPERTY(Category = "Preview Settings", BlueprintReadWrite, EditAnywhere)
    float DrawScale = 1.f;
};

UCLASS()
class JCVAGG_API UJCVAGGFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

    static void DrawSite(UAGGPathController& path, const FJCVSite& site, bool bClear = true)
    {
        const FJCVEdge* g = site.edges;
        if (bClear)
            path.Clear();
        path.MoveTo(g->pos[0].x, g->pos[0].y);
        do
        {
            path.LineTo(g->pos[1].x, g->pos[1].y);
        }
        while ((g=g->next) != nullptr);
        path.ClosePolygon();
    }
    
    static void DrawMap(FJCVDiagramMap& Map, UAGGPathController& Path, UAGGRendererBase& Renderer, const FJCVAGGFeatureDrawParams& Params, FColor Color)
    {
        const uint8 et = Params.EdgeType;
        const uint8 ft = Params.FeatureType;

        const float Scale = Params.DrawScale;
        const bool bHasScaling = ! FMath::IsNearlyEqual(Params.DrawScale, 1.f, KINDA_SMALL_NUMBER);

        FJCVCellSet cg;
        FJCVConstEdgeSet es;
        TArray<FJCVEdgeNode> eg;
        FAGGCurveSettings curve;
        int32 feature = 0;

        do
        {
            cg.Reset();
            Map.GetBorderCells(cg, ft, -1, feature++, true, true);

            es.Reset();
            es.Reserve(cg.Num());

            Map.GetBorderEdges(cg, es, true);

            eg.Reset(cg.Num());
            Map.SortEdges(es, eg);

            if (eg.Num() <= 0)
            {
                continue;
            }

            Path.Clear();
            Path.ResetTransform();

            if (et < 2)
            {
                Path.MoveTo(eg.Last().GetMid().x, eg.Last().GetMid().y);

                for (const FJCVEdgeNode& n : eg)
                {
                    const FJCVPoint& h(n.GetHead());
                    const FJCVPoint& m(n.GetMid());

                    if (et == 0)
                    {
                        Path.Curve3(h.x, h.y, m.x, m.y);
                    }
                    else
                    {
                        Path.Curve4(h.x, h.y, h.x, h.y, m.x, m.y);
                    }
                }

                Path.PathAsCurve(curve);
                Path.ApplyConversion();
            }
            else
            {
                Path.MoveTo(eg[0].GetHead().x, eg[0].GetHead().y);

                for (const FJCVEdgeNode& n : eg)
                {
                    const FJCVPoint& h(n.GetHead());
                    Path.LineTo(h.x, h.y);
                }

                Path.ClosePolygon();
            }

            if (bHasScaling)
            {
                Path.Scale(Scale);
                Path.ApplyTransform();
            }

            Renderer.SetColor(Color);
            Renderer.Render(&Path);
        }
        while (cg.Num() > 0);
    }

public:

    UFUNCTION(BlueprintCallable, Category="JCV AGG")
    static void DrawFeature(UJCVDiagramAccessor* Accessor, UAGGPathController* Path, UAGGRendererBase* Renderer, const FJCVAGGFeatureDrawParams& Params, FColor Color)
    {
        if (! IsValid(Path))
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVAGGFunctionLibrary::DrawMap() ABORTED, INVALID AGG PATH CONTROLLER"));
            return;
        }

        if (! IsValid(Renderer))
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVAGGFunctionLibrary::DrawMap() ABORTED, INVALID AGG RENDERER"));
            return;
        }

        if (! IsValid(Accessor) || ! Accessor->HasValidMap())
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVAGGFunctionLibrary::DrawMap() ABORTED, INVALID DIAGRAM ACCESSOR"));
            return;
        }

        FJCVDiagramMap& Map(Accessor->GetMap());
        DrawMap(Map, *Path, *Renderer, Params, Color);
    } 
    UFUNCTION(BlueprintCallable, Category="JCV AGG")
    static void DrawCell(UJCVDiagramAccessor* Accessor, UAGGPathController* Path, UAGGRendererBase* Renderer, const FJCVCellRef& Cell, FColor Color)
    {
        if (! IsValid(Path))
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVAGGFunctionLibrary::DrawCell() ABORTED, INVALID AGG PATH CONTROLLER"));
            return;
        }

        if (! IsValid(Renderer))
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVAGGFunctionLibrary::DrawCell() ABORTED, INVALID AGG RENDERER"));
            return;
        }

        if (! IsValid(Accessor) || ! Accessor->HasValidMap())
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVAGGFunctionLibrary::DrawCell() ABORTED, INVALID DIAGRAM ACCESSOR"));
            return;
        }

        if (! Cell.Data || ! Cell.Data->Site)
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVAGGFunctionLibrary::DrawCell() ABORTED, INVALID CELL REFERENCE"));
            return;
        }

        DrawSite(*Path, *Cell.Data->Site);
        Renderer->SetColor(Color);
        Renderer->Render(Path);
    }

    UFUNCTION(BlueprintCallable, Category="JCV AGG")
    static void DrawCellGroup(UJCVDiagramAccessor* Accessor, UAGGPathController* Path, UAGGRendererBase* Renderer, const FJCVCellRefGroup& CellGroup, FColor Color)
    {
        if (! IsValid(Path))
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVAGGFunctionLibrary::DrawCellGroup() ABORTED, INVALID AGG PATH CONTROLLER"));
            return;
        }

        if (! IsValid(Renderer))
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVAGGFunctionLibrary::DrawCellGroup() ABORTED, INVALID AGG RENDERER"));
            return;
        }

        if (! IsValid(Accessor) || ! Accessor->HasValidMap())
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVAGGFunctionLibrary::DrawCellGroup() ABORTED, INVALID DIAGRAM ACCESSOR"));
            return;
        }

        Path->Clear();

        for (const FJCVCellRef& Cell : CellGroup.Data)
        {
            if (Cell.Data && Cell.Data->Site)
            {
                DrawSite(*Path, *Cell.Data->Site, false);
            }
        }

        Renderer->SetColor(Color);
        Renderer->Render(Path);
    }

    UFUNCTION(BlueprintCallable, Category="JCV AGG")
    static void DrawSite(UJCVDiagramAccessor* Accessor, UAGGPathController* Path, UAGGRendererBase* Renderer, int32 SiteID, FColor Color)
    {
        if (! IsValid(Path))
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVAGGFunctionLibrary::DrawSite() ABORTED, INVALID AGG PATH CONTROLLER"));
            return;
        }

        if (! IsValid(Renderer))
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVAGGFunctionLibrary::DrawSite() ABORTED, INVALID AGG RENDERER"));
            return;
        }

        if (! IsValid(Accessor) || ! Accessor->HasValidMap())
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVAGGFunctionLibrary::DrawSite() ABORTED, INVALID DIAGRAM ACCESSOR"));
            return;
        }

        FJCVDiagramMap& Map(Accessor->GetMap());

        if (! Map.IsValidIndex(SiteID))
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVAGGFunctionLibrary::DrawSite() ABORTED, INVALID ISLAND SITE ID"));
            return;
        }

        DrawSite(*Path, *Map.GetSite(SiteID));
        Renderer->SetColor(Color);
        Renderer->Render(Path);
    }

    UFUNCTION(BlueprintCallable, Category="JCV AGG")
    static void DrawSiteByType(UJCVDiagramAccessor* Accessor, UAGGPathController* Path, UAGGRendererBase* Renderer, uint8 Type, int32 TypeID, FColor Color)
    {
        if (! IsValid(Path))
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVAGGFunctionLibrary::DrawSiteByType() ABORTED, INVALID AGG PATH CONTROLLER"));
            return;
        }

        if (! IsValid(Renderer))
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVAGGFunctionLibrary::DrawSiteByType() ABORTED, INVALID AGG RENDERER"));
            return;
        }

        if (! IsValid(Accessor) || ! Accessor->HasValidMap())
        {
            UE_LOG(LogTemp,Warning, TEXT("UJCVAGGFunctionLibrary::DrawSiteByType() ABORTED, INVALID DIAGRAM ACCESSOR"));
            return;
        }

        FJCVDiagramMap& Map(Accessor->GetMap());

        Path->Clear();
        for (int32 i=0; i<Map.Num(); ++i)
            if (Map.GetCell(i).IsType(Type, TypeID))
                DrawSite(*Path, *Map.GetSite(i), false);

        Renderer->SetColor(Color);
        Renderer->Render(Path);
    }
};
