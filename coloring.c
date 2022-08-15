#ifndef K_COLORING_C
#define K_COLORING_C

#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

#include "graph.c"
#include "heuristics.c"
#include "low-k.c"
#include "bounds.c"

bool PRINT = false;

/*********************
 * GENERAL FUNCTIONS *
 *********************/

/**
 * Resets the graph by setting the color of all the vertices to -1.
 * @param vertices: the vertices of the graph
 */
void resetGraph(struct Vertex* (*vertices)) {
    for (int i=0; i<VERTICES; i++) { vertices[i]->color=-1; }
    return;
}

/**
 * Restores the changes by setting the color of all the vertices in @param coloredVertices to -1 and
 *  setting the available colors of all the vertices in @param changedVertices back by adding the change
 *  listed in @param vertexChanges.
 *  The parameters are reset: @param changedVertices and @param coloredVertices are set to NULL pointers
 *      and @param vertexChanges is set to 0 bitvectors.
 * @param changedVertices: the vertices of the graph where the availableColors was changed
 *                         After the function, the array only contains NULL pointers.
 * @param vertexChanges: the changes made to availableColors for all the vertices in @param changedVertices
 *                       vertexChanges[i] corresponds to the availableColors that were removed from changedVertices[i]
 *                       After the function, the array only contains 0 bitvectors.
 * @param coloredVertices: the vertices that were colored
 *                         After the function, the array only contains NULL pointers.
 */
void restore(struct Vertex* (*changedVertices), uint64_t (*vertexChanges), struct Vertex* (*coloredVertices)){
    int j = 0;
    while (coloredVertices[j] != NULL && j < VERTICES) {
        coloredVertices[j]->color = -1;
        coloredVertices[j] = NULL;
        j++;
    }
    int i=0;
    while (changedVertices[i] != NULL && i < VERTICES) {
        changedVertices[i]->availableColors += vertexChanges[i];
        if (SEARCH == EXHAUSTIVE) { updateVertexAvailability(changedVertices[i], CHROMATIC); }
        changedVertices[i] = NULL;
        vertexChanges[i] = (uint64_t)0;
        i++;
    }
}

/**
 * Deletes @param color as an available color for all the vertices in the linked list @param neighbor.
 *  Adds the affected vertices to @param changedVertices
 *  and adds the color to @param vertexChanges for the affected vertices.
 * @param neighbor: linked list of neighbors where @param color needs to be removed as an available color
 *                  At the end of the function, all vertices in the linked list no longer have @param color as an
 *                  available color.
 * @param color: the color that needs to be removed from the vertices in the linked list @param neighbor
 * @param changedVertices: array that tracks the vertices with a change in their availableColors
 *                         At the end of the function, all the vertices in @param neighbor that had @param color
 *                         as an available color are added to the array, if they were not already in the array.
 * @param vertexChanges: array that tracks the changes in availableColors for all the vertices in @param changedVertices
 *                       vertexChanges[i] corresponds to the availableColors that were removed from changedVertices[i]
 *                       At the end of the function, all the entries in @param vertexChanges corresponding to vertices
 *                       in @param changedVertices that are in @param neighbor have @param color listed as a change
 *                       in the available colors.
 */
void removeNeighborColors(struct ListElem* neighbor, int color, struct Vertex* (*changedVertices), uint64_t (*vertexChanges)){
    while (neighbor != NULL) {
        struct Vertex* nextVertex = neighbor->vertex;
        if (nextVertex->availableColors & (uint64_t)1<<color) {
            /* delete color from available colors */
            nextVertex->availableColors -= (uint64_t)1<<color;
            int i=0;
            while (changedVertices[i] != NULL) {
                if (changedVertices[i] == nextVertex) {
                    vertexChanges[i] += (uint64_t)1<<color;
                    break;
                }
                i++;
            }
            if (changedVertices[i] == NULL) {
                changedVertices[i] = nextVertex;
                vertexChanges[i] = (uint64_t)1<<color;
            }
        }
        neighbor = neighbor->next;
    }
}

