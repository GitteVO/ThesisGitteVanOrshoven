#ifndef GRAPH_C
#define GRAPH_C

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * Number of vertices of the graph
 */
int VERTICES;

/**
 * Size of array of bivectors; BIT_ARRAY_SIZE = (VERTICES+63)/64
 */
int BIT_ARRAY_SIZE;

/**
 * Number of edges of the graph
 */
int EDGES;

/**
 * Average degree of the graph
 */
float AVG_DEGREE;

/**
 * Maximal degree of the graph
 */
int MAX_DEGREE;

/**
 * Density of the graph; DENSITY = AVG_DEGREE / VERTICES
 */
float DENSITY;

/**
 * Balance of the graph; BALANCE = MAX_DEGREE / AVG_DEGREE
 */
float BALANCE;

/**
 * Highest color allowed for a proper coloring
 */
int MAX_COLOR; //highest allowed color; color range: [0..MAX_COLOR]

int VERTEX_COUNTER;

/**
 * Highest used color so far
 * @invariant: MAX_USED<=MAX_COLOR
 */
int MAX_USED;

/**
 * Highest used color for the best proper coloring so far
 * @invariant: chromatic number <= CHROMATIC+1
 */
int CHROMATIC;

/**
 * Sets the number of vertices to @param nbVertices and updates global variable BIT_ARRAY_SIZE accordingly
 */
void setNbVertices(int nbVertices) {
    VERTICES = nbVertices;
    BIT_ARRAY_SIZE = (VERTICES+63)/64;
}

/**
 * Sets MAX_COLOR to @param maxNbColors-1.
 * The allowed colors are [0..MAX_COLOR].
 */
void setMaxColor(int maxNbColors) { MAX_COLOR = maxNbColors-1; }

/**************
 * STRUCTURES *
 **************/

struct Vertex{
    int number;
    int degree;
    int color;
    uint64_t availableColors;
    struct ListElem* adjacentVertices;
    uint64_t* neighbors;
    double nbRecolorings;
    double nbConflicts;
};

struct ListElem{
    struct Vertex* vertex;
    struct ListElem* next;
};

/**
 * Allocates memory for a new vertex, initializes it, sets the number to @param v and returns the pointer to the vertex.
 * @param v: vertex number
 * @return pointer to the newly created vertex
 */
struct Vertex* createVertex(int v) {
    struct Vertex* vertex = malloc(sizeof(struct Vertex));
    if (vertex == NULL) { printf("Could not allocate memory\n"); exit(4);}
    vertex->number = v;
    vertex->degree = 0;
    vertex->color = -1;
    vertex->availableColors = (uint64_t)0;
    vertex->adjacentVertices = NULL;
    uint64_t* neighbors = malloc(sizeof(uint64_t)*BIT_ARRAY_SIZE);
    if (neighbors == NULL) { printf("Could not allocate memory\n"); exit(4);}
    vertex->neighbors = neighbors;
    for (int i=0; i<BIT_ARRAY_SIZE; i++) { vertex->neighbors[i] = (uint64_t)0; }
    vertex->nbRecolorings = 0;
    vertex->nbConflicts = 0;
    return vertex;
}

/**
 * Create a new linked list element that stores the @param vertex
 * @param vertex: vertex to be stored in the linked list element
 * @return pointer to the newly created linked list element
 */
struct ListElem* createListElem(struct Vertex* vertex){
    struct ListElem* listElem = malloc(sizeof(struct ListElem));
    if (listElem == NULL) { printf("Could not allocate memory\n"); exit(4);}
    listElem->vertex = vertex;
    listElem->next = NULL;
    return listElem;
}

/**
 * Add the @param listElem to the front of the @param linkedList.
 * @return the pointer to the updated @param linkedList
 */
struct ListElem* addListElem(struct ListElem* listElem, struct ListElem** linkedList) {
    listElem->next = *linkedList;
    *linkedList = listElem;
    return *linkedList;
}

