#include "../bruteforce/graph.c"
#include "../bruteforce/heuristics.c"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <time.h>
#include <stdint.h>
#include <math.h>

#define difference(set1, set2) ((set1) & ~(set2))

int TABLE_SIZE;
int SUBSETS;

void print32Bits(int number) {
    for (int64_t i=31; i>=0; i--) {
        int testbit = 1<<i;
        if (number & testbit) {
            printf("1");
        } else {
            printf("0");
        }
    }
}

void printSubgraphTable(int (*subgraphTable)) {
    for (int i=0; i<TABLE_SIZE; i++) {
        print32Bits(subgraphTable[i]);
        printf("\t%d bits", __builtin_popcount(subgraphTable[i]));
        printf("\n");
    }
}

int compareByPopCount(const void* int1, const void* int2) {
    return (__builtin_popcount(*(int*) int1) - __builtin_popcount(*(int*) int2));
}

/**
 * Creates a table with all permutations of active vertices.
 * Sorts the table by non-decreasing number of active vertices.
 * @param subgraphTable: the table to create
 */
void createTable(int (*subgraphTable)){
    for (int i=0; i<TABLE_SIZE; i++) { subgraphTable[i] = i; }
    qsort(subgraphTable, TABLE_SIZE, sizeof(int), compareByPopCount);
}

void setTableSize() {
    TABLE_SIZE = 1<<VERTICES;
}

/**
 * Checks if the vertices of the graph @param vertices listed in @param subset form an independent set.
 * @return true is no pair of vertices in @param subset are connected in the graph, false otherwise.
 */
bool independentSet(int subset, struct Vertex* (*vertices)) {
    int nbSetBits = __builtin_popcount(subset);
    if (nbSetBits<2) { return true; }
    struct Vertex* subsetVertices[nbSetBits];
    int counter = 0;
    int counter1 = 0;
    while (subset != 0) {
        if (subset%2 == 1) {
            subsetVertices[counter] = vertices[counter1];
            counter++;
        }
        subset = subset>>1;
        counter1++;
    }
    for (int i=0; i<nbSetBits-1; i++) {
        for (int j=i+1; j<nbSetBits; j++) {
            if (neighbors(subsetVertices[i], subsetVertices[j])) { return false; }
        }
    }
    return true;
}

bool isSubgraph(int graph, int subgraph) {
    return ((subgraph&~graph) == 0);
}

/**
 * Finds all the independent subsets of the @param subgraph and store them in @param subsets.
 */
void getSubsets(int subgraph, struct Vertex* (*vertices), int (*subsets), const int (*subgraphTable)) {
    /* get all subsets */
    SUBSETS = 0;
    int subgraphSize = __builtin_popcount(subgraph);
    for (int i=1; i<TABLE_SIZE; i++) {
        if (__builtin_popcount(subgraphTable[i])>subgraphSize) { break; }
        if (isSubgraph(subgraph, subgraphTable[i])) {
            subsets[SUBSETS] = subgraphTable[i];
            SUBSETS++;
        }
    }
    for (int i=SUBSETS; i<TABLE_SIZE; i++) { subsets[i] = 0; }
    /* remove non-independent sets */
    int j=0;
    for (int i=0; i<SUBSETS; i++) {
        if (independentSet(subsets[i], vertices)) {
            subsets[j] = subsets[i];
            j++;
        }
    }
    for (int i=j; i<SUBSETS; i++) { subsets[i] = 0; }
    SUBSETS = j;
}

/**
 * Calculates the entries of the @param chromaticTable by using the formula:
 *      T(W) = 1 + minT(W \ S), where the minimum is taken over all non-empty independent sets S in the graph.
 */
void calculateChromaticTable(const int (*subgraphTable), int (*chromaticTable), struct Vertex* (*vertices)) {
    chromaticTable[0] = 0;
    int minimum;
    int subgraph;
    int subsets[TABLE_SIZE];
    int tws;
    for (int i=1; i<TABLE_SIZE; i++) {
        minimum = VERTICES;
        subgraph = subgraphTable[i];
        getSubsets(subgraph, vertices, subsets, subgraphTable);
        for (int j=0; j<SUBSETS; j++) {
            tws = chromaticTable[difference(subgraph, subsets[j])];
            minimum = (tws<minimum) ? tws : minimum;
        }
        chromaticTable[subgraph] = 1+minimum;
    }
}

/**
 * Computes the chromatic number of the graph by using the dynamic programming principle.
 * @param vertices: the vertices of the graph
 * @return the chromatic number of the graph.
 */
int dynamic(struct Vertex* (*vertices)) {
    int chromaticTable[TABLE_SIZE];
    int subgraphTable[TABLE_SIZE];
    createTable(subgraphTable);
    calculateChromaticTable(subgraphTable, chromaticTable, vertices);
    return chromaticTable[TABLE_SIZE-1];
}

/**
 * Main function to calculate the chromatic number by dynamic programming.
 * @return the chromatic number of the graph.
 */
int main(int argc, char** argv) {
    clock_t startBisection = clock();
    clock_t endBisection;
    double cpu_time_used;
    /* INPUT VARIABLES */
    char filename[100];
    strncpy(filename, argv[1], 100);
    setNbVertices(atoi(argv[2]));
    setTableSize();
    struct Vertex* vertices[VERTICES];
    for (int v=0; v<VERTICES; v++) { vertices[v] = createVertex(v); }
    readGraph(filename, vertices);

    printf("%s\t\t%d", filename, VERTICES);

    int chromaticNumber = dynamic(vertices);
    printf("\tX(G)=%d", chromaticNumber);
    endBisection = clock();
    cpu_time_used = ((double) (endBisection-startBisection)) / CLOCKS_PER_SEC;
    printf("\tduration: %f seconds\n", cpu_time_used);
    return chromaticNumber;
}