/**
 * Iterates over all the affected vertices (listed in the linked list @param neighbor) to persist
 *  the changes made to the availableColors.
 *  If a vertex in @param neighbor has no colors left in their availableColors: -1 is returned to signal that
 *      a conflict occured when persisting the changes.
 *  If a vertex in @param neighbor has only a single color left in their availableColors: the vertex is colored
 *      with that color, the color is removed as an availableColor from its neighbors and the changes are persisted.
 * @param neighbor: linked list of all affected vertices
 * @param changedVertices: array that tracks the vertices with a change in their availableColors
 *                         At the end of the function, all the vertices that had a change in their availableColors
 *                         are added to the array, if they were not already in the array.
 * @param vertexChanges: array that tracks the changes in availableColors for all the vertices in @param changedVertices
 *                       vertexChanges[i] corresponds to the availableColors that were removed from changedVertices[i]
 *                       At the end of the function, all changes in availableColors for any vertex
 *                       in @param changedVertices is included in its corresponding entry in @param vertexChanges.
 * @param coloredVertices: array that tracks all the vertices that were colored.
 * @return if no conflict occurred: 1
 *         else: -1
 */
int persistColors(struct ListElem* neighbor, struct Vertex* (*changedVertices), uint64_t (*vertexChanges), struct Vertex* (*coloredVertices)) {
    while (neighbor != NULL) {
        struct Vertex *nextNeighbor = neighbor->vertex;
        if (nextNeighbor->color == -1) {
            uint64_t neighborsAvailableColors = nextNeighbor->availableColors;
            if (neighborsAvailableColors == 0) {
                nextNeighbor->nbConflicts++;
                return -1;
            } // if the neighboring vertex has no possible colors left, report failure
            if (!(neighborsAvailableColors & (neighborsAvailableColors - 1))){ // if the neighboring vertex has only 1 possible color left, color the vertex
                int color = __builtin_ctzll(neighborsAvailableColors);
                nextNeighbor->color = color;
                if (color > MAX_USED) { MAX_USED = color; }
                int i = 0;
                while (coloredVertices[i] != NULL) { i++; }
                coloredVertices[i] = nextNeighbor;
                removeNeighborColors(nextNeighbor->adjacentVertices, color, changedVertices, vertexChanges);
                if (persistColors(nextNeighbor->adjacentVertices, changedVertices, vertexChanges, coloredVertices) == -1) { return -1; }
            }
        }
        neighbor = neighbor->next;
    }
    return 1;
}

/********************
 * GLOBAL FUNCTIONS *
 ********************/

/**
 * Colors the graph recursively by selecting an uncolored vertex.
 * If no next vertex can be selected, all the vertices are colored without conflicts, and
 *      if SEARCH != EXHAUSTIVE: success (1) is returned
 *      else: the value CHROMATIC is updated to be the maximal color used (MAX_USED) as the found coloring uses less
 *          colors than the previously found coloring. The availableColors of the vertices are updated so that the
 *          color MAX_USED is no longer an available color as the program continues to find a coloring with less colors
 * If a next vertex can be selected, the program tries to find a color for that vertex.
 *      If no color can be found, failure (-1) is returned.
 *      If a color can be found, the vertex is colored with the color, the color is removed as an available color from
 *      its neighbors, and the changes are persisted.
 *          If persisting the changes does not result in a conflict, the function is called recursively to try and
 *          color a next vertex. If the recursive function call returns  failure (-1), all changes are restored and
 *          the function will try to find a different color for the vertex. If the recursive function call returns
 *          success (1), success (1) is returned if SEARCH != EXHAUSTIVE, otherwise the program will continue to try to
 *          find a coloring with less colors.
 *          If persisting the changes results in a conflict, all the changes that were made to the vertices are
 *          restored and the function will try to find a different color for the vertex.
 * @param vertices: the vertices of the graph
 * @return if SEARCH != EXHAUSTIVE: returns 1 if graph is colored, -1 if graph cannot be colored
 *         if SEARCH == EXHAUSTIVE: returns -1 as the exhaustive search will keep updating the value CHROMATIC and try
 *         to find a coloring using less colors, as soon as CHROMATIC+1 equals the chromatic number of a graph, no new
 *         coloring can be found and so the function will return failure (-1). The global variable CHROMATIC however, is
 *         set to the highest color needed for the optimal vertex coloring (CHROMATIC+1 = chromatic number of graph).
 */