/**
 * Sets the color of @param vertex to @param color.
 * Increases the nbRecolorings counter of @param vertex.
 */
void setVertexColor(struct Vertex* vertex, int color) {
    vertex->color = color;
    vertex->nbRecolorings++;
}

/**
 * Adds @param v2 as a neighbor of @param v1. Does not add @param v1 as a neighbor of @param v2.
 * @param v1: vertex where @param v2 is added as a neighbor
 * @param v2: vertex that is added as neighbor of @param v1
 */
void addEdgeSingle(struct Vertex* v1, struct Vertex* v2) {
    /* add v2 as a neighbor of v1 */
    struct ListElem* listElem = createListElem(v2);
    if (listElem == NULL) { printf("Could not allocate memory\n"); exit(4);}
    listElem->next = v1->adjacentVertices;
    v1->adjacentVertices = listElem;
    v1->degree++;
    v1->neighbors[v2->number/64] |= (uint64_t)1<<(v2->number%64);
}

/**
 * Adds @param v2 as a neighbor of @param v1 and adds @param v1 as a neighbor of @param v2.
 * @param v1: vertex that is added as neighbor of @param v2
 * @param v2: vertex that is added as neighbor of @param v1
 */
void addEdgeDouble(struct Vertex* v1, struct Vertex* v2) {
    /* add v1 as a neighbor of v2 */
    struct ListElem* listElem1 = createListElem(v1);
    listElem1->next = v2->adjacentVertices;
    v2->adjacentVertices = listElem1;
    int v1nb = v1->number;
    v2->neighbors[v1nb/64] |= (uint64_t)1<<(v1nb%64);
    v2->degree++;
    /* add v2 as a neighbor of v1 */
    struct ListElem* listElem2 = createListElem(v2);
    listElem2->next = v1->adjacentVertices;
    v1->adjacentVertices = listElem2;
    int v2nb = v2->number;
    v1->neighbors[v2nb/64] |= (uint64_t)1<<(v2nb%64);
    v1->degree++;
}

/**
 * Free the linked list @param nextListElem from memory.
 */
void freeLinkedList(struct ListElem* nextListElem) {
    struct ListElem* temp;
    while(nextListElem != NULL) {
        temp = nextListElem;
        nextListElem = nextListElem->next;
        free(temp);
    }
}

/**
 * Free the @param vertices from memory.
 * Also frees the linked list "adjacentVertices" of each vertex.
 */
void freeVertices(struct Vertex* (*vertices)) {
    for (int i=0; i<VERTICES; i++) {
        freeLinkedList(vertices[i]->adjacentVertices);
        free(vertices[i]);
    }
}

/**
 * Sets the available colors of @param vertices to [0..@param maxColor).
 * Also sets global variable MAX_COLOR to @param maxColor-1.
 */
void setAvailableColors(struct Vertex* (*vertices), int maxColor) { //color set = [0,maxColor)
    setMaxColor(maxColor);
    uint64_t availability;
    if (MAX_COLOR == 63) { availability = (uint64_t)(-1); }
    else {                 availability = ((uint64_t)1<<(MAX_COLOR+1))-1; }
    for (int i=0; i<VERTICES; i++) {
        vertices[i]->availableColors = availability;
    }
}

/**
 * Updates the availableColors of all @param vertices to no longer have any color larger than or
 * equal to @param maxColor as one of their available colors.
 */
void updateAvailability(struct Vertex* (*vertices), int maxColor) {
    uint64_t update = (uint64_t)(-1) & ~(((uint64_t)1<<(maxColor))-1);
    for (int i=0; i<VERTICES; i++) {
        vertices[i]->availableColors = vertices[i]->availableColors & ~update;
    }
}

/**
 * Update the availableColors of a single @param vertex to no longer have any color larger than or
 * equal to @param maxColor as one of their available colors.
 */
