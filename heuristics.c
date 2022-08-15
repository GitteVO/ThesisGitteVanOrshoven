#ifndef HEURISTICS_C
#define HEURISTICS_C

#include <math.h>

/**
 * Possible search methods
 */
enum search{KCOLORING,BINARY,GREEDY,GREBIN,EXHAUSTIVE};
/**
 * The configuration denoting which search method to use
 */
enum search SEARCH;
/**
 * The configuration denoting whether or not to enforce a connected sequential ordering
 */
bool CS;
/**
 * Possible sorting heuristics
 */
enum selection{VERTEX,DEGREE,IDO,DSATUR,RECOLOR,CONFLICT};
/**
 * The configuration denoting which sorting heuristic to use
 */
enum selection SORTING;
/**
 * The configuration denoting the frequency for dynamic sorting
 */
int SORTING_RATE; // if 0 then no sorting else sorting every 1/SORTING_RATE iteration
int SORTING_COUNTER = 0;
/**
 * The configuration denoting the decay for the recolor and conflict counters
 */
double DECAY_FACTOR;

/****************************
 * VERTEX SORTING FUNCTIONS *
 ****************************/

/**
 * Compares the vertices by their number.
 * Vertices are sorted by increasing vertex number.
 */
int compareByVertexNumber(const void* vertex1, const void* vertex2) {
    const struct Vertex* vertexA = *(struct Vertex**) vertex1;
    const struct Vertex* vertexB = *(struct Vertex**) vertex2;
    return (vertexA->number - vertexB->number); // sort by increasing vertex number
    // return value < 0 ==> vertex1 before vertex2
    // return value > 0 ==> vertex2 before vertex1
}

/**
 * Compares the vertices by their degree.
 * Vertices are sorted by non-increasing degree.
 */
int compareByDegree(const void* vertex1, const void* vertex2) {
    const struct Vertex* vertexA = *(struct Vertex**) vertex1;
    const struct Vertex* vertexB = *(struct Vertex**) vertex2;
    return (vertexB->degree - vertexA->degree); // sort by decreasing degree
}

/**
 * Compares the vertices by their number of colored neighbors.
 * Vertices are sorted by non-increasing number of colored neighbors.
 */
int compareByIDO(const void* vertex1, const void* vertex2) {
    const struct Vertex* vertexA = *(struct Vertex**) vertex1;
    const struct Vertex* vertexB = *(struct Vertex**) vertex2;
    int nbA = 0;
    int nbB = 0;
    struct ListElem* neighborA = vertexA->adjacentVertices;
    struct ListElem* neighborB = vertexB->adjacentVertices;
    while (neighborA != NULL) {
        if (neighborA->vertex->color != -1) { nbA++; }
        neighborA = neighborA->next;
    }
    while (neighborB != NULL) {
        if (neighborB->vertex->color != -1) { nbB++; }
        neighborB = neighborB->next;
    }
    return (nbB-nbA);
}

/**
 * Compares vertices by their degree of saturation.
 * Vertices are sorted by non-increasing degree of saturation.
 */
int compareBySaturationDegree(const void* vertex1, const void* vertex2) {
    const struct Vertex* vertexA = *(struct Vertex**) vertex1;
    const struct Vertex* vertexB = *(struct Vertex**) vertex2;
    //degree of saturation = #unavailable colors = MAX_COLOR+1-nbAvailableColors ==> DSATUR_A >= DSATUR_B <=> nbAvailableColors_A <= nbAvailableColors_B
    int nbAvailableColorsA = __builtin_popcount(vertexA->availableColors);
    int nbAvailableColorsB = __builtin_popcount(vertexB->availableColors);
    if (nbAvailableColorsA <  nbAvailableColorsB) { return -1; }
    if (nbAvailableColorsA == nbAvailableColorsB) {
        if (vertexA->degree >  vertexB->degree) { return -1; }
        if (vertexA->degree == vertexB->degree) { return  0; }
        if (vertexA->degree <  vertexB->degree) { return  1;}
    }
    return  1;
}

/**
 * Compares vertices by their number of recolorings.
 * Vertices are sorted by non-increasing number of recolorings.
 */
int compareByRecolorings(const void* vertex1, const void* vertex2) {
    const struct Vertex* vertexA = *(struct Vertex**) vertex1;
    const struct Vertex* vertexB = *(struct Vertex**) vertex2;
    if      (vertexA->nbRecolorings >  vertexB->nbRecolorings) { return -1; }
    else if (vertexA->nbRecolorings == vertexB->nbRecolorings) { return  0; }
    else                                                       { return  1; } // sort by decreasing nbRecolorings
}

/**
 * Compares vertices by their number of conflicts.
 * Vertices are sorted by non-increasing number of conflicts.
 */
int compareByConflicts(const void* vertex1, const void* vertex2) {
    const struct Vertex* vertexA = *(struct Vertex**) vertex1;
    const struct Vertex* vertexB = *(struct Vertex**) vertex2;
    if      (vertexA->nbConflicts >  vertexB->nbConflicts) { return -1; }
    else if (vertexA->nbConflicts == vertexB->nbConflicts) { return  0; }
    else                                                   { return  1; } // sort by decreasing nbConflicts
}

/******************************
 * VERTEX AND COLOR SELECTION *
 ******************************/

/**
 * Finds the first vertex in @param vertices that is uncolored.
 * @param vertices: the vertices of the graph
 * @return the index of the first vertex in @param vertices that is uncolored, if such a vertex exists
 *         -1, otherwise
 */