int colorGraph(struct Vertex* (*vertices)) {
    /* SELECT NEXT VERTEX */
    int old_counter = VERTEX_COUNTER;
    int nextVertex;
    if      (SORTING == RECOLOR)  {for (int i=0; i<VERTICES; i++) { vertices[i]->nbRecolorings *= DECAY_FACTOR;}}
    else if (SORTING == CONFLICT) {for (int i=0; i<VERTICES; i++) { vertices[i]->nbConflicts   *= DECAY_FACTOR;}}
    SORTING_COUNTER++;
    nextVertex = selectNextVertex(vertices);
    if (!CS) { VERTEX_COUNTER = nextVertex; }
    if (nextVertex == -1) {
        if (SEARCH == EXHAUSTIVE && MAX_USED<CHROMATIC) {
            CHROMATIC = MAX_USED;
            updateAvailability(vertices,CHROMATIC);
            printf("\tX(G)<=%d\n", CHROMATIC+1);
        }
        return 1;
    }
    /* INITIALIZE TRACKING OF CHANGES */
    struct Vertex* coloredVertices[VERTICES];
    struct Vertex* changedVertices[VERTICES];
    uint64_t vertexChanges[VERTICES];
    for (int i=0; i<VERTICES; i++) {
        coloredVertices[i] = NULL;
        changedVertices[i] = NULL;
        vertexChanges[i] = (uint64_t)0;
    }
    /* RECURSION STEP */
    int nextColor=-1;
    int max_used_temp = MAX_USED;
    while (1) {
        if (SEARCH == EXHAUSTIVE && MAX_USED >= CHROMATIC) { return 1; }
        coloredVertices[0] = vertices[nextVertex];
        /* select a color for the vertex */
        nextColor = findColorFF(nextColor + 1, vertices[nextVertex]);
        setVertexColor(vertices[nextVertex], nextColor);
        if (PRINT) { printColorsCompact(vertices); }
        // if no color is found, backtrack over the last choice
        if (nextColor == -1) {
            VERTEX_COUNTER = old_counter;
            return -1;
        }
        if (nextColor > MAX_USED) { MAX_USED = nextColor; }
        /* remove the color from neighbors */
        removeNeighborColors(vertices[nextVertex]->adjacentVertices, nextColor, changedVertices, vertexChanges);
        if (persistColors(vertices[nextVertex]->adjacentVertices, changedVertices, vertexChanges, coloredVertices) == 1) {
            if (PRINT) { printColorsCompact(vertices); }
            /* find coloring for rest of vertices */
            if (SEARCH == EXHAUSTIVE) { colorGraph(vertices); }
            else if (colorGraph(vertices) == 1) { return 1; }
        }
        /* coloring was not found -> prepare for next iteration */
        MAX_USED = max_used_temp;
        restore(changedVertices, vertexChanges, coloredVertices);
        if (PRINT) { printColorsCompact(vertices); }
    }
}

/**
 * Initializes global counters.
 */
void initializeCounters() {
    MAX_USED = -1;
    VERTEX_COUNTER = -1;
    SORTING_COUNTER = 0;
}

/**
 * Finds a proper vertex coloring of @param vertices.
 * @param vertices: the vertices of the graph
 * @return if a proper coloring can be found: number of colors needed for the proper vertex coloring
 *         else: -1
 */
int coloring(struct Vertex* (*vertices)) {
    initializeCounters();
    /* FIND COLORING */
    if (SEARCH == EXHAUSTIVE) {
        initialSorting(vertices);
        printf("\tSEARCH:\n");
        int bound = getUpperBound(vertices);
        initializeCounters();
        if (BOUNDS == BROOKS && bound == MAX_DEGREE+1) { return bound; }
        if (bound<64) {
            CHROMATIC = bound-1;
            setAvailableColors(vertices, CHROMATIC);
        } else {
            CHROMATIC = 127;
            setAvailableColors(vertices, 64);
        }
        colorGraph(vertices);
        printf("\n");
        return CHROMATIC+1;
    } else {
//        if (MAX_COLOR < 4) { printf("\tspecialized %d-coloring\n", MAX_COLOR+1); return kColoring(vertices, MAX_COLOR+1); } // 2-, 3-, or 4-coloring <== availability set [0..MAX_COLOR]
        initialSorting(vertices);
        int result = colorGraph(vertices);
        if (result != -1) { return MAX_USED+1; }
        return result;
    }
}

#endif