void updateVertexAvailability(struct Vertex* vertex, int maxColor) {
    uint64_t update = (uint64_t)(-1) & ~(((uint64_t)1<<(maxColor))-1);
    vertex->availableColors = vertex->availableColors & ~update;
}

/**
 * Determines if a @param vertex is adjacent to at least one vertex that is colored.
 * @return true if at least one neighbor of @param vertex is colored, false otherwise
 */
bool hasColoredNeighbor(struct Vertex* vertex) {
    struct ListElem* neighbor = vertex->adjacentVertices;
    while (neighbor != NULL) {
        if (neighbor->vertex->color != -1) { return true; }
        neighbor = neighbor->next;
    }
    return false;
}

/**
 * Sets the entry in @param connectedVertices of all neighbors in the linked list @param connectedNeighbor and
 * all vertices connected to one of those neighbors to 1.
 */
void checkConnectedVertices(struct ListElem* connectedNeighbor, int *connectedVertices) {
    struct Vertex* connectedVertex;
    while (connectedNeighbor != NULL) {
        connectedVertex = connectedNeighbor->vertex;
        if (connectedVertices[connectedVertex->number] == 0) {
            connectedVertices[connectedVertex->number] = 1;
            checkConnectedVertices(connectedVertex->adjacentVertices, connectedVertices);
        }
        connectedNeighbor = connectedNeighbor->next;
    }
}

/**
 * Checks of the graph is connected.
 * @param vertices: the vertices of the graph
 * @return true if the graph is connected, false otherwise
 */
bool connectedGraph(struct Vertex* (*vertices)) {
    int connectedVertices[VERTICES];
    connectedVertices[0] = 1;
    for (int i=1; i<VERTICES; i++) { connectedVertices[i] = 0; }
    checkConnectedVertices(vertices[0]->adjacentVertices, connectedVertices);
    for (int i=1; i<VERTICES; i++) { if (connectedVertices[i] ==0) return false; }
    return true;
}

int compareByVertexNumber(const void* vertex1, const void* vertex2);

/**
 * Creates a component of @param vertices by adding the first vertex of @param vertices and all the vertices that
 * are connected to that vertex to @param subGraph1.
 * All remaining vertices are added to @param subGraph2.
 * @param subGraphSize is set to the number of vertices in @param subGraph1.
 */
void subGraph(struct Vertex* (*vertices), struct Vertex* (*subGraph1), struct Vertex* (*subGraph2), int *subGraphSize) {
    qsort(vertices, VERTICES, sizeof(struct Vertex*), &compareByVertexNumber);
    int connectedVertices[VERTICES];
    connectedVertices[0] = 1;
    for (int i=1; i<VERTICES; i++) { connectedVertices[i] = 0; }
    checkConnectedVertices(vertices[0]->adjacentVertices, connectedVertices);
    int j = 0;
    int k = 0;
    for (int i=0; i<VERTICES; i++) {
        if (connectedVertices[i] == 1) {
            *subGraphSize = *subGraphSize+1;
            subGraph1[j] = vertices[i];
            subGraph1[j]->number = j;
            j++;
        } else {
            subGraph2[k] = vertices[i];
            subGraph2[k]->number = k;
            k++;
        }
    }
}

/**
 * Removes the vertex with number @param isolatedVertex from the neighbors of all the vertices in @param vertices
 * by deleting the @param isolatedVertex-th least significant bit (by shifting all other bits).
 * E.g.: a vertex with neighbors = 0110110010 and isolatedVertex = 4 => neighbors of the vertex become 0011011010
 */
