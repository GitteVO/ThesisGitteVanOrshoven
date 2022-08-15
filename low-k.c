#ifndef LOW_K_C
#define LOW_K_C

#include "graph.c"
#include "coloring.c"

int kColoringDisconnected(struct Vertex* (*vertices), int k);
int twoColoring(struct Vertex* (*vertices));

/*****************
 * BRON KERBOSCH *
 *****************/

/**
 * Creates a subgraph of all vertices in @param vertices that are not present in @param graph2
 *  and stores it in @param result.
 *  The global variable VERTICES is set to the number of vertices in @param result.
 * @param result: pointer to where the subgraph should be stored
 * @param vertices: the vertices of the graph
 * @param graph2: the vertices that cannot be included in the subgraph
 */
void createSubGraph(struct Vertex* (*result), struct Vertex* (*vertices), uint64_t (*graph2)) { // result := vertices\graph2 & VERTICES = size(result)
    for (int i=0; i<VERTICES; i++) { result[i] = createVertex(i); }
    for (int i=0; i<VERTICES-1; i++) {
        if ((uint64_t)1<<(i%64) & graph2[i/64]) { continue; }
        for (int j=i+1; j<VERTICES; j++) {
            if ((uint64_t)1<<(j%64) & graph2[j/64]) { continue; }
            if ((vertices[i]->neighbors[j/64] & ((uint64_t)1<<(j%64)))) { addEdgeDouble(result[i], result[j]); }
        }
    }
    removeIsolatedVertices(result);
}

/**
 * Finds the vertex with the highest degree present in @param bitarray
 * @param vertices: the vertices of the graph
 * @param bitarray: bitset of vertex candidates
 * @return the vertex with the highest degree present in @param bitarray
 */
int findHighestDegree(struct Vertex* (*vertices), uint64_t (*bitarray)) {
    int maxDegree = 0;
    int vertex;
    for (int i=0; i<BIT_ARRAY_SIZE; i++) {
        for (int64_t j=0; j<64; j++) {
            int vertexIndex = i*64+j;
            if ((bitarray[i] & (uint64_t)1<<j) && (vertices[vertexIndex]->degree > maxDegree)) {
                maxDegree = vertices[vertexIndex]->degree;
                vertex = vertexIndex;
            }
        }
    }
    return vertex;
}

/**
 * Uses the Bron-Kerbosch algorithm to find cliques in the dual graph @param complement and subsequently determines
 *  if @param vertices without the vertices from the clique of @param complement can be colored with k-1 colors.
 * @param vertices: the vertices of the graph
 * @param complement: the vertices of the dual graph
 * @param r: bitset denoting which vertices of @param complement are present in r
 *           a vertex of @param complement is present in r <=> r[vertex->number/64] & ((uint64_t)1<<(vertex->number%64))
 * @param p: bitset denoting which vertices of @param complement are present in p
 *           a vertex of @param complement is present in p <=> p[vertex->number/64] & ((uint64_t)1<<(vertex->number%64))
 * @param x: bitset denoting which vertices of @param complement are present in x
 *           a vertex of @param complement is present in x <=> x[vertex->number/64] & ((uint64_t)1<<(vertex->number%64))
 * @param k: number of colors allowed to color @param vertices
 * @param smallestClique: bitset that contains the vertices present in the smallest clique found so far
 * @param sizeClique: size of @param smallestClique
 * @return if the graph @param vertices can be k-colored: k
 *         else: -1
 */
int bronKerbosch(struct Vertex* (*vertices), struct Vertex* (*complement), uint64_t (*r), uint64_t (*p), uint64_t (*x), int k, uint64_t (*smallestClique), int sizeClique) {
    if (k!=3 && k!=4) { printf("k=%d\n",k); exit(10); }
    int sizeP = 0;
    int sizeX = 0;
    int sizeR = 0;
    for (int i=0; i<BIT_ARRAY_SIZE; i++) {
        sizeP += __builtin_popcount(p[i]);
        sizeX += __builtin_popcount(x[i]);
        sizeR += __builtin_popcount(r[i]);
    }
    if (sizeP == 0) {
        if (sizeX == 0) {// R is maximal clique
            // possibility to enforce an order on the maximal cliques
            int tempVertices = VERTICES;
            struct Vertex* subGraph[VERTICES];
            createSubGraph(subGraph, vertices, r);
            if (VERTICES == 0) { return 1; }
            int colorable = kColoringDisconnected(subGraph,k-1);
            setNbVertices(tempVertices);
            return colorable;
        }
        return -1;
    }
    int pivotIndex = findHighestDegree(complement, p);
    struct Vertex* pivot = complement[pivotIndex];
    int i=1;
    int vertexIndex = getKthSetBitPos(p, i);
    while (vertexIndex != -1) {
        struct Vertex* vertex = complement[vertexIndex];
        if (!(pivot->neighbors[vertexIndex/64] & ((uint64_t)1<<(vertexIndex%64)))) {
            uint64_t newR[BIT_ARRAY_SIZE];
            uint64_t newP[BIT_ARRAY_SIZE];
            uint64_t newX[BIT_ARRAY_SIZE];
            for (int j=0; j<BIT_ARRAY_SIZE; j++) {
                newR[j] = r[j];
                newP[j] = p[j] & vertex->neighbors[j];
                newX[j] = x[j] & vertex->neighbors[j];
            }
            newR[vertexIndex/64] |= (uint64_t)1<<(vertexIndex%64);
            if (bronKerbosch(vertices, complement, newR, newP, newX, k, smallestClique, sizeClique) != -1) { return 1; }
            p[vertexIndex/64] = p[vertexIndex/64] &~ ((uint64_t)1<<(vertexIndex%64));
            x[vertexIndex/64] = x[vertexIndex/64] |  ((uint64_t)1<<(vertexIndex%64));
            sizeP--;
            vertexIndex = getKthSetBitPos(p, i);
        } else {
            i++;
            vertexIndex = getKthSetBitPos(p, i);
        }
    }
    return -1;
}

