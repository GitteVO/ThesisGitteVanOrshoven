#ifndef BOUNDS_C
#define BOUNDS_C

#include "graph.c"
#include "heuristics.c"
#include <time.h>

/**
 * Possible upper bounds for the chromatic number
 */
enum bounds{NO,BROOKS,RLF,WP};
/**
 * The configuration denoting which upper bound should be used.
 */
enum bounds BOUNDS;
/**
 * Global variables to help the RLF algorithm
 */
int SECONDARY_SIZE;
int SET_U_SIZE;
int SET_V_SIZE;

/**********
 * BROOKS *
 **********/

/**
 * Determines the upper bound of the vertices by using Brooks' Theorem.
 * @param vertices: the vertices of the graph
 * @return  if the graph is not complete and not an odd cycle: maximum degree
 *          else: maximum degree + 1
 */
int brooksUpperBound(struct Vertex* (*vertices)) {
    if (VERTICES%2 == 1) {
        int degree = vertices[0]->degree;
        if (degree!=2 && degree!=(VERTICES-1)) { return MAX_DEGREE; }
        for (int i=1; i<VERTICES; i++) { if (vertices[i]->degree!=degree) { return MAX_DEGREE; } }
        return MAX_DEGREE+1;
    } else {
        for (int i=0; i<VERTICES; i++) { if (vertices[i]->degree!=(VERTICES-1)) { return MAX_DEGREE; } }
        return MAX_DEGREE+1;
    }
}

/*******
 * RLF *
 *******/

/**
 * Creates the sets U and V needed for the RLF algorithm.
 * @param primaryVertex: the active vertex for which the sets U and V need to be created
 * @param setU: pointer to the set U that will be created in this function
 *              Set U will contain all the vertices in @param vertices that are adjacent to @param primaryVertex.
 *              Set U has size VERTICES, the vertices in set U will be placed in the first indices of the array.
 *              Remaining indices will contain NULL pointers.
 * @param setV: pointer to the set V that will be created in this function
 *              Set V will contain all the vertices in @param vertices that are not adjacent to @param primaryVertex.
 *              Set V has size VERTICES, the vertices in set V will be placed in the first indices of the array.
 *              Remaining indices will contain NULL pointers.
 * @param vertices: the vertices of the graph
 */
void createSetsUV(struct Vertex* primaryVertex, struct Vertex* (*setU), struct Vertex* (*setV), struct Vertex* (*vertices)) {
    SET_U_SIZE = 0;
    SET_V_SIZE = 0;
    struct Vertex* vertex;
    for (int i=0; i<VERTICES; i++) {
        vertex = vertices[i];
        if (vertex == primaryVertex) { continue; }
        if (vertex->color!=-1 || neighbors(vertex, primaryVertex)) {
            setU[SET_U_SIZE] = vertex;
            SET_U_SIZE++;
        } else {
            setV[SET_V_SIZE] = vertex;
            SET_V_SIZE++;
        }
    }
    for (int i=SET_U_SIZE; i<VERTICES; i++) { setU[i] = NULL; }
    for (int i=SET_V_SIZE; i<VERTICES; i++) { setV[i] = NULL; }
}

/**
 * Calculates for all the vertices in @param setV the number of neighbors it has in @param setU.
 *  Returns the vertex from @param setV that has the highest number of neighbors in @param setU.
 * @param setU: a set of vertices of the graph
 * @param setV: a set of vertices of the graph
 * @return a vertex from @param setV that has the highest number of neighbors in @param setU.
 *         In case more than one vertex in @param setV have the highest number of neighbors in @param setU,
 *         the vertex with the lowest index in @param setV is returned.
 */
struct Vertex* findSecondaryVertex(struct Vertex* (*setU), struct Vertex* (*setV)) {
    int max = -1;
    struct Vertex* secondaryVertex;
    int tempMax;
    struct Vertex* tempSecondaryVertex;
    for (int v=0; v<SET_V_SIZE; v++) {
        tempMax = 0;
        tempSecondaryVertex = setV[v];
        for (int u=0; u<SET_U_SIZE; u++) { if (neighbors(tempSecondaryVertex, setU[u])) { tempMax++; } }
        if (tempMax > max) {
            max = tempMax;
            secondaryVertex = tempSecondaryVertex;
        }
    }
    return secondaryVertex;
}