void removeIsolatedNeighbor(struct Vertex* (*vertices), int isolatedVertex) {
    struct Vertex* vertex;
    for (int i=0; i<VERTICES; i++) {
        vertex = vertices[i];
        if (vertex == NULL) { continue; }
        for (int j=isolatedVertex/64; j<BIT_ARRAY_SIZE; j++) {
            uint64_t neighborsj = vertex->neighbors[j];
            if (j==isolatedVertex/64) {
                vertex->neighbors[j] = ((isolatedVertex%64==0) ? (uint64_t)0 : (neighborsj<<(64-isolatedVertex%64)>>(64-isolatedVertex%64))) | (neighborsj>>(isolatedVertex%64+1)<<(isolatedVertex%64));
            } else {
                vertex->neighbors[j] = neighborsj>>1;
            }
            if (j<BIT_ARRAY_SIZE-1 && vertex->neighbors[j+1]%2==1) {
                vertex->neighbors[j] |= (uint64_t)1<<63;
            }
        }
    }
}

/**
 * removes all isolated vertices from @param vertices and corrects the neighbors bitarray of the remaining vertices accordingly.
 * @param vertices
 */
void removeIsolatedVertices(struct Vertex* (*vertices)) {
    int counter = 0;
    for (int i=0; i<VERTICES; i++) {
        if (vertices[i]->degree == 0) {
            removeIsolatedNeighbor(vertices, counter);
            free(vertices[i]);
            vertices[i]=NULL;
        } else {
            if (i>counter) {
                vertices[counter] = vertices[i];
                vertices[i]=NULL;
            }
            vertices[counter]->number = counter;
            counter++;
        }
    }
    setNbVertices(counter);
}

/**
 * Returns true if @param vertex1 and @param vertex2 are adjacent, false otherwise.
 */
bool neighbors(struct Vertex* vertex1, struct Vertex* vertex2) {
    if (vertex1 == NULL) {
        printf("vertex1 == NULL\n");
        exit(16);
    }
    if (vertex2 == NULL) {
        printf("vertex2 == NULL\n");
        exit(16);
    }
    struct ListElem* neighbor = vertex1->adjacentVertices;
    while (neighbor != NULL) {
        if (neighbor->vertex == vertex2) return true;
        neighbor = neighbor->next;
    }
    return false;
}

/*******************
 * PRINT FUNCTIONS *
 *******************/

void print64Bits(int64_t number) {
    for (int64_t i=63; i>=0; i--) {
        uint64_t testbit = (uint64_t)1<<i;
        if (number & testbit) {
            printf("1");
        } else {
            printf("0");
        }
    }
}

void printAvailabilities(struct Vertex* (*vertices)) {
    for (int i=0; i<VERTICES; i++) {
        printf("v%d: ", vertices[i]->number);
        print64Bits(vertices[i]->availableColors);
        printf("\n");
    }
}

void printColorsCompact(struct Vertex* (*vertices)){
    printf("[ ");
    int color;
    for (int i = 0; i<VERTICES; i++) {
        if (vertices[i] == NULL) {
            printf("XX ");
        } else {
            printf("V%d:", vertices[i]->number);
            color = vertices[i]->color;
            if (color == -1) { printf("__ "); }
            else { printf("C%d ",color); }
        }
    }
    printf("]\n");
}

void printVertexNeighbors(struct Vertex* vertex) {
    printf("v%d ", vertex->number);
    struct ListElem* neighbor = vertex->adjacentVertices;
    while (neighbor != NULL) {
        int number = neighbor->vertex->number;
        printf("-> v%d ", number);
        if (number<10) { printf(" "); }
        neighbor = neighbor->next;
    }
    printf("\n");
}

void printVertices(struct Vertex* (*vertices)) {
    printf("[ ");
    for (int i=0; i<VERTICES; i++) {
        if (vertices[i] == NULL) {
            printf("xx ");
        } else {
            printf("v%d ", vertices[i]->number);
        }
    }
    printf("]\n");
}

void printVertexDegrees(struct Vertex* (*vertices)) {
    printf("[ ");
    for (int i=0; i<VERTICES; i++) { printf("v%d:d%d ", vertices[i]->number, vertices[i]->degree); }
    printf("]\n");
}

