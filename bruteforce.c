#include "bounds.c"
#include "search.c"
#include "heuristics.c"
#include "coloring.c"
#include "graph.c"
#include <time.h>

/**
 * Possible graph colorings
 */
enum coloring{VERTEX_COLORING,EDGE_COLORING,TOTAL_COLORING};
/**
 * Graph coloring to execute
 */
enum coloring coloringType = VERTEX_COLORING;

/**
 * Decision tree for automatic algorithm configurations
 */
void configurations() {
    if      (BALANCE>1.5 && BALANCE<=1.75) { SEARCH = GREEDY;     }
    else                                   { SEARCH = EXHAUSTIVE; }

    if (SEARCH == BINARY) {
        if      (VERTICES<=220) { BOUNDS = WP;     }
        else if (VERTICES<=290) { BOUNDS = RLF;    }
        else                    { BOUNDS = BROOKS; }
    } else if (SEARCH == GREBIN) {
        if      (BALANCE<=1.25) { BOUNDS = WP;     }
        else if (BALANCE<=2.25) { BOUNDS = BROOKS; }
        else                    { BOUNDS = WP;     }
    } else if (SEARCH == EXHAUSTIVE) {
        if      (VERTICES<=80)  { BOUNDS = RLF;    }
        else if (VERTICES<=290) { BOUNDS = WP;     }
        else                    { BOUNDS = BROOKS; }
    } else { BOUNDS = NO; }

    if      (VERTICES<=120) { SORTING = RECOLOR;  }
    else if (VERTICES<=250) { SORTING = DSATUR;   }
    else if (VERTICES<=300) { SORTING = CONFLICT; }
    else                    { SORTING = DSATUR;   }

    if (SORTING == VERTEX) {
        SORTING_RATE = 0;
        if      (VERTICES<=40)  { CS = true;  }
        else if (VERTICES<=400) { CS = false; }
        else                    { CS = true;  }
    } else if (SORTING == DEGREE) {
        SORTING_RATE = 0;
        if      (VERTICES<=100) { CS = true;  }
        else if (VERTICES<=200) { CS = false; }
        else                    { CS = true;  }
    } else if (SORTING == IDO) {
        CS = false; // CS implicitly ensured, not necessary to check condition
        if      (VERTICES<=70)  { SORTING_RATE = 0; }
        else if (VERTICES<=270) { SORTING_RATE = 4; }
        else                    { SORTING_RATE = 2; }
    } else if (SORTING == DSATUR) {
        CS = false; // CS implicitly ensured, not necessary to check condition
        if      (VERTICES<=50)  { SORTING_RATE = 6; }
        else if (VERTICES<=100) { SORTING_RATE = 0; }
        else                    { SORTING_RATE = 2; }
    } else if (SORTING == RECOLOR) {
        if      (BALANCE<=2.75) { DECAY_FACTOR = 0.4;  }
        else if (BALANCE<=3.5)  { DECAY_FACTOR = 0.1;  }
        else                    { DECAY_FACTOR = 0.95; }
        if      (DENSITY<=0.05) { SORTING_RATE = 1; CS = false; }
        else if (DENSITY<=0.5)  { SORTING_RATE = 2; CS = false; }
        else                    { SORTING_RATE = 3; CS = true;  }
    } else if (SORTING == CONFLICT) {
        if      (VERTICES<=80)  { DECAY_FACTOR = 0.45; }
        else if (VERTICES<=160) { DECAY_FACTOR = 0.35; }
        else                    { DECAY_FACTOR = 1;    }
        if      (DENSITY<=0.15) { SORTING_RATE = 2; CS = false; }
        else if (DENSITY<=0.35) { SORTING_RATE = 5; CS = true;  }
        else                    { SORTING_RATE = 1; CS = false; }
    }

    /* BEST STATIC CONFIGURATION */

//    SEARCH = EXHAUSTIVE;
//    BOUNDS = RLF;
//
//    SORTING = CONFLICT;
//    DECAY_FACTOR = 1;
//    SORTING_RATE = 1;
//    CS = false;

}
 /**
  * Function to print the algorithm configurations
  */