int selectNextVertexFF(struct Vertex* (*vertices)) {
    for (int vi=VERTEX_COUNTER+1; vi<VERTICES;vi++){ if (vertices[vi]->color == -1) { return vi; } }
    return -1;
}

/**
 * Finds the first vertex in @param vertices that is uncolored and has at least one colored neighbor.
 * @param vertices: the vertices of the graph
 * @return the index of the first vertex in @param vertices that is uncolored and has at least one
 *          colored neighbor, if such a vertex exists
 *         -1, otherwise
 */
int selectNextVertexCS(struct Vertex* (*vertices)) {
    while (VERTEX_COUNTER+1<VERTICES && vertices[VERTEX_COUNTER+1]->color != -1) { VERTEX_COUNTER++; }
    for (int vi=VERTEX_COUNTER+1; vi<VERTICES;vi++){
        if ((hasColoredNeighbor(vertices[vi])) && (vertices[vi]->color == -1)) {
            return vi;
        }
    }
    return selectNextVertexFF(vertices);
}

/**
 * Sorts the vertices according to the configurations and returns
 *  if CS = false: the first vertex that is uncolored.
 *  if CS = true:  the first vertex that is uncolored and has at least one colored neighbor.
 * @param vertices: the vertices of the graph
 * @return the index of the next vertex to be colored according to the configuration
 */
int selectNextVertex(struct Vertex* (*vertices)) {
    if (SORTING_RATE != 0 && (SORTING_COUNTER % SORTING_RATE) == 0) {
        struct Vertex* (*startVertex) = vertices + (VERTEX_COUNTER+1);
        int numberToSort = VERTICES-VERTEX_COUNTER-1;
        if      (SORTING == IDO)      { qsort(startVertex, numberToSort, sizeof(struct Vertex*), &compareByIDO); }
        else if (SORTING == DSATUR)   { qsort(startVertex, numberToSort, sizeof(struct Vertex*), &compareBySaturationDegree); }
        else if (SORTING == RECOLOR)  { qsort(startVertex, numberToSort, sizeof(struct Vertex*), &compareByRecolorings); }
        else if (SORTING == CONFLICT) { qsort(startVertex, numberToSort, sizeof(struct Vertex*), &compareByConflicts); }
    }
    if (CS) { return selectNextVertexCS(vertices); }
    else    { return selectNextVertexFF(vertices); }
}

/**
 * Returns the index of the least significant bit of @param n
 */
int getFirstSetBitPos(uint64_t n) {
    return (int)log2(n & -n) + 1;
}

/**
 * Returns the index of the @param k-th least significant bit of @param n
 */
int getKthSetBitPos(uint64_t (*n), int k) {
    for (int i=0; i<BIT_ARRAY_SIZE; i++) {
        int nbSetBits = __builtin_popcount(n[i]);
        if (nbSetBits<k) { k -= nbSetBits; continue; }
        uint64_t bitArray = n[i];
        for (int j=0; j<k-1; j++) { bitArray &=~ (bitArray & -bitArray); } // remove (k-1) least significant bits
        return (int)log2(bitArray & -bitArray) + 64*i; // return position of least significant bit
    }
    return -1;
}

/**
 * Find the first available color of @param vertex that is larger than or equal to @param minColor and
 *  if SEARCH != EXHAUSTIVE: is smaller than or equal to the maximal allowed color.
 *  else: is smaller than or equal to CHROMATIC+1, the number of colors for the best proper coloring found so far.
 * @param minColor: the minimal color needed
 * @param vertex: the vertex for which a color needs to be found
 * @return the first available color of @param vertex that is larger than or equal to @param minColor if it exists and
 *              if SEARCH != EXHAUSTIVE: is smaller than or equal to the maximal allowed color.
 *              else: is smaller than or equal to CHROMATIC+1, the number of colors for the best proper coloring found so far.
 *         -1 if no such color exists
 */
int findColorFF(int minColor, struct Vertex* vertex){
    int color = getFirstSetBitPos((vertex->availableColors)>>(minColor)) + minColor -1;
    if      (SEARCH != EXHAUSTIVE && 0<=color && color<=MAX_USED+1 && color<=MAX_COLOR) { return color; }
    else if (SEARCH == EXHAUSTIVE && 0<=color && color<=MAX_USED+1 && color<CHROMATIC)  { return color; }
    else { return -1; }
}

/**
 * Sort the vertices at the start of the execution according to the configuration of the algorithm.
 * If SORTING is set to VERTEX, the vertices are sorted by increasing vertex number.
 * If SORTING is set to DEGREE, DSATUR, or IDO, the vertices are sorted by non-increasing degree.
 * If SORTING is set to RECOLOR, the vertices are sorted by non-increasing number of recolorings.
 * If SORTING is set to CONFLICT, the vertices are sorted by non-increasing number of conflicts.
 */
void initialSorting(struct Vertex* (*vertices)) {
    if (SORTING == VERTEX) {
        qsort(vertices, VERTICES, sizeof(struct Vertex*), &compareByVertexNumber);
    } else if (SORTING == DEGREE || SORTING == DSATUR || SORTING == IDO) {
        qsort(vertices, VERTICES, sizeof(struct Vertex*), &compareByDegree);
    } else if (SORTING == RECOLOR) {
        qsort(vertices, VERTICES, sizeof(struct Vertex*), &compareByRecolorings);
    } else if (SORTING == CONFLICT) {
        qsort(vertices, VERTICES, sizeof(struct Vertex*), &compareByConflicts);
    }
}

#endif