void printAdjacencies(struct Vertex* (*vertices)) {
    for (int i=0; i<VERTICES; i++) { printVertexNeighbors(vertices[i]); }
}

void printVertexColors(struct Vertex* (*vertices)) {
    for (int v=0; v<VERTICES; v++) { printf("v%d <- c%d\n", vertices[v]->number, vertices[v]->color); }
}

void printLinkedList(struct ListElem* nextElem) {
    while (nextElem != NULL) {
        printf("%d -> ", nextElem->vertex->number);
        nextElem = nextElem->next;
    }
    printf("NULL \n");
}

/****************
 * FILE READERS *
 ****************/

/**
 * Reads a graph from a .mat input file and stores it in @param vertices.
 * Sets the global variables VERTICES, EDGES, AVG_DEGREE, MAX_DEGREE, DENSITY, BALANCE, and BIT_ARRAY_SIZE.
 * @param vertices: pointer to where the graph should be stored
 * @param filename: path to .mat filename of input file
 * @return 0 if file cannot be read
 *         1 otherwise
 */
int readFileMat(struct Vertex* (*vertices), const char* filename) {
    EDGES = 0;
    MAX_DEGREE = 0;
    AVG_DEGREE = 0;
    FILE *fpointer;
    fpointer = fopen(filename, "r");
    if (fpointer == NULL) { return 0; }
    int value;
    for(size_t i = 0; i < VERTICES; ++i) {
        for(size_t j = 0; j < VERTICES; ++j) {
            int scan = fscanf(fpointer, "%d", &value);
            if (value == 1) {
                addEdgeSingle(vertices[i], vertices[j]);
                int degree = vertices[i]->degree;
                MAX_DEGREE = (degree > MAX_DEGREE) ? degree : MAX_DEGREE;
                AVG_DEGREE++;
                EDGES++;
            }
        }
    }
    EDGES /= 2;
    AVG_DEGREE /= (float)VERTICES;
    DENSITY = AVG_DEGREE / (float)VERTICES;
    BALANCE = (float)MAX_DEGREE / AVG_DEGREE;
    fclose(fpointer);
    return 1;
}

/**
 * Reads a graph from a .txt input file and stores it in @param vertices.
 * Sets the global variables VERTICES, EDGES, AVG_DEGREE, MAX_DEGREE, DENSITY, BALANCE, and BIT_ARRAY_SIZE.
 * @param vertices: pointer to where the graph should be stored
 * @param filename: path to .txt filename of input file
 * @return 0 if file cannot be read
 *         1 otherwise
 */
int readFileTxt(struct Vertex* (*vertices), const char* filename) {
    EDGES = 0;
    MAX_DEGREE = 0;
    AVG_DEGREE = 0;
    FILE *fpointer;
    fpointer = fopen(filename, "r");
    if (fpointer == NULL) { return 0; }
    int value;
    for(size_t i = 0; i < VERTICES; ++i) {
        for(size_t j = 0; j < VERTICES; ++j) {
            int scan = fscanf(fpointer, "%1d", &value);
            if (value == 1) {
                addEdgeSingle(vertices[i], vertices[j]);
                int degree = vertices[i]->degree;
                MAX_DEGREE = (degree > MAX_DEGREE) ? degree : MAX_DEGREE;
                AVG_DEGREE++;
                EDGES++;
            }
        }
    }
    EDGES /= 2;
    AVG_DEGREE /= (float)VERTICES;
    DENSITY = AVG_DEGREE / (float)VERTICES;
    BALANCE = (float)MAX_DEGREE / AVG_DEGREE;
    fclose(fpointer);
    return 1;
}

