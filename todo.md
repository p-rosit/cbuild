[ ] handle forward declarations of compiler associated with file
[x] keep file id in node instead of pointer to file (pointer might become invalid, id never becomes invalid)
[ ] handle case where cache exists but .o file is missing
[x] separate file and graph data in serialized cache
[ ] move compilation of project to different file
[x] move functions and includes to `bld_file` keep edges in `bld_node`
[ ] handle failed compilation
[ ] make sure no path separator is passed when removing file ending
[ ] use realloc instead of memcpy and free
[x] move value size from function call to generic container struct
[ ] extract exposed functions that can be used by the user
[ ] define private function prototypes somewhere
[ ] hide ubuntu dependencies in building behind interface
[x] find include dependencies and put in graph
[ ] make graph optional
[ ] make cache optional
[x] recompile if file changed due to include dependency
[ ] use cache to populate graph
[ ] split generating dependency graph from indexing project
[ ] detect when .c file was included and disregard when compiling
[ ] detect if .h file is header only
    - unclear what it means for a file to be "header only"
[x] remove duplication in growable arrays
[ ] use hash maps where appropriate
    - hash maps do not seem to be needed, except perhaps for separating the parsing from the intepretation of the parsed data
[ ] implement generic hash map
[x] implement generic hash set for faster access
[x] implement generic array to reduce code duplication

