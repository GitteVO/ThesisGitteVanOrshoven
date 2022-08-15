#ifndef SEARCH_C
#define SEARCH_C

#include "coloring.c"
#include "bounds.c"

int CHROMATIC_NUMBER = -1;
int STARTING_INTERVAL;
const int INIT_STARTING_INTERVAL = 64;
bool PRINT_INTERVALS = true;
bool CONNECTED_GRAPH = false;

/**
 * Search the chromatic number of the graph according to the algorithm configurations.
 * The chromatic number of the graph is stored in the local variable CHROMATIC_NUMBER.
 * Frees the @param vertices (and the linked lists stored in those vertices) before returning.
 * @param vertices: vertices of the connected graph
 */
void connectedSearch(struct Vertex* (*vertices)) {
    initializeCounters();
    int upperBound;
    if (SEARCH == GREEDY) {
        upperBound = 1;
        if (PRINT_INTERVALS) { printf("\tSEARCH:\n\tinterval:\t(0,infty)\t-> try %d colors\n", upperBound); }
        while (1) {
            if (upperBound > 64) { printf("\t-> too much colors\n"); return; }
            /* RESET GRAPH */
            resetGraph(vertices);
            setAvailableColors(vertices, upperBound); // color set [0,CHROMATIC_NUMBER)
            initialSorting(vertices);
            /* FIND COLORING*/
            if (coloring(vertices) != -1) { break; }
            if (PRINT_INTERVALS) { printf("\tnew interval:\t(%d,infty)\t-> try %d colors\n", upperBound, upperBound+1); }
            upperBound++;
        }
        if (PRINT_INTERVALS) { printf("\n"); }
    } else {
        /* BOUNDS OF THE CHROMATIC NUMBER */
        int bound = getUpperBound(vertices);
        initializeCounters();
        if (BOUNDS == BROOKS && bound == MAX_DEGREE+1) { CHROMATIC_NUMBER = (bound>CHROMATIC_NUMBER) ? bound : CHROMATIC_NUMBER; return; }
        STARTING_INTERVAL = (bound < INIT_STARTING_INTERVAL) ? bound : INIT_STARTING_INTERVAL;
        int lowerBound = 0;
        /* DETERMINE UPPER BOUND */
        if (SEARCH == BINARY) {
            upperBound = (STARTING_INTERVAL<INIT_STARTING_INTERVAL) ? STARTING_INTERVAL : STARTING_INTERVAL*2;
            if (PRINT_INTERVALS) { printf("\tSEARCH:\n\tinterval:\t(0,%d]  ", upperBound); }
        } else if (SEARCH == GREBIN) {
            upperBound = 1;
            if (PRINT_INTERVALS) { printf("\tSEARCH:\n\tinterval:\t(0,%d?  ",upperBound); }
            while (1) {
                if (PRINT_INTERVALS) { printf("\t-> try %d colors\n", upperBound); }
                /* RESET GRAPH */
                resetGraph(vertices);
                setAvailableColors(vertices, upperBound); // color set [0,upperBound)
                initialSorting(vertices);
                /* FIND COLORING*/
                int colorable = coloring(vertices);
                if (colorable != -1) {
                    upperBound = (colorable < upperBound) ? colorable : upperBound;
                    if (PRINT_INTERVALS) { printf("\tinterval:\t(%d,%d]  ", lowerBound, upperBound); }
                    break;
                }
                else {
                    lowerBound = upperBound;
                    if (lowerBound >= 64) { printf("\t-> too much colors needed\n"); }
                    if (upperBound*2<bound) { upperBound = upperBound*2; }
                    else {
                        upperBound = bound;
                        if (PRINT_INTERVALS) { printf("\tinterval:\t(%d,%d]  ", lowerBound, upperBound); }
                        break;
                    }
                }
                if (PRINT_INTERVALS) { printf("\tnew interval:\t(%d,%d?  ", lowerBound, upperBound); }
            }
        }
        /* BINARY SEARCH */
        int newK;
        while (upperBound > lowerBound+1) {
            /* NEW INTERVAL */
            newK = (lowerBound+upperBound)/2;
            if (newK > 64) { printf("\t-> too much colors needed\n"); }
            if (PRINT_INTERVALS) { printf("\t-> try %d colors\n", newK); }
            /* RESET GRAPH */
            resetGraph(vertices);
            setAvailableColors(vertices, newK); // color set [0,newK)
            initialSorting(vertices);
            /* FIND COLORING*/
            int colorable = coloring(vertices);
            if (colorable != -1) { upperBound = (colorable < newK) ? colorable : newK; } //chromatic number in lower half
            else { lowerBound = newK; } //chromatic number in upper half
            if (PRINT_INTERVALS) { printf("\tnew interval:\t(%d,%d]  ", lowerBound, upperBound); }
        }
        if (PRINT_INTERVALS) { printf("\n\n"); }
    }
    /* TERMINATE SEARCH */
    freeVertices(vertices);
    if (upperBound > CHROMATIC_NUMBER) { CHROMATIC_NUMBER = upperBound; }
}

/**
 * Search the chromatic number by first checking if the graph is connected.
 * If it is connected, start the search.
 * If it is not connected, create a subgraph of the first component of the graph and start the search for that subgraph.
 *      Afterwards, start the search for the remaining vertices.
 * The chromatic number of the graph is stored in the local variable CHROMATIC_NUMBER.
 * @param vertices: vertices of the, not necessarily connected, graph
 */
void disconnectedSearch(struct Vertex* (*vertices)) {
    struct Vertex* subGraph1[VERTICES];
    struct Vertex* subGraph2[VERTICES];
    int subGraphSize=0;
    subGraph(vertices, subGraph1, subGraph2, &subGraphSize);
    if (subGraphSize != VERTICES) {
        printf("disconnected\n");
        int remainingVertices = VERTICES-subGraphSize;
        setNbVertices(subGraphSize);
        if (subGraphSize <= CHROMATIC_NUMBER) { freeVertices(subGraph1); }  // no need to calculate X(G) of a subgraph with less nodes that the current minimally needed colors
        else { connectedSearch(subGraph1); }
        setNbVertices(remainingVertices);
        disconnectedSearch(subGraph2);
    } else {
        connectedSearch(vertices);
    }
}

/**
 * Start the search for the chromatic number.
 * The chromatic number of the graph is stored in the local variable CHROMATIC_NUMBER.
 */
int search(struct Vertex* (*vertices)) {
    STARTING_INTERVAL = INIT_STARTING_INTERVAL;
    CHROMATIC_NUMBER = -1;
    if (CONNECTED_GRAPH) { connectedSearch(vertices); }
    else { disconnectedSearch(vertices); }
    return CHROMATIC_NUMBER;
}

#endif