/**
 * Reads a graph from a .col input file and stores it in @param vertices.
 * Sets the global variables VERTICES, EDGES, AVG_DEGREE, MAX_DEGREE, DENSITY, BALANCE, and BIT_ARRAY_SIZE.
 * @param vertices: pointer to where the graph should be stored
 * @param filename: path to .col filename of input file
 * @return 0 if file cannot be read
 *         1 otherwise
 */
int readFileCol(struct Vertex* (*vertices), const char* filename) {
    EDGES = 0;
    MAX_DEGREE = 0;
    AVG_DEGREE = 0;
    /* pointer to file */
    FILE *fpointer;
    fpointer = fopen(filename, "r");
    if (fpointer == NULL) { return 0; }
    /* variables for processing */
    char buff[255];
    char * token;
    int v1;
    int v2;
    /* read file line by line */
    while(fscanf(fpointer, "%[^\n] ", buff) != EOF) {
        token = strtok(buff, " ");
        /* read all edges from file */
        if (strcmp(token, "e") == 0) {
            v1 = atoi(strtok(NULL, " "))-1;
            v2 = atoi(strtok(NULL, " "))-1;
            addEdgeDouble(vertices[v1], vertices[v2]);
            int v2degree = vertices[v2]->degree;
            int v1degree = vertices[v1]->degree;
            MAX_DEGREE = (v2degree > MAX_DEGREE) ? v2degree : MAX_DEGREE;
            MAX_DEGREE = (v1degree > MAX_DEGREE) ? v1degree : MAX_DEGREE;
            AVG_DEGREE+=2;
            EDGES++;
        }
    }
    AVG_DEGREE /= (float)VERTICES;
    DENSITY = AVG_DEGREE / (float)VERTICES;
    BALANCE = (float)MAX_DEGREE / AVG_DEGREE;
    fclose(fpointer);
    return 1;
}

// Unsafe because no defined behaviour if character = 0. ctz and clz work with 32 bit numbers.
#define unsafePrev(character, current) (__builtin_ctz(character) - current >= 0 ? -1 : current -__builtin_clz((character) << (32 - current)) - 1)

#define prev(character,current) (character ? unsafePrev(character,current) : -1)

// Returns the number of vertices the graph represented by graphString has.
int getNumberOfVerticesG6(const char * graphString) {
    if(strlen(graphString) == 0){
        printf("Error: String is empty.\n");
        abort();
    }
    else if((graphString[0] < 63 || graphString[0] > 126) && graphString[0] != '>') {
        printf("Error: Invalid start of graphstring.\n");
        abort();
    }

    int index = 0;
    if (graphString[index] == '>') { // Skip >>graph6<< header.
        index += 10;
    }

    if(graphString[index] < 126) { // 0 <= n <= 62
        return (int) graphString[index] - 63;
    }

    else if(graphString[++index] < 126) {
        int number = 0;
        for(int i = 2; i >= 0; i--) {
            number |= (graphString[index++] - 63) << i*6;
        }
        return number;
    }

    else if (graphString[++index] < 126) {
        int number = 0;
        for (int i = 5; i >= 0; i--) {
            number |= (graphString[index++] - 63) << i*6;
        }
        return number;
    }

    else {
        printf("Error: Format only works for graphs up to 68719476735 vertices.\n");
        abort();
    }
}