void printConfigurations() {
    printf("\tCONFIGURATIONS:\n");
    if      (SEARCH == KCOLORING)  { printf("\tsearch:   %d-COLORING\n", MAX_COLOR+1); }
    else if (SEARCH == BINARY)     { printf("\tsearch:   BINARY\n"); }
    else if (SEARCH == GREEDY)     { printf("\tsearch:   GREEDY\n"); }
    else if (SEARCH == GREBIN)     { printf("\tsearch:   GREBIN\n"); }
    else if (SEARCH == EXHAUSTIVE) { printf("\tsearch:   EXHAUSTIVE\n"); }

    if      (BOUNDS == NO)     { printf("\tbounds:   NO\n"); }
    else if (BOUNDS == BROOKS) { printf("\tbounds:   BROOKS\n"); }
    else if (BOUNDS == WP)     { printf("\tbounds:   WP\n"); }
    else if (BOUNDS == RLF)    { printf("\tbounds:   RLF\n"); }

    if (SORTING == VERTEX) {
        if (CS) { printf("\tsorting:  CS VERTEX\n"); }
        else    { printf("\tsorting:  VERTEX\n"); }
    } else if (SORTING == DEGREE) {
        if (CS) { printf("\tsorting:  CS DEGREE\n"); }
        else    { printf("\tsorting:  DEGREE\n"); }
    } else if (SORTING == IDO) {
                  printf("\tsorting:  IDO\n");
                  printf("\tsortrate: %d\n", SORTING_RATE);
    } else if (SORTING == DSATUR) {
                  printf("\tsorting:  DSATUR\n");
                  printf("\tsortrate: %d\n", SORTING_RATE);
    } else if (SORTING == RECOLOR) {
        if (CS) { printf("\tsorting:  CS RECOLOR\n"); }
        else    { printf("\tsorting:  RECOLOR\n"); }
                  printf("\tsortrate: %d\n", SORTING_RATE);
                  printf("\trecfac:   %.2f\n", DECAY_FACTOR);
    } else if (SORTING == CONFLICT) {
        if (CS) { printf("\tsorting:  CS CONFLICT\n"); }
        else    { printf("\tsorting:  CONFLICT\n"); }
                  printf("\tsortrate: %d\n", SORTING_RATE);
                  printf("\tconfac:   %.2f\n", DECAY_FACTOR);
    }
    printf(    "\n");
}

/**
 * Main function that reads the graph from the input file and returns the chromatic number of the graph
 */
int main(int argc, char** argv) {
    clock_t start = clock();
    double cpu_time_used;

    /* INPUT VARIABLES */
    if (argc<2) { exit(10); }
    char filename[100];
    strncpy(filename, argv[1], 100);
    setNbVertices(atoi(argv[2]));
    struct Vertex* vertices[VERTICES];
    for (int v=0; v<VERTICES; v++) { vertices[v] = createVertex(v); }

    /* READ GRAPH */
    readGraph(filename, vertices);
    /* SET CONFIGURATIONS */
    configurations();
    removeIsolatedVertices(vertices);
    if (coloringType == EDGE_COLORING) {
        int vertex_max_degree = MAX_DEGREE;
        struct Vertex* lineGraph[EDGES];
        createLineGraph(vertices, lineGraph);
        int edges_temp = EDGES;
        EDGES = VERTICES;
        setNbVertices(edges_temp);
        setAvailableColors(lineGraph, vertex_max_degree);
        printf("%s\n", filename);
        int result = coloring(lineGraph);
        cpu_time_used = ((double) (clock()-start)) / CLOCKS_PER_SEC;
        if (result == -1) {
            printf(    "\tRESULT:\n\tX'(G)=%d\n\tduration: %f seconds\n", vertex_max_degree+1, cpu_time_used);
        }
        else {
            printf(    "\tRESULT:\n\tX'(G)=%d\n\tduration: %f seconds\n", vertex_max_degree, cpu_time_used);
        }
        return 1;
    } else if (coloringType == TOTAL_COLORING) {
        printf(    "%s\n\ttotal coloring not supported\n", filename);
        return -1;
    } else {
        printf("%s\n", filename);
        if (SEARCH == KCOLORING) {
            if (argc<3) { printf("\tno value for k provided\n"); return 0; }
            int colors = atoi(argv[3]);
            printf("\tVERTICES:\t%d\n\tEDGES:\t%d\n\n", VERTICES, EDGES);
            if (colors<=0) {
                cpu_time_used = ((double) (clock()-start)) / CLOCKS_PER_SEC;
                printf(    "\tRESULT:\n\t%d-coloring not possible \n\tduration: %f seconds\n", colors, cpu_time_used);
                return 0;
            } else if (colors>=VERTICES) {
                cpu_time_used = ((double) (clock()-start)) / CLOCKS_PER_SEC;
                printf(    "\tRESULT:\n\t%d-coloring possible \n\tduration: %f seconds\n", colors, cpu_time_used);
                return 0;
            } else if (colors>64) {
                printf(    "\tfailed: too much colors\n");
                return 0;
            }
            setAvailableColors(vertices, colors); // color set = [0,colors)
        }

        printConfigurations();

        /* EXECUTE ALGORITHM */
        int result;
        if (SEARCH == KCOLORING || SEARCH == EXHAUSTIVE) {  result = coloring(vertices);  }
        else {  result = search(vertices);  }
        cpu_time_used = ((double) (clock()-start)) / CLOCKS_PER_SEC;
        if (SEARCH == KCOLORING) {
            if (result != -1) {
                printf(    "\tRESULT:\n\t%d-coloring possible     \n\tduration: %f seconds\n", MAX_COLOR+1, cpu_time_used);
            } else {
                printf(    "\tRESULT:\n\t%d-coloring not possible \n\tduration: %f seconds\n", MAX_COLOR+1, cpu_time_used);
            }
        } else if (SEARCH == EXHAUSTIVE && result == 128) {
            printf(    "\tRESULT:\n\tX(G)>64 \n\tduration: %f seconds\n", cpu_time_used);
        } else {
            printf(    "\tRESULT:\n\tX(G)=%d \n\tduration: %f seconds\n", result, cpu_time_used);
        }
    }
}