/**************
 * K COLORING *
 **************/

/**
 * Creates the complement of the graph @param vertices and stores it in @param complement.
 * @param complement: pointer to where the complement graph is to be stored
 * @param vertices: the vertices of the graph
 */
void createComplement(struct Vertex* (*complement), struct Vertex* (*vertices)) {
    struct Vertex* vertex;
    struct Vertex* complementVertex;
    for (int i=0; i<VERTICES; i++) {
        vertex = vertices[i];
        complementVertex = createVertex(i);
        for (int j=0; j<BIT_ARRAY_SIZE-1; j++) { complementVertex->neighbors[j] = -(vertex->neighbors[j])-1; }
        complementVertex->neighbors[BIT_ARRAY_SIZE-1] = (-(vertex->neighbors[BIT_ARRAY_SIZE-1])-1)<<(64-VERTICES%64)>>(64-VERTICES%64);
        complementVertex->neighbors[i/64] &=~ ((uint64_t)1<<(i%64));
        complementVertex->degree = VERTICES - vertex->degree - 1;
        if (complementVertex->degree == 0) {
            free(complementVertex);
            complement[i] = NULL;
        } else {
            complement[i] = complementVertex;
        }
    }
}

/**
 * Determines if the graph can be k-colored.
 * @param vertices: the vertices of a connected graph
 * @param k: the number of colors
 * @return if the graph can be k-colored: @param k
 *         else: -1
 */
int kColoring(struct Vertex* (*vertices), int k) {
    if (k==2) return twoColoring(vertices);
    if (k!=3 && k!=4) { printf("k=%d\n",k); exit(10); }
    int temp_vertices = VERTICES;
    struct Vertex* complement[VERTICES];
    createComplement(complement, vertices);
    uint64_t r[BIT_ARRAY_SIZE];
    uint64_t p[BIT_ARRAY_SIZE];
    uint64_t x[BIT_ARRAY_SIZE];
    for (int i=0; i<BIT_ARRAY_SIZE; i++) {
        r[i] = (uint64_t)0;
        p[i] = (uint64_t)0;
        x[i] = (uint64_t)0;
    }
    for (int i=0; i<VERTICES; i++) {
        if (complement[i] != NULL) { p[i/64] |= (uint64_t)1<<(i%64); }
    }
    uint64_t firstClique[BIT_ARRAY_SIZE];
    bool coloring = (bronKerbosch(vertices, complement, r, p, x, k, firstClique, VERTICES) != -1);
    setNbVertices(temp_vertices);
    return coloring ? k : -1;
}

/**
 * Determines whether the graph can be k-colored.
 *  If the @param vertices are connected, the function will check if it can be k-colored.
 *  If the @param vertices are not connected, the function creates a component of the graph and checks if it can be k-colored.
 *      If the component can be k-colored, the function will recursively call itself with the remaining
 *      vertices (graph \ component) to determine if the remaining vertices can be k-colored.
 * @param vertices: the vertices of the, not necessarily connected, graph
 * @param k: the number of colors
 * @return if the graph can be k-colored: @param k
 *         else: -1
 */
int kColoringDisconnected(struct Vertex* (*vertices), int k) {
    if (k !=2 && k != 3) { printf("k=%d\n",k); exit(10); }
    struct Vertex* subGraph1[VERTICES];
    struct Vertex* subGraph2[VERTICES];
    int subGraphSize=0;
    subGraph(vertices, subGraph1, subGraph2, &subGraphSize);
    if (subGraphSize != VERTICES) {
        int remainingVertices = VERTICES-subGraphSize;
        setNbVertices(subGraphSize);
        if (subGraphSize > k && kColoring(subGraph1, k) == -1) { return -1; }
        setNbVertices(remainingVertices);
        return kColoringDisconnected(subGraph2, k);
    } else {
        return kColoring(subGraph1, k);
    }
}

/**************
 * 2 COLORING *
 **************/

/**
 * Colors @param vertex with @param color and tries to color all neighbors of @param vertex with the opposite color.
 * @param vertex: the vertex to be colored with @param color
 * @param color: the color to color @param vertex
 * @return if all neighbors of @param vertex can be colored with the opposite color of @param color: 1
 *         else: -1
 */
int twoColorVertex(struct Vertex* vertex, int color) {
    vertex->color = color;
    struct ListElem* neighbor = vertex->adjacentVertices;
    while (neighbor!=NULL) {
        if (neighbor->vertex->color == color) { return -1; }
        else if (neighbor->vertex->color == -1) { if (twoColorVertex(neighbor->vertex, 1-color) == -1) { return -1; }}
        neighbor = neighbor->next;
    }
    return 1;
}

/**
 * Determines if the graph can be colored using two colors.
 * @param vertices: the vertices in the graph
 * @return if graph can be 2-colored: 2
 *         else: -1
 */
int twoColoring(struct Vertex* (*vertices)) {
    if (twoColorVertex(vertices[0], 0) == 1) { return 2; }
    return -1;
}

#endif