// Warning: although graph6 can store graphs up to a lot more vertices, only
// works for graphs up to 258047 vertices.
void loadGraphG6(const char * graphString, int numberOfVertices, struct Vertex* (*vertices)) {
    int startIndex = 0;
    if (graphString[startIndex] == '>') { // Skip >>graph6<< header.
        startIndex += 10;
    }
    if (numberOfVertices <= 62) {
        startIndex += 1;
    }
    else if (numberOfVertices <= 258047) {
        startIndex += 4;
    }
    else {
        printf("Error: Program can only handle graphs with %d vertices or fewer.\n",258047);
        abort();
    }

    int currentVertex = 1;
    int sum = 0;

    // This for loop will loop over all vertices labelled from 0 to
    // numberOfVertices - 1.
    for (int index = startIndex; graphString[index] != '\n'; index++) {
        int i;
        // This for loop will loop over all neighbours of the current vertex.
        for (i = prev(graphString[index] - 63, 6); i != -1; i = prev(graphString[index] - 63, i)) {
            while(5-i+(index-startIndex)*6 - sum >= 0) {
                sum += currentVertex;
                currentVertex++;
            }
            sum -= --currentVertex;
            int neighbour = 5-i+(index - startIndex)*6 - sum;

            // Here you should translate into your own format. This point in
            // the code will be called for all pairs of ints
            // (currentVertex, neighbour).
            // E.g. addToGraph(graph, currentVertex, neighbour);
            // For some defined addToGraph method.
            addEdgeSingle(vertices[currentVertex],vertices[neighbour]);
        }
    }
    printAdjacencies(vertices);
}

/**
 * Reads a graph from a .graph6 input file and stores it in @param vertices.
 * Sets the global variables VERTICES, EDGES, AVG_DEGREE, MAX_DEGREE, DENSITY, BALANCE, and BIT_ARRAY_SIZE.
 * @param vertices: pointer to where the graph should be stored
 * @param filename: path to .graph6 filename of input file
 * @return 0 if file cannot be read
 *         1 otherwise
 */
int readFileGraph6(struct Vertex* (*vertices), const char* filename) {
    EDGES = 0;
    MAX_DEGREE = 0;
    AVG_DEGREE = 0;
    FILE *fpointer;
    fpointer = fopen(filename, "r");
    if (fpointer == NULL) { return 0; }
    char buff[255];
    int value;
    int scan = fscanf(fpointer, "%[^\n] ", buff);
    for(size_t i = 0; i < VERTICES; ++i) {
        for(size_t j = 0; j < VERTICES; ++j) {
            scan = fscanf(fpointer, "%1d", &value);
            if (value == 1) { addEdgeSingle(vertices[i], vertices[j]); EDGES++;}
        }
    }
    EDGES /= 2;
    AVG_DEGREE /= (float)VERTICES;
    DENSITY = AVG_DEGREE / (float)VERTICES;
    BALANCE = (float)MAX_DEGREE / AVG_DEGREE;
    fclose(fpointer);
    return 1;
}

/**
 * Reads a graph from a .g6 input file and stores it in @param vertices.
 * Sets the global variables VERTICES, EDGES, AVG_DEGREE, MAX_DEGREE, DENSITY, BALANCE, and BIT_ARRAY_SIZE.
 * @param vertices: pointer to where the graph should be stored
 * @param filename: path to .g6 filename of input file
 * @return 0 if file cannot be read
 *         1 otherwise
 */
int readFileG6(struct Vertex* (*vertices), char* filename) {
    EDGES = 0;
    MAX_DEGREE = 0;
    AVG_DEGREE = 0;
    loadGraphG6(filename, VERTICES, vertices);
    EDGES /= 2;
    AVG_DEGREE /= (float)VERTICES;
    DENSITY = AVG_DEGREE / (float)VERTICES;
    BALANCE = (float)MAX_DEGREE / AVG_DEGREE;
    return 1;
}

/**
 * Reads the graph from the input file @param filename and stores it in @param vertices
 * @param filename: path to input file
 * @param vertices: resulting graph
 */