/**
 * Updates the sets @param setU and @param setV by deleting the @param secondaryVertex and its neighbors
 *  from @param setV and adding them to @param setU.
 * @param setU: a set of vertices of the graph
 *              Before the function, @param setU does not contain @param secondaryVertex.
 *              After the function, @param setU contains @param secondaryVertex and all its neighbors found in @param setV.
 * @param setV: a set of vertices of the graph
 *              Before the function, @param setV contains @param secondaryVertex.
 *              After the function, @param setV does not contain @param secondaryVertex or any of its neighbors.
 * @param secondaryVertex: the vertex that is removed along with its neighbors from @param setV and added to @param setU
 */
void updateUV(struct Vertex* (*setU), struct Vertex* (*setV), struct Vertex* secondaryVertex) {
    struct Vertex* vertex;
    int set_v_size = 0;
    for (int v=0; v<SET_V_SIZE; v++) {
        vertex = setV[v];
        if (vertex != secondaryVertex && !neighbors(vertex, secondaryVertex)) {
            setV[set_v_size] = vertex;
            set_v_size++;
        } else {
            setU[SET_U_SIZE] = vertex;
            SET_U_SIZE++;
        }
    }
    for (int v=set_v_size; v<SET_V_SIZE; v++) { setV[v] = NULL; }
    SET_V_SIZE = set_v_size;
}

/**
 * Determines an upper bound for the chromatic number based on the RLF algorithm.
 * @param vertices: the vertices of the graph
 * @return the number of colors needed for a proper vertex coloring by the RLF algorithm.
 */
int upperBoundRLF(struct Vertex* (*vertices)) {
    qsort(vertices, VERTICES, sizeof(struct Vertex*), &compareByDegree);
    int primaryVertexIndex  = selectNextVertexFF(vertices);
    struct Vertex* primaryVertex;
    struct Vertex* setU[VERTICES];
    struct Vertex* setV[VERTICES];
    struct Vertex* secondaryVertex;
    int activeColor = 0;
    while (primaryVertexIndex != -1) {
        primaryVertex = vertices[primaryVertexIndex];
        setVertexColor(primaryVertex, activeColor);
        createSetsUV(primaryVertex, setU, setV, vertices);
        while (SET_V_SIZE > 0) {
            secondaryVertex = findSecondaryVertex(setU, setV);
            setVertexColor(secondaryVertex, activeColor);
            updateUV(setU, setV, secondaryVertex);
        }
        activeColor++;
        primaryVertexIndex = selectNextVertexFF(vertices);
    }
    for (int i=0; i<VERTICES; i++) {
        struct Vertex* vertex = vertices[i];
        vertex->color = -1;
        vertex->nbRecolorings = 0;
        vertex->nbConflicts = 0;
    }
    return activeColor;
}

/******
 * WP *
 ******/

/**
 * Determines which vertices in @param vertices are uncolored and add those vertices to @param secondarySet.
 *  The vertices in @param secondarySet are placed in the first indices of the array.
 *  The remaining entries in @param secondarySet hold NULL pointers.
 * @param vertices: the vertices of the graph
 * @param secondarySet: pointer to the set that is to be constructed
 *                      Before the function, the set can be either empty or non-empty.
 *                      After the function, the set will contain all vertices from @param vertices
 *                      that are uncolored. Those vertices are placed in the first indices of the array.
 *                      The remaining entries in @param secondarySet are appointed NULL pointers.
 */
void uncoloredVertices(struct Vertex* (*vertices), struct Vertex* (*secondarySet)) {
    SECONDARY_SIZE = 0;
    for (int j=0; j<VERTICES; j++) {
        if (vertices[j]->color == -1) {
            secondarySet[SECONDARY_SIZE] = vertices[j];
            SECONDARY_SIZE++;
        }
    }
    for (int j=SECONDARY_SIZE; j<VERTICES; j++) { secondarySet[j] = NULL; }
}

