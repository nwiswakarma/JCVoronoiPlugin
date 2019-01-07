/*

ABOUT:

	A fast single file 2D voronoi diagram generator.

HISTORY:

    0.3     2017-04-16	- Added clipping box as input argument (Automatically calcuated if needed)
                        - Input points are pruned based on bounding box
    0.2     2016-12-30  - Fixed issue of edges not being closed properly
                        - Fixed issue when having many events
                        - Fixed edge sorting
                        - Code cleanup
    0.1                 Initial version

LICENSE:

	The MIT License (MIT)

	Copyright (c) 2015 Mathias Westerdahl
	Copyright (c) 2017 Nuraga Wiswakarma

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.


DISCLAIMER:

	This software is supplied "AS IS" without any warranties and support

USAGE:

	The input points are pruned if

		* There are duplicates points
		* The input points are outside of the bounding box

	The input bounding box is optional

	The api is very small:

	void jcv_diagram_generate( int num_points, const jcv_point* points, const jcv_rect* rect, jcv_diagram* diagram );
	void jcv_diagram_generate_useralloc( int num_points, const jcv_point* points, const jcv_rect* rect, void* userallocctx, FJCVAllocFn allocfn, FJCVFreeFn freefn, jcv_diagram* diagram );
	void jcv_diagram_free( jcv_diagram* diagram );

	const jcv_site* jcv_diagram_get_sites( const jcv_diagram* diagram );
	const jcv_edge* jcv_diagram_get_edges( const jcv_diagram* diagram );

	An example usage:

	#define JC_VORONOI_IMPLEMENTATION
	// If you wish to use doubles
	//#define JCV_REAL_TYPE double
	//#define JCV_FABS fabs
	//#define JCV_ATAN2 atan2
	#include "jc_voronoi.h"

	void draw_edges(const jcv_diagram* diagram);
	void draw_cells(const jcv_diagram* diagram);

	void generate_and_draw(int numpoints, const jcv_point* points)
	{
		jcv_diagram diagram;
		memset(&diagram, 0, sizeof(jcv_diagram));
		jcv_diagram_generate(count, points, 0, &diagram);

		draw_edges(diagram);
		draw_cells(diagram);

		jcv_diagram_free( &diagram );
	}

	void draw_edges(const jcv_diagram* diagram)
	{
		// If all you need are the edges
		const jcv_edge* edge = jcv_diagram_get_edges( diagram );
		while( edge )
		{
			draw_line(edge->pos[0], edge->pos[1]);
			edge = edge->next;
		}
	}

	void draw_cells(const jcv_diagram* diagram)
	{
		// If you want to draw triangles, or relax the diagram,
		// you can iterate over the sites and get all edges easily
		const jcv_site* sites = jcv_diagram_get_sites( diagram );
		for( int i = 0; i < diagram->numsites; ++i )
		{
			const jcv_site* site = &sites[i];

			const jcv_graphedge* e = site->edges;
			while( e )
			{
				draw_triangle( site->p, e->pos[0], e->pos[1]);
				e = e->next;
			}
		}
	}

	// Here is a simple example of how to do the relaxations of the cells
	void relax_points(const jcv_diagram* diagram, jcv_point* points)
	{
		const jcv_site* sites = jcv_diagram_get_sites(diagram);
		for( int i = 0; i < diagram->numsites; ++i )
		{
			const jcv_site* site = &sites[i];
			jcv_point sum = site->p;
			int count = 1;

			const jcv_graphedge* edge = site->edges;

			while( edge )
			{
				sum.x += edge->pos[0].x;
				sum.y += edge->pos[0].y;
				++count;
				edge = edge->next;
			}

			points[site->index].x = sum.x / count;
			points[site->index].y = sum.y / count;
		}
	}

 */

#ifndef JC_VORONOI_H
#define JC_VORONOI_H

#include <math.h>
#include <stddef.h>

#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef JCV_REAL_TYPE
	#define JCV_REAL_TYPE float
#endif

#ifndef JCV_ATAN2
	#define JCV_ATAN2(_Y_, _X_)	atan2f(_Y_, _X_)
#endif

#ifndef JCV_SQRT
	#define JCV_SQRT(_X_)		sqrtf(_X_)
#endif

#ifndef JCV_FABS
	#define JCV_FABS(_X_)		fabsf(_X_)
#endif

#ifndef JCV_PI
	#define JCV_PI 3.14159265358979323846264338327950288f
#endif

typedef JCV_REAL_TYPE jcv_real;

#pragma pack(push, 1)

typedef struct _jcv_point
{
	jcv_real x;
	jcv_real y;
} jcv_point;

typedef struct _jcv_graphedge
{
	struct _jcv_graphedge*	next;
	struct _jcv_edge*		edge;
	struct _jcv_site*		neighbor;
	jcv_point				pos[2];
	jcv_real				angle;
} jcv_graphedge;

typedef struct _jcv_site
{
	jcv_point		p;
	int				index;	// Index into the original list of points
	jcv_graphedge* 	edges;	// The half edges owned by the cell
} jcv_site;

typedef struct _jcv_edge
{
	struct _jcv_edge*	next;
	jcv_site* 			sites[2];
	jcv_point			pos[2];
	jcv_real			a;
	jcv_real			b;
	jcv_real			c;
} jcv_edge;

typedef struct _jcv_rect
{
	jcv_point 	min;
	jcv_point 	max;
} jcv_rect;

typedef struct _jcv_diagram
{
	struct _jcv_context_internal* internal;
	jcv_edge*	edges;
	jcv_site*	sites;
	int			numsites;
	jcv_point 	min;
	jcv_point 	max;
} jcv_diagram;

#pragma pack(pop)


typedef void* (*FJCVAllocFn)(void* userctx, size_t size);
typedef void (*FJCVFreeFn)(void* userctx, void* p);

/** Same as above, but allows the client to use a custom allocator
 */
extern void jcv_diagram_generate_useralloc( int num_points, const jcv_point* points, const jcv_rect* rect, void* userallocctx, FJCVAllocFn allocfn, FJCVFreeFn freefn, jcv_diagram* diagram );


/** Uses free (or the registered custom free function)
 */
extern void jcv_diagram_free( jcv_diagram* diagram );

/** Returns an array of sites, where each index is the same as the original input point array.
 */
extern const jcv_site* jcv_diagram_get_sites( const jcv_diagram* diagram );

/** Returns a linked list of all the voronoi edges
 */
extern const jcv_edge* jcv_diagram_get_edges( const jcv_diagram* diagram );


#ifdef __cplusplus
}
#endif

#endif // JC_VORONOI_H