void readGraph(char filename[100], struct Vertex* (*vertices)) {
    /* DETERMINE FILETYPE */
    char filetype[100];
    strncpy(filetype, filename, 100);
    char * filetypeToken = filetype;
    filetypeToken = strtok(filetypeToken, ".");
    char * nextFiletypeToken = filetypeToken;
    while(nextFiletypeToken != NULL ) {
        filetypeToken = nextFiletypeToken;
        nextFiletypeToken = strtok(NULL, ".");
    }
    /* READ FILE */
    if (strcmp(filetypeToken, "mat") == 0) {
        if (readFileMat(vertices, filename) == 0) {
            printf("> no such file\n");
            freeVertices(vertices);
            exit(5);
        }
    } else if (strcmp(filetypeToken, "col") == 0) {
        if (readFileCol(vertices, filename) == 0) {
            printf("> no such file\n");
            freeVertices(vertices);
            exit(5);
        }
    } else if (strcmp(filetypeToken, "txt") == 0) {
        if (readFileTxt(vertices, filename) == 0) {
            printf("> no such file\n");
            freeVertices(vertices);
            exit(5);
        }
    } else if (strcmp(filetypeToken, "g6") == 0) {

        if (readFileG6(vertices, filename) == 0) {
            printf("> no such file\n");
            freeVertices(vertices);
            exit(5);
        }
    } else if (strcmp(filetypeToken, "graph6") == 0) {
        if (readFileGraph6(vertices, filename) == 0) {
            printf("> no such file\n");
            freeVertices(vertices);
            exit(5);
        }
    } else {
        printf("Filetype <%s> not supported\n", filetypeToken);
        freeVertices(vertices);
        exit(5);
    }
}

/**
 * Creates the line graph of the @param vertexGraph and stores it in @param lineGraph.
 * @param vertexGraph: the original graph
 * @param lineGraph: pointer to where the line graph should be stored
 */
void createLineGraph(struct Vertex* (*vertexGraph), struct Vertex* (*lineGraph)) {
    MAX_DEGREE = 0;
    AVG_DEGREE = 0;
    struct Vertex* lineMatrix[VERTICES][VERTICES];
    for (int i=0; i<VERTICES; i++) {
        for (int j=0; j<VERTICES; j++) {
            lineMatrix[i][j] = NULL;
        }
    }
    qsort(vertexGraph, VERTICES, sizeof(struct Vertex*), &compareByVertexNumber);
    for (int i=0; i<VERTICES; i++) {
        struct ListElem* neighbor = vertexGraph[i]->adjacentVertices;
        while (neighbor!=NULL) {
            int neighborNumber = neighbor->vertex->number;
            struct Vertex* firstVertex = (i<=neighborNumber) ? lineMatrix[i][neighborNumber] : lineMatrix[neighborNumber][i];
            if (firstVertex == NULL) {
                if (i<=neighborNumber) {
                    lineMatrix[i][neighborNumber] = createVertex(1);
                    firstVertex = lineMatrix[i][neighborNumber];
                }
                else {
                    lineMatrix[neighborNumber][i] = createVertex(1);
                    firstVertex = lineMatrix[neighborNumber][i];
                }
            }
            struct ListElem* neighbor1 = neighbor->next;
            while (neighbor1!=NULL) {
                int neighbor1Number = neighbor1->vertex->number;
                struct Vertex* secondVertex = (i<=neighbor1Number) ? lineMatrix[i][neighbor1Number] : lineMatrix[neighbor1Number][i];
                if (secondVertex == NULL) {
                    if (i<=neighbor1Number) {
                        lineMatrix[i][neighbor1Number] = createVertex(1);
                        secondVertex = lineMatrix[i][neighbor1Number];
                    }
                    else {
                        lineMatrix[neighbor1Number][i] = createVertex(1);
                        secondVertex = lineMatrix[neighbor1Number][i];
                    }
                }
                addEdgeDouble(firstVertex, secondVertex);
                neighbor1 = neighbor1->next;
            }
            neighbor = neighbor->next;
        }
    }
    int edge_counter = 0;
    for (int i=0; i<VERTICES; i++) {
        for (int j=0; j<VERTICES; j++) {
            if (lineMatrix[i][j] != NULL) {
                lineGraph[edge_counter] = lineMatrix[i][j];
                lineGraph[edge_counter]->number = edge_counter;
                edge_counter++;
            }
        }
    }
}

#endif