/**
 * Removes the neighbors of @param vertex from @param secondarySet.
 *  The entries in @param secondarySet are reordered so that all its vertices are placed in the first indices of the array.
 *  The remaining entries in @param secondarySet are NULL pointers.
 * @param vertex: the vertex of which the neighbors are removed from @param secondarySet
 * @param secondarySet: set set from which the neighbors of @param vertex are removed
 *                      Before the function, the set may or may not contain neighbors of @param vertex and the set contains @param vertex.
 *                      After the function, the set will no longer contain any neighbors of @param vertex.
 *                      The set will still contain @param vertex itself.
 *                      All the entries in the set are placed in the first indices of the array.
 *                      The remaining entries are NULL pointers.
 */
void removeNeighbors(struct Vertex* vertex, struct Vertex* (*secondarySet)) {
    int j = 0;
    for (int i=0; i<SECONDARY_SIZE; i++) {
        if (vertex != secondarySet[i] && !neighbors(vertex, secondarySet[i])) {
            secondarySet[j] = secondarySet[i];
            j++;
        }
    }
    for (int i=j; i<SECONDARY_SIZE; i++) { secondarySet[i] = NULL; }
    SECONDARY_SIZE = j;
}

/**
 * Determines an upper bound for the chromatic number based on the WP algorithm.
 * @param vertices: the vertices of the graph
 * @return the number of colors needed for a proper vertex coloring by the WP algorithm.
 */
int upperBoundWP(struct Vertex* (*vertices)) {
    qsort(vertices, VERTICES, sizeof(struct Vertex*), &compareByDegree);
    int primaryVertexIndex  = selectNextVertexFF(vertices);
    struct Vertex* primaryVertex;
    int activeColor = 0;
    struct Vertex* secondarySet[VERTICES];
    struct Vertex* secondaryVertex;
    while (primaryVertexIndex != -1) {
        primaryVertex = vertices[primaryVertexIndex];
        setVertexColor(primaryVertex, activeColor);
        uncoloredVertices(vertices,secondarySet);
        removeNeighbors(primaryVertex, secondarySet);
        secondaryVertex = secondarySet[0];
        while (secondaryVertex != NULL) {
            setVertexColor(secondaryVertex, activeColor);
            removeNeighbors(secondaryVertex, secondarySet);
            secondaryVertex = secondarySet[0];
        }
        primaryVertexIndex = selectNextVertexFF(vertices);
        activeColor++;
    }
    for (int i=0; i<VERTICES; i++) {
        struct Vertex* vertex = vertices[i];
        vertex->color = -1;
        vertex->nbRecolorings = 0;
        vertex->nbConflicts = 0;
    }
    return activeColor;
}

/******************
 * GENERAL BOUNDS *
 ******************/

/**
 * Determines the upper bound for the chromatic number based on the algorithm configuration BOUNDS.
 * @param vertices: the vertices of the graph
 * @return if BOUNDS is set to BROOKS: the upper bound for the chromatic number based on Brooks' Theorem
 *         if BOUNDS is set to RLF: the number of colors needed for a proper vertex coloring by using the RLF algorithm
 *         if BOUNDS is set to WP: the number of colors needed for a proper vertex coloring by using the WP algorithm
 *         else: 64, which is not an upper bound exactly, but the maximal number of colors the program can handle
 */
int getUpperBound(struct Vertex* (*vertices)) {
    printf("\tMAX_DEGREE:\t\t%d\n", MAX_DEGREE);
    if (BOUNDS == BROOKS) {
        int bound = brooksUpperBound(vertices);
        printf("\tBROOKS BOUND:\t%d\n\n", bound);
        return bound;
    }
    if (BOUNDS == RLF) {
        int bound = upperBoundRLF(vertices);
        printf("\tRLF BOUND:\t%d\n\n", bound);
        return bound;
    }
    if (BOUNDS == WP) {
        int bound = upperBoundWP(vertices);
        printf("\tWP BOUND:\t%d\n\n", bound);
        return bound;
    }
    return 64